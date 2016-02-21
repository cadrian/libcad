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

#include <string.h>

#include "test.h"
#include "cad_stache.h"
#include "cad_stream.h"

#define TEMPLATE "<html><head><title>{{{title}}}</title></head><body><h1>{{title}}</h1><ul>{{#list}}<li>{{data}}</li>{{/list}}{{^list}}<li>NO DATA</li>{{/list}}</ul></body></html>"
#define EXPECTED_RESULT "<html><head><title>Test 'stache</title></head><body><h1>Test &apos;stache</h1><ul><li>foo</li><li>bar</li></ul></body></html>"

static const char *error_message = NULL;
static int error_index = 0;
static int error = 0;
static void on_error(const char *m, int i) {
   fprintf(stderr, "**** %s at %d\n", m, i);
   error_message = m;
   error_index = i;
   error = 1;
}

static cad_stache_resolved_t *data;

static const char *get_title(cad_stache_resolved_t *this);
static int free_title(cad_stache_resolved_t *this);
static cad_stache_resolved_t title = {
   .string = {
      .get = get_title,
      .free = free_title,
   },
};
static const char *get_title(cad_stache_resolved_t *this) {
   assert(this == &title);
   return "Test 'stache";
}
static int free_title(cad_stache_resolved_t *this) {
   assert(this == &title);
   return 1;
}

static const char *get_data1(cad_stache_resolved_t *this);
static int free_data1(cad_stache_resolved_t *this);
static cad_stache_resolved_t data1 = {
   .string = {
      .get = get_data1,
      .free = free_data1,
   },
};
static const char *get_data1(cad_stache_resolved_t *this) {
   assert(this == &data1);
   assert(this == data);
   return "foo";
}
static int free_data1(cad_stache_resolved_t *this) {
   assert(this == &data1);
   assert(this == data);
   return 1;
}

static const char *get_data2(cad_stache_resolved_t *this);
static int free_data2(cad_stache_resolved_t *this);
static cad_stache_resolved_t data2 = {
   .string = {
      .get = get_data2,
      .free = free_data2,
   },
};
static const char *get_data2(cad_stache_resolved_t *this) {
   assert(this == &data2);
   assert(this == data);
   return "bar";
}
static int free_data2(cad_stache_resolved_t *this) {
   assert(this == &data2);
   assert(this == data);
   return 1;
}

static int get_list(cad_stache_resolved_t *this, int index);
static int close_list(cad_stache_resolved_t *this);
static cad_stache_resolved_t list = {
   .list = {
      .get = get_list,
      .close = close_list,
   },
};
static int get_list(cad_stache_resolved_t *this, int index) {
   int result = 0;
   assert(this == &list);
   switch(index) {
   case 0:
      data = &data1;
      result = 1;
      break;
   case 1:
      data = &data2;
      result = 1;
      break;
   default:
      data = NULL;
      break;
   }
   return result;
}
static int close_list(cad_stache_resolved_t *this) {
   assert(this == &list);
   return 1;
}

static cad_stache_lookup_type stache_callback(cad_stache_t *stache, const char *name, cad_stache_resolved_t **resolved) {
   cad_stache_lookup_type result = Cad_stache_not_found;

   assert(stache != NULL);
   assert(name != NULL);
   assert(strlen(name) > 0);
   assert(resolved != NULL);

   if (!strcmp(name, "title")) {
      result = Cad_stache_string;
      *resolved = &title;
   } else if (!strcmp(name, "list")) {
      result = Cad_stache_list;
      *resolved = &list;
   } else if (!strcmp(name, "data")) {
      if (data) {
         result = Cad_stache_string;
         *resolved = data;
      }
   } else {
      printf("INVALID NAME: %s\n", name);
      assert(0 /* invalid name */);
   }

   return result;
}

int main() {
   cad_stache_t *stache = new_cad_stache(stdlib_memory, stache_callback);
   assert(stache != NULL);
   cad_input_stream_t *input = new_cad_input_stream_from_string(TEMPLATE, stdlib_memory);
   char *output_string;
   cad_output_stream_t *output = new_cad_output_stream_from_string(&output_string, stdlib_memory);

   stache->render(stache, input, output, on_error);
   assert(error == 0);

   assert(!strcmp(output_string, EXPECTED_RESULT));

   stdlib_memory.free(output_string);
   output->free(output);
   input->free(input);
   stache->free(stache);

   return 0;
}
