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

#include <stdarg.h>
#include <string.h>

#include "test.h"
#include "cad_hash.h"

struct check_data {
     va_list data;
     int index;
};

static int test_salt(void) {
     return 0;
}

static void check_iterator(void *hash, int index, const void *key, void *value, struct check_data *expected) {
     char *expected_key = va_arg(expected->data, char*);
     void *expected_value = va_arg(expected->data, void*);
     assert(expected->index == index);
     assert(!strcmp(expected_key, key));
     assert(expected_value == value);
     expected->index++;
}

static void check_hash(cad_hash_t *h, int count, ...) {
     struct check_data data;
     assert(h->count(h) == count);

     va_start(data.data, count);
     data.index = 0;
     h->iterate(h, (cad_hash_iterator_fn)check_iterator, &data);
     va_end(data.data);

     assert(count == data.index);
}

int main() {
     set_hash_salt(test_salt);

     cad_hash_t *h = cad_new_hash(stdlib_memory, cad_hash_strings);
     void *foo = (void*)1;
     void *bar = (void*)2;
     void *foo2 = (void*)42;
     void *val;

     assert(h->count(h) == 0);

     h->set(h, "foo", foo);
     assert(h->count(h) == 1);
     val = h->get(h, "foo");
     assert(val == foo);
     check_hash(h, 1, "foo", foo);

     h->set(h, "bar", bar);
     assert(h->count(h) == 2);
     val = h->get(h, "bar");
     assert(val == bar);
     val = h->get(h, "foo");
     assert(val == foo);
     check_hash(h, 2, "foo", foo, "bar", bar);

     h->set(h, "foo", foo2);
     assert(h->count(h) == 2);
     val = h->get(h, "bar");
     assert(val == bar);
     val = h->get(h, "foo");
     assert(val == foo2);
     check_hash(h, 2, "foo", foo2, "bar", bar);

     h->del(h, "foo");
     assert(h->count(h) == 1);
     val = h->get(h, "bar");
     assert(val == bar);
     check_hash(h, 1, "bar", bar);

     h->del(h, "foo");
     assert(h->count(h) == 1);
     val = h->get(h, "bar");
     assert(val == bar);
     check_hash(h, 1, "bar", bar);

     h->del(h, "bar");
     assert(h->count(h) == 0);

     return 0;
}
