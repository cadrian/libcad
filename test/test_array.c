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
#include "cad_array.h"

static void check_array(cad_array_t *a, int count, ...) {
     va_list data;
     int index;
     void *value;
     void **elt;

     assert(a->count(a) == count);

     va_start(data, count);
     for (index = 0; index < count; index++) {
          value = va_arg(data, void*);
          elt = (void**)a->get(a, index);
          assert(value == *elt);
     }
     va_end(data);
}

static int compare(const void *a, const void *b) {
     char *xa = *(char**)a;
     char *xb = *(char**)b;
     if (xa == NULL) {
          if (xb == NULL) {
               return 0;
          }
          return -1;
     }
     if (xb == NULL) {
          return 1;
     }
     return strcmp(xa, xb);
}

int main() {
     cad_array_t *a = cad_new_array(stdlib_memory, sizeof(char*));
     char *foo = "foo";
     char *bar = "bar";
     char *foo2 = "foo2";
     char *bar2 = "bar2";
     char**val;

     assert(a->count(a) == 0);

     a->insert(a, 0, &foo);
     check_array(a, 1, foo);

     a->insert(a, 1, &bar);
     check_array(a, 2, foo, bar);

     a->insert(a, 1, &foo2);
     check_array(a, 3, foo, foo2, bar);

     val = a->update(a, 1, &bar2);
     assert(*val == bar2);
     check_array(a, 3, foo, bar2, bar);

     a->insert(a, 5, &foo2);
     check_array(a, 6, foo, bar2, bar, NULL, NULL, foo2);

     val = a->del(a, 1);
     assert(*val == bar);
     check_array(a, 5, foo, bar, NULL, NULL, foo2);

     a->sort(a, compare);
     check_array(a, 5, NULL, NULL, bar, foo, foo2);

     return 0;
}
