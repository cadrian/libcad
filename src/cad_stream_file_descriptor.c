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
 * This file contains the implementation of the file descriptor streams.
 */

#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "cad_stream.h"

#define BUFFER_SIZE 4096

struct cad_input_stream_file_descriptor {
     struct cad_input_stream fn;
     cad_memory_t memory;

     int fd;
     char buffer[BUFFER_SIZE];
     int max;
     int index;
};

static void free_input(struct cad_input_stream_file_descriptor *this) {
     this->memory.free(this);
}

static int next(struct cad_input_stream_file_descriptor *this) {
     int result = 0;
     if (this->max) {
          this->index++;
          if (this->index >= this->max) {
               this->max = read(this->fd, this->buffer, BUFFER_SIZE);
               this->index = 0;
               if (this->max < 0) {
                    result = -1;
               }
          }
     }
     return result;
}

static int item(struct cad_input_stream_file_descriptor *this) {
     if (this->max == 0) {
          return EOF;
     }
     return this->buffer[this->index];
}

static cad_input_stream_t input_fn = {
     (cad_input_stream_free_fn)free_input,
     (cad_input_stream_next_fn)next      ,
     (cad_input_stream_item_fn)item      ,
};

__PUBLIC__ cad_input_stream_t *new_cad_input_stream_from_file_descriptor(int fd, cad_memory_t memory) {
     struct cad_input_stream_file_descriptor *result = (struct cad_input_stream_file_descriptor *)memory.malloc(sizeof(struct cad_input_stream_file_descriptor));
     if (!result) return NULL;
     result->fn     = input_fn;
     result->memory = memory;
     result->fd     = fd;
     result->max    = -1;
     result->index  = 0;
     next(result);
     return &(result->fn);
}



struct cad_output_stream_file_descriptor {
     struct cad_output_stream fn;
     cad_memory_t memory;

     int   fd;
     char *buffer;
     int   capacity;
};

static void free_output(struct cad_input_stream_file_descriptor *this) {
     this->memory.free(this->buffer);
     this->memory.free(this);
}

static void put(struct cad_output_stream_file_descriptor *this, const char *format, ...) {
     va_list args;
     int i, n, m;
     va_start(args, format);
     n = vsnprintf(this->buffer, this->capacity, format, args);
     va_end(args);

     if (n >= this->capacity) {
          this->memory.free(this->buffer);
          do {
               this->capacity <<= 1;
          } while (n >= this->capacity);
          this->buffer = this->memory.malloc(this->capacity);

          va_start(args, format);
          n = vsnprintf(this->buffer, this->capacity, format, args);
          va_end(args);
     }

     while (m < n) {
          i = write(this->fd, this->buffer + m, n - m);
          if (!i) {
               // TODO: lost characters!!
               return;
          }
          m += i;
     }
}

static void flush(struct cad_output_stream_file_descriptor *this) {
     fsync(this->fd);
}

static cad_output_stream_t output_fn = {
     (cad_output_stream_free_fn )free_output,
     (cad_output_stream_put_fn  )put        ,
     (cad_output_stream_flush_fn)flush      ,
};

__PUBLIC__ cad_output_stream_t *new_cad_output_stream_from_file_descriptor(int fd, cad_memory_t memory) {
     struct cad_output_stream_file_descriptor *result = (struct cad_output_stream_file_descriptor*)memory.malloc(sizeof(struct cad_output_stream_file_descriptor));
     result->fn       = output_fn;
     result->memory   = memory;
     result->fd       = fd;
     result->buffer   = (char*)memory.malloc(128);
     result->capacity = 128;

     return &(result->fn);
}
