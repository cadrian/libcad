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
 * @ingroup cad_array
 * @file
 *
 * This file contains the implementation of arrays. That
 * implementation is a general-purpose array.
 */

#include <string.h>

#include "cad_array.h"

struct cad_array_impl {
     cad_array_t fn;
     cad_memory_t memory;

     int capacity;
     int count;
     int eltsize;

     void *content;
};

static void free_(struct cad_array_impl *this) {
     this->memory.free(this->content);
     this->memory.free(this);
}

static unsigned int count(struct cad_array_impl *this) {
     return this->count;
}

static void *get(struct cad_array_impl *this, unsigned int index) {
     void *result = NULL;
     if (index < this->count) {
          result = this->content + index * this->eltsize;
     }
     return result;
}

static void grow(struct cad_array_impl *this, int grow_factor) {
     int new_capacity;
     if (this->capacity == 0) {
          new_capacity = grow_factor * grow_factor;
          this->content = this->memory.malloc(new_capacity * this->eltsize);
          memset(this->content, 0, new_capacity * this->eltsize);
     } else {
          new_capacity = this->capacity * grow_factor;
          this->content = this->memory.realloc(this->content, new_capacity * this->eltsize);
          memset(this->content + this->count * this->eltsize, 0, this->count * this->eltsize);
     }
     this->capacity = new_capacity;
}

static void *insert(struct cad_array_impl *this, unsigned int index, void *value) {
     void *result;
     if (this->count == this->capacity) {
          grow(this, 2);
     }
     while (index >= this->capacity) {
          grow(this, 2);
     }
     result = this->content + index * this->eltsize;
     if (index < this->count) {
          bcopy(result, result + this->eltsize, (this->count - index) * this->eltsize);
     }
     memcpy(result, value, this->eltsize);
     this->count = index < this->count ? this->count + 1 : index + 1;
     return result;
}

static void *update(struct cad_array_impl *this, unsigned int index, void *value) {
     void *result;
     while (index >= this->capacity) {
          grow(this, 2);
     }
     result = this->content + index * this->eltsize;
     memcpy(result, value, this->eltsize);
     this->count = index < this->count ? this->count : index + 1;
     return result;
}

static void *del(struct cad_array_impl *this, unsigned int index) {
     void *result = NULL;
     if (index < this->count) {
          result = this->content + index * this->eltsize;
          this->count--;
          if (this->count > index) {
               bcopy(result + this->eltsize, result, (this->count - index) * this->eltsize);
               memset(this->content + (this->count + 1) * this->eltsize, 0, this->eltsize);
          }
     }
     return result;
}

static void sort(struct cad_array_impl *this, comparator_fn comparator) {
     qsort(this->content, this->count, this->eltsize, comparator);
}

static void clear(struct cad_array_impl *this) {
     memset(this->content, 0, this->capacity * this->eltsize);
     this->count = 0;
}

static cad_array_t fn = {
     (cad_array_free_fn   )free_  ,
     (cad_array_count_fn  )count  ,
     (cad_array_get_fn    )get    ,
     (cad_array_insert_fn )insert ,
     (cad_array_update_fn )update ,
     (cad_array_del_fn    )del    ,
     (cad_array_sort_fn   )sort   ,
     (cad_array_clear_fn  )clear  ,
};

__PUBLIC__ cad_array_t *cad_new_array(cad_memory_t memory, size_t size) {
     struct cad_array_impl *result = (struct cad_array_impl *)memory.malloc(sizeof(struct cad_array_impl));
     if (!result) return NULL;
     result->fn      = fn;
     result->memory  = memory;
     result->capacity= 0;
     result->count   = 0;
     result->content = NULL;
     result->eltsize = size;
     return (cad_array_t*)result;
}
