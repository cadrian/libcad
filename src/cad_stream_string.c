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
 * @ingroup cad_stream
 * @file
 *
 * This file contains the implementation of the string streams.
 */

#include <string.h>
#include <stdarg.h>

#include "cad_stream.h"

struct cad_input_stream_string {
     struct cad_input_stream fn;
     cad_memory_t memory;

     const char *string;
     int index;
};

static void free_input(struct cad_input_stream_string *this) {
     this->memory.free(this);
}

static int next(struct cad_input_stream_string *this) {
     if (this->string[this->index]) {
          this->index++;
     }
     return 0;
}

static int item(struct cad_input_stream_string *this) {
     int result = this->string[this->index];
     return result ? result : EOF;
}

static cad_input_stream_t input_fn = {
     (cad_input_stream_free_fn)free_input,
     (cad_input_stream_next_fn)next      ,
     (cad_input_stream_item_fn)item      ,
};

__PUBLIC__ cad_input_stream_t *new_cad_input_stream_from_string(const char *string, cad_memory_t memory) {
     struct cad_input_stream_string *result = (struct cad_input_stream_string *)memory.malloc(sizeof(struct cad_input_stream_string));
     if (!result) return NULL;
     result->fn     = input_fn;
     result->memory = memory;
     result->string = string;
     result->index  = 0;
     return &(result->fn);
}



struct cad_output_stream_string {
     struct cad_output_stream fn;
     cad_memory_t memory;

     char **string;
     int capacity;
     int count;
};

static void free_output(struct cad_input_stream_string *this) {
     this->memory.free(this);
}

static void vput(struct cad_output_stream_string *this, const char *format, va_list args) {
     int c;
     int new_capacity = this->capacity;
     char *string = *(this->string);
     char *new_string = string;
     va_list args0;

     va_copy(args0, args);
     c = vsnprintf("", 0, format, args0) + 1;
     va_end(args0);

     if (new_capacity == 0) {
          new_capacity = 128;
     }
     while (c + this->count > new_capacity) {
          new_capacity *= 2;
     }
     if (new_capacity > this->capacity) {
          new_string = (char *)this->memory.malloc(new_capacity);
          if (string) {
               strcpy(new_string, string);
          }
          *(this->string) = new_string;
          this->capacity = new_capacity;
     }

     this->count += vsprintf(new_string + this->count, format, args);
}

static void put(struct cad_output_stream_string *this, const char *format, ...) {
     va_list args;
     va_start(args, format);
     vput(this, format, args);
     va_end(args);
}

static void flush(struct cad_output_stream_string *this) {
     /* do nothing */
}

static cad_output_stream_t output_fn = {
     (cad_output_stream_free_fn )free_output,
     (cad_output_stream_put_fn  )put        ,
     (cad_output_stream_vput_fn )vput       ,
     (cad_output_stream_flush_fn)flush      ,
};

__PUBLIC__ cad_output_stream_t *new_cad_output_stream_from_string(char **string, cad_memory_t memory) {
     struct cad_output_stream_string *result = (struct cad_output_stream_string *)memory.malloc(sizeof(struct cad_output_stream_string));
     if (!result) return NULL;
     result->fn       = output_fn;
     result->memory   = memory;
     result->string   = string;
     result->capacity = 0;
     result->count    = 0;
     *string = NULL;
     return &(result->fn);
}
