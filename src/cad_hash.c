/*
  This file is part of libCad.

  libCad is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, version 3 of the License.

  libCad is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with libCad.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @ingroup cad_hash
 * @file
 *
 * This file contains the implementation of hash tables. That
 * implementation is a general-purpose hashing table.
 *
 * The original hashing algorithm comes from Python; implementation
 * and bugs are mine.
 */

#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "cad_hash.h"

#define PERTURB_SHIFT 5

typedef struct cad_hash_key {
     const void *key;
     unsigned int hash;
} cad_hash_key_t;

typedef struct cad_hash_entry {
     cad_hash_key_t  key;
     void            *value;
} cad_hash_entry_t;

struct cad_hash_impl {
     cad_hash_t fn;
     cad_memory_t memory;
     cad_hash_keys_t   keys;

     int capacity;
     int count;
     int salt;
     cad_hash_entry_t *entries;
};

static unsigned int string_hash(const char *key) {
     unsigned int result = 0;
     while (*key) {
          result = result * 92821 + *key;
          key++;
     }
     return result;
}

__PUBLIC__ cad_hash_keys_t cad_hash_strings = {
     (cad_hash_keys_hash_fn)string_hash,
     (cad_hash_keys_compare_fn)strcmp,
     (cad_hash_keys_clone_fn)strdup,
     (cad_hash_keys_free_fn)free,
};

static cad_hash_key_t hash(const char *key, cad_hash_keys_t keys) {
     cad_hash_key_t result = { key, keys.hash(key) };
     return result;
}

typedef struct index_context {
     unsigned int index;
     unsigned int perturb;
} index_context_t;

static void next_index_of(index_context_t *context) {
     context->index = (5 * context->index) + 1 + context->perturb;
     context->perturb >>= PERTURB_SHIFT;
}

static int index_of(cad_hash_entry_t *entries, cad_hash_keys_t keys, int capacity, cad_hash_key_t key, int salt, index_context_t *save_context) {
     int result;
     const void *k;
     int found;
     cad_hash_keys_compare_fn cmp = keys.compare;

     index_context_t context = {
          key.hash + salt,
          key.hash,
     };

     result = context.index % capacity;
     k = entries[result].key.key;
     found = k ? !cmp(key.key, k) : 1;

     while (!found) {
          next_index_of(&context);
          result = context.index % capacity;
          k = entries[result].key.key;
          found = k ? !cmp(key.key, k) : 1;
     }

     if (!k) result = -result - 1;

     if (save_context) *save_context = context;

     return result;
}

static void rehash(struct cad_hash_impl *this) {
     int i, index;
     cad_hash_key_t key;

     for (i = 0; i < this->capacity; i++) {
          key = this->entries[i].key;
          if (key.key) {
             index = index_of(this->entries, this->keys, this->capacity, key, this->salt, NULL);
               if (index < 0) {
                    // broken collision cycle, fix it
                    index = -index - 1;
                    this->entries[index].key   = key;
                    this->entries[index].value = this->entries[i].value;
                    this->entries[i].key.key = NULL;
                    this->entries[i].value   = NULL;
                    i = -1; // restart
               }
          }
     }
}

static void grow(struct cad_hash_impl *this, int grow_factor) {
     int new_capacity;
     cad_hash_entry_t *new_entries;
     cad_hash_entry_t field;
     int i, index;
     if (this->capacity == 0) {
          new_capacity = grow_factor * grow_factor;
          this->entries = (cad_hash_entry_t *)this->memory.malloc(new_capacity * sizeof(cad_hash_entry_t));
          memset(this->entries, 0, new_capacity * sizeof(cad_hash_entry_t));
     }
     else {
          new_capacity = this->capacity * grow_factor;
          new_entries = (cad_hash_entry_t *)this->memory.malloc(new_capacity * sizeof(cad_hash_entry_t));
          memset(new_entries, 0, new_capacity * sizeof(cad_hash_entry_t));
          for (i = 0; i < this->capacity; i++) {
               field = this->entries[i];
               if (field.key.key) {
                    index = -index_of(new_entries, this->keys, new_capacity, field.key, this->salt, NULL) - 1;
                    new_entries[index] = field;
               }
          }
          this->memory.free(this->entries);
          this->entries = new_entries;
     }
     this->capacity = new_capacity;
}

static unsigned int count(struct cad_hash_impl *this) {
     return this->count;
}

static void iterate_(struct cad_hash_impl *this, cad_hash_iterator_fn iterator, void *data, int clean) {
     int i, index = 0;
     cad_hash_entry_t entry;
     for (i = 0; index < this->count; i++) {
          entry = this->entries[i];
          if (entry.key.key) {
               iterator(this, index++, entry.key.key, entry.value, data);
               if (clean) {
                    this->keys.free((void*)entry.key.key);
               }
          }
     }
}

static void iterate(struct cad_hash_impl *this, cad_hash_iterator_fn iterator, void *data) {
     iterate_(this, iterator, data, 0);
}

static void clean(struct cad_hash_impl *this, cad_hash_iterator_fn iterator, void *data) {
     iterate_(this, iterator, data, 1);
     this->count = 0;
     memset(this->entries, 0, this->capacity * sizeof(cad_hash_entry_t));
}

static void *get(struct cad_hash_impl *this, const void *key) {
     void *result = NULL;
     int index;
     if (this->capacity) {
          index = index_of(this->entries, this->keys, this->capacity, hash(key, this->keys), this->salt, NULL);
          if (index >= 0) {
               result = this->entries[index].value;
          }
     }
     return result;
}

static void *set(struct cad_hash_impl *this, const void *key, void *value) {
     void *result = NULL;
     int index;
     cad_hash_key_t hkey = hash(key, this->keys);
     if (this->capacity == 0) {
          grow(this, 2);
     }
     index = index_of(this->entries, this->keys, this->capacity, hkey, this->salt, NULL);
     if (index >= 0) {
          result = this->entries[index].value;
     }
     else {
          if (this->count * 3 >= this->capacity * 2) {
               grow(this, 2);
               index = index_of(this->entries, this->keys, this->capacity, hkey, this->salt, NULL);
          }
          index = -index - 1;
          hkey.key = this->keys.clone(key);
          this->entries[index].key = hkey;
          this->count++;
     }
     this->entries[index].value = value;
     return result;
}

static void *del(struct cad_hash_impl *this, const void *key) {
     void *result = NULL;
     cad_hash_key_t hkey = hash(key, this->keys);
     int index = index_of(this->entries, this->keys, this->capacity, hkey, this->salt, NULL);

     if (index >= 0) {
          result = this->entries[index].value;
          this->keys.free((void*)this->entries[index].key.key);
          this->entries[index].key.key = NULL;
          this->entries[index].value   = NULL;
          this->count--;
          rehash(this);
     }
     return result;
}

static void free_(struct cad_hash_impl *this) {
     int i, index = 0;
     cad_hash_entry_t entry;
     for (i = 0; index < this->count; i++) {
          entry = this->entries[i];
          if (entry.key.key) {
               index++;
               this->keys.free((void*)entry.key.key);
          }
     }
     this->memory.free(this->entries);
     this->memory.free(this);
}

static cad_hash_t fn = {
     (cad_hash_free_fn   )free_  ,
     (cad_hash_count_fn  )count  ,
     (cad_hash_iterate_fn)iterate,
     (cad_hash_get_fn    )get    ,
     (cad_hash_set_fn    )set    ,
     (cad_hash_del_fn    )del    ,
     (cad_hash_clean_fn  )clean  ,
};

static void init_rand(void) {
   int seed = 0;
   int fd = open("/dev/random", O_RDONLY);
   int ok = 0;
   if (fd != -1) {
      ssize_t n = read(fd, &seed, sizeof(int));
      if (n > 0) {
         srand(seed);
         ok = 1;
      }
      close(fd);
   }
   if (!ok) {
      srand(time(NULL));
   }
}

int default_hash_salt(void) {
   static int init = 0;
   if (!init) {
      init_rand();
      init = 1;
   }
   return rand();
}

static hash_salt_fn salt = default_hash_salt;

__PUBLIC__ cad_hash_t *cad_new_hash(cad_memory_t memory, cad_hash_keys_t keys) {
     struct cad_hash_impl *result = (struct cad_hash_impl *)memory.malloc(sizeof(struct cad_hash_impl));
     if (!result) return NULL;
     result->fn       = fn;
     result->memory   = memory;
     result->keys     = keys;
     result->capacity = 0;
     result->count    = 0;
     result->salt     = salt();
     result->entries  = NULL;
     return (cad_hash_t*)result;
}

__PUBLIC__ void set_hash_salt(hash_salt_fn new_salt) {
   salt = new_salt == NULL ? default_hash_salt : new_salt;
}
