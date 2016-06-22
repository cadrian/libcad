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
 * @ingroup cad_stache
 * @file
 *
 * This file contains the implementation of the 'stache engine, which
 * is meant to be complatible with Mustache.
 *
 * http://mustache.github.io/
 */

#include <stdarg.h>
#include <string.h>

#include "cad_array.h"
#include "cad_stache.h"

struct cad_stache_impl {
   cad_stache_t fn;
   cad_memory_t memory;
   cad_stache_resolve_cb callback;
   void *cb_data;
   char *open;
   char *close;
};

enum loop_type {
   loop_else = 0,
   loop_while,
};

struct loop {
   enum loop_type type;
   char *name;
   cad_stache_lookup_type lookup_type;
   cad_stache_resolved_t *resolved;
   int buffer_index;
   int loop_index;
   int empty;
};

struct partial {
   cad_stache_lookup_type lookup_type;
   cad_stache_resolved_t *resolved;
   cad_input_stream_t *stream;
   char save;
};

struct buffer {
   cad_memory_t memory;
   cad_input_stream_t *input;
   char *data;
   int index;
   int fill;
   int capacity;
   int eof;
   cad_array_t *partials;
   cad_array_t *loops;
   const char *error;
};

static void free_(struct cad_stache_impl *this) {
   this->memory.free(this);
}

static int buffer_next(struct buffer *buffer) {
   int result = 1;

   if (!buffer->eof) {
      buffer->index++;
      if (buffer->index == buffer->fill && !buffer->eof) {
         if (buffer->fill == buffer->capacity) {
            buffer->capacity = buffer->capacity * 2;
            buffer->data = buffer->memory.realloc(buffer->data, buffer->capacity);
         }
         int np = buffer->partials->count(buffer->partials);

         cad_input_stream_t *input;
         struct partial *partial;
         if (np > 0) {
            partial = buffer->partials->get(buffer->partials, np - 1);
            input = partial->stream;
         } else {
            input = buffer->input;
         }

         int c = input->item(input);
         if (c == -1) {
            if (np > 0) {
               switch(partial->lookup_type) {
               case Cad_stache_not_found:
               case Cad_stache_list:
                  buffer->error = "BUG unexpected type";
                  break;
               case Cad_stache_string:
                  c = partial->save;
                  input->free(input);
                  partial->resolved->string.free(partial->resolved);
                  break;
               case Cad_stache_partial:
                  c = partial->save;
                  // don't free() input, the contract is that the
                  // implementation of a partial free() should take
                  // care of it (we don't own the stream)
                  partial->resolved->partial.free(partial->resolved);
                  break;
               }
               buffer->partials->del(buffer->partials, --np);
               buffer->data[buffer->fill++] = c;
            } else {
               buffer->eof = 1;
            }
         } else {
            buffer->data[buffer->fill++] = c;
            if (input->next(input)) {
               buffer->eof = 1;
            }
         }

      } else {
         // OK, already filled
      }
   } else {
      result = 0;
      buffer->error = "Buffer end reached";
   }
   return result;
}

static int buffer_eof(struct buffer *buffer) {
   return buffer->eof && buffer->index >= buffer->fill;
}

static char buffer_item(struct buffer *buffer) {
   return buffer->data[buffer->index];
}

static int buffer_look_at(struct buffer *buffer, const char *expected) {
   int result = 1;
   int index = buffer->index;
   int i, n = strlen(expected);

   for (i = 0; result && i < n; i++) {
      if (buffer_item(buffer) == expected[i]) {
         if (!buffer_next(buffer)) {
            result = 0;
         }
      } else {
         result = 0;
      }
   }

   if (!result) {
      buffer->index = index;
   }
   return result;
}

static void buffer_unwind_loops(struct buffer *buffer){
   int i, n = buffer->loops->count(buffer->loops);
   struct loop *loop;
   for (i = 0; i < n; i++) {
      loop = buffer->loops->get(buffer->loops, i);
      buffer->memory.free(loop->name);
   }
   buffer->loops->clear(buffer->loops);
}

static int buffer_skip_output(struct buffer *buffer) {
   int result = 0;
   int n = buffer->loops->count(buffer->loops);
   while (!result && n > 0) {
      struct loop *loop = buffer->loops->get(buffer->loops, n - 1);
      if (loop->type == loop_while) {
         result = loop->empty;
      } else {
         result = !loop->empty;
      }
      n--;
   }
   return result;
}

void buffer_output(struct buffer *buffer, cad_output_stream_t *output, const char *format, ...) {
   if (!buffer_skip_output(buffer)) {
      va_list args;
      va_start(args, format);
      output->vput(output, format, args);
      va_end(args);
   }
}

static void buffer_skip_blanks(struct buffer *buffer) {
   int again = 1;
   while (again && !buffer_eof(buffer)) {
      char c = buffer_item(buffer);
      switch(c) {
      case ' ':
      case '\n':
         again = buffer_next(buffer);
         break;
      default:
         again = 0;
         break;
      }
   }
}

static char *parse_name(struct cad_stache_impl *this, const char *suffix, struct buffer *buffer) {
   int capacity = 16, n = 0;
   int index = buffer->index;
   char *result = NULL;
   char *name = this->memory.malloc(capacity);
   int more = 1;
   buffer_skip_blanks(buffer);
   while (more && result == NULL && !buffer_eof(buffer)) {
      if (suffix != NULL && buffer_look_at(buffer, suffix)) {
         if (buffer_look_at(buffer, this->close)) {
            result = name;
         } else {
            more = 0;
         }
      } else if (buffer_look_at(buffer, this->close)) {
         if (suffix == NULL) {
            result = name;
         } else {
            more = 0;
         }
      } else {
         if (capacity == n) {
            capacity *= 2;
            name = this->memory.realloc(name, capacity);
         }
         name[n++] = buffer_item(buffer);
         more = buffer_next(buffer);
      }
   }
   if (result != NULL) {
      result[n] = 0;
   } else {
      buffer->error = "Unclosed or invalid 'stache";
      this->memory.free(name);
      buffer->index = index;
   }
   return result;
}

static int render_stache_raw(struct cad_stache_impl *this, struct buffer *buffer, cad_output_stream_t *output) {
   int result = 1;
   char *name = parse_name(this, "}", buffer);
   if (name != NULL) {
      if (*name == 0) {
         buffer->error = "invalid raw name";
         result = 0;
      } else {
         cad_stache_resolved_t *resolved;
         cad_stache_lookup_type type = this->callback((cad_stache_t*)this, name, this->cb_data, &resolved);
         const char *c;
         switch(type) {
         case Cad_stache_not_found:
            // nothing written
            break;
         case Cad_stache_string:
            c = resolved->string.get(resolved);
            buffer_output(buffer, output, "%s", c);
            if (!resolved->string.free(resolved)) {
               buffer->error = "could not free string";
               result = 0;
            }
            break;
         case Cad_stache_list:
            buffer->error = "unexpected list";
            result = 0;
            break;
         case Cad_stache_partial:
            buffer->error = "unexpected partial";
            result = 0;
            break;
         default:
            buffer->error = "invalid type";
            result = 0;
            break;
         }
      }
      this->memory.free(name);
   }
   return result;
}

static int render_stache_escape(struct cad_stache_impl *this, struct buffer *buffer, cad_output_stream_t *output) {
   int result = 1;
   char *name = parse_name(this, NULL, buffer);
   if (name != NULL) {
      if (*name == 0) {
         buffer->error = "invalid escape name";
         result = 0;
      } else {
         cad_stache_resolved_t *resolved;
         cad_stache_lookup_type type = this->callback((cad_stache_t*)this, name, this->cb_data, &resolved);
         const char *c;
         switch(type) {
         case Cad_stache_not_found:
            // nothing written
            break;
         case Cad_stache_string:
            c = resolved->string.get(resolved);
            while (*c) {
               switch(*c) {
               case '<':
                  buffer_output(buffer, output, "%s", "&lt;");
                  break;
               case '>':
                  buffer_output(buffer, output, "%s", "&gt;");
                  break;
               case '&':
                  buffer_output(buffer, output, "%s", "&amp;");
                  break;
               case '"':
                  buffer_output(buffer, output, "%s", "&quot;");
                  break;
               case '\'':
                  buffer_output(buffer, output, "%s", "&apos;");
                  break;
               default:
                  buffer_output(buffer, output, "%c", *c);
                  break;
               }
               c++;
            }
            if (!resolved->string.free(resolved)) {
               buffer->error = "could not free string";
               result = 0;
            }
            break;
         case Cad_stache_list:
            buffer->error = "unexpected list";
            result = 0;
            break;
         case Cad_stache_partial:
            buffer->error = "unexpected partial";
            result = 0;
            break;
         default:
            buffer->error = "invalid type";
            result = 0;
            break;
         }
      }
      this->memory.free(name);
   }
   return result;
}

static int render_stache_loop(struct cad_stache_impl *this, struct buffer *buffer, cad_output_stream_t *output, enum loop_type loop_type) {
   int result = 1;
   char *name = parse_name(this, NULL, buffer);
   if (name != NULL) {
      cad_stache_resolved_t *resolved;
      cad_stache_lookup_type type = this->callback((cad_stache_t*)this, name, this->cb_data, &resolved);
      const char *c;
      cad_input_stream_t *s;
      struct loop loop = { loop_type, name, type, NULL, buffer->index, 0, 0 };
      switch(type) {
      case Cad_stache_not_found:
         loop.empty = 1;
         break;
      case Cad_stache_string:
         c = resolved->string.get == NULL ? NULL : resolved->string.get(resolved);
         loop.empty = (c == NULL) || (*c == 0);
         if (resolved->string.free == NULL || !resolved->string.free(resolved)) {
            buffer->error = "could not free string";
            result = 0;
         }
         break;
      case Cad_stache_list:
         loop.resolved = resolved;
         loop.empty = (resolved->list.get == NULL || resolved->list.get(resolved, 0) == 0);
         break;
      case Cad_stache_partial:
         s = resolved->partial.get == NULL ? NULL : resolved->partial.get(resolved);
         if (s == NULL) {
            loop.empty = 1;
         } else if (s->next(s)) {
            buffer->error = "error while reading partial";
            result = 0;
         } else if (s->item(s) == -1) {
            loop.empty = 1;
         } else {
            loop.empty = 0;
         }
         if (resolved->partial.free == NULL || !resolved->partial.free(resolved)) {
            buffer->error = "could not free partial";
            result = 0;
         }
         break;
      default:
         buffer->error = "invalid type";
         result = 0;
         break;
      }
      buffer->loops->insert(buffer->loops, buffer->loops->count(buffer->loops), &loop);
   }
   return result;
}

static int render_stache_while(struct cad_stache_impl *this, struct buffer *buffer, cad_output_stream_t *output) {
   return render_stache_loop(this, buffer, output, loop_while);
}

static int render_stache_else(struct cad_stache_impl *this, struct buffer *buffer, cad_output_stream_t *output) {
   return render_stache_loop(this, buffer, output, loop_else);
}

static int render_stache_end(struct cad_stache_impl *this, struct buffer *buffer, cad_output_stream_t *output) {
   int result = 1;
   char *name = parse_name(this, NULL, buffer);
   if (name != NULL) {
      if (buffer->loops->count(buffer->loops) == 0) {
         buffer->error = "closing non-open loop";
         result = 0;
      } else {
         struct loop *loop = buffer->loops->get(buffer->loops, buffer->loops->count(buffer->loops) - 1);
         if (strcmp(name, loop->name)) {
            buffer->error = "closing loop name different from opening loop name";
            result = 0;
         } else {
            switch(loop->lookup_type) {
            case Cad_stache_not_found:
               buffer->loops->del(buffer->loops, buffer->loops->count(buffer->loops) - 1);
               this->memory.free(loop->name);
               break;
            case Cad_stache_string:
            case Cad_stache_partial:
               buffer->loops->del(buffer->loops, buffer->loops->count(buffer->loops) - 1);
               this->memory.free(loop->name);
               break;
            case Cad_stache_list:
               loop->loop_index++;
               int get = loop->type == loop_else ? 0 : loop->resolved->list.get(loop->resolved, loop->loop_index);
               if (get == 0) {
                  if (loop->resolved->list.close != NULL && loop->resolved->list.close(loop->resolved)) {
                     buffer->loops->del(buffer->loops, buffer->loops->count(buffer->loops) - 1);
                  } else {
                     buffer->error = "could not close loop list";
                     result = 0;
                  }
                  this->memory.free(loop->name);
               } else {
                  buffer->index = loop->buffer_index;
               }
            }
         }
      }
      this->memory.free(name);
   }
   return result;
}

static int render_stache_delimiters(struct cad_stache_impl *this, struct buffer *buffer) {
   int result = 1;
   char *delimiters = parse_name(this, "=", buffer);

   if (delimiters == NULL) {
      result = 0;
   } else {
      char *open_delimiter = delimiters;
      char *close_delimiter = delimiters;

      int found_space = 0;
      int found_close = 0;

      while (!found_space) {
         switch(*close_delimiter) {
         case ' ':
         case '\n':
            found_space = 1;
            *close_delimiter = 0;
            break;
         }
         close_delimiter++;
      }

      while (!found_close && *close_delimiter) {
         switch(*close_delimiter) {
         case ' ':
         case '\n':
            close_delimiter++;
            break;
         default:
            found_close = 1;
            break;
         }
      }

      if (*close_delimiter) {
         if (strchr(close_delimiter, ' ') == NULL && strchr(close_delimiter, '\n') == NULL) {
            int n = strlen(close_delimiter);
            if (n > 1 && close_delimiter[n - 1] == '=') {
               close_delimiter[n - 1] = 0;

               this->memory.free(this->open);
               this->memory.free(this->close);

               this->open = this->memory.malloc(strlen(open_delimiter) + 1);
               sprintf(this->open, "%s", open_delimiter);

               this->close = this->memory.malloc(n);
               sprintf(this->close, "%s", close_delimiter);
            } else {
               buffer->error = "invalid delimiter change: missing closing equal sign";
               result = 0;
            }
         } else {
            buffer->error = "invalid delimiter change: no space allowed in close delimiter";
            result = 0;
         }
      } else {
         buffer->error = "invalid delimiter change: close delimiter not found";
         result = 0;
      }

      this->memory.free(delimiters);
   }
   return result;
}

static int render_stache_comment(struct cad_stache_impl *this, struct buffer *buffer) {
   int result = 1;
   while (result && !buffer_look_at(buffer, this->close)) {
      result = buffer_next(buffer);
   }
   return result;
}

static int render_stache_partial(struct cad_stache_impl *this, struct buffer *buffer, cad_output_stream_t *output, int start_index) {
   int result = 1;
   char *name = parse_name(this, NULL, buffer);
   if (name != NULL) {
      cad_stache_resolved_t *resolved;
      cad_stache_lookup_type type = this->callback((cad_stache_t*)this, name, this->cb_data, &resolved);
      const char *c;
      struct partial partial = { type, resolved, NULL, '\0' };
      switch(type) {
      case Cad_stache_not_found:
         // OK, empty partial
         break;
      case Cad_stache_string:
         c = resolved->string.get(resolved);
         if (c != NULL) {
            partial.stream = new_cad_input_stream_from_string(c, this->memory);
         } else {
            resolved->string.free(resolved);
         }
         break;
      case Cad_stache_list:
         buffer->error = "unexpected list for partial";
         result = 0;
         resolved->list.close(resolved);
         break;
      case Cad_stache_partial:
         partial.stream = resolved->partial.get(resolved);
         if (partial.stream == NULL) {
            resolved->partial.free(resolved);
         }
         break;
      }
      if (partial.stream != NULL) {
         partial.save = buffer_item(buffer);
         buffer->partials->insert(buffer->partials, buffer->partials->count(buffer->partials), &partial);
      }

      /*
       * In all cases, we must remove the {{>...}} from the buffer
       * otherwise the result is incorrect, esp. when looping.
       *
       * By design, this happens only when first reading the buffer
       * even in a loop.
       */
      buffer->index = start_index - 1;
      buffer->fill = start_index;
      if (!buffer_next(buffer)) {
         buffer->error = "BUG: no next at partial start";
         result = 0;
      }
   }
   return result;
}

static int render_stache(struct cad_stache_impl *this, struct buffer *buffer, cad_output_stream_t *output, int start_index) {
   int result = 1;
   if (buffer_eof(buffer)) {
      buffer->error = "Invalid 'stache: nothing found after 'stache opening";
      result = 0;
   } else {
      char next = buffer_item(buffer);
      if (!buffer_next(buffer)) {
         buffer->error = "Invalid 'stache: nothing found after 'stache opening";
         result = 0;
      } else {
         switch(next) {
         case '{':
            result = render_stache_raw(this, buffer, output);
            break;
         case '#':
            result = render_stache_while(this, buffer, output);
            break;
         case '/':
            result = render_stache_end(this, buffer, output);
            break;
         case '^':
            result = render_stache_else(this, buffer, output);
            break;
         case '=':
            result = render_stache_delimiters(this, buffer);
            break;
         case '!':
            result = render_stache_comment(this, buffer);
            break;
         case '>':
            result = render_stache_partial(this, buffer, output, start_index);
            break;
         default:
            buffer->index--;
            result = render_stache_escape(this, buffer, output);
         }
      }
   }
   return result;
}

static int render_template(struct cad_stache_impl *this, struct buffer *buffer, cad_output_stream_t *output) {
   int result = 1;
   while (result && !buffer_eof(buffer)) {
      int index = buffer->index;
      if (buffer_look_at(buffer, this->open)) {
         result = render_stache(this, buffer, output, index);
      } else {
         char c = buffer_item(buffer);
         buffer_output(buffer, output, "%c", c);
         result = buffer_next(buffer);
      }
   }
   return result;
}

static int render(struct cad_stache_impl *this, cad_input_stream_t *input, cad_output_stream_t *output, void (*on_error)(const char *, int, void*), void *on_error_data) {
   int result = 0;
   this->open = this->memory.malloc(3);
   sprintf(this->open, "%s", "{{");
   this->close = this->memory.malloc(3);
   sprintf(this->close, "%s", "}}");

   struct buffer buffer = {
      .memory   = this->memory,
      .input    = input,
      .data     = this->memory.malloc(4096),
      .index    =   -1,
      .fill     =    0,
      .capacity = 4096,
      .eof      = 0,
      .partials = cad_new_array(this->memory, sizeof(struct partial)),
      .loops    = cad_new_array(this->memory, sizeof(struct loop)),
      .error    = "",
   };

   if (buffer_next(&buffer)) {
      if (render_template(this, &buffer, output)) {
         if (buffer.loops->count(buffer.loops) > 0) {
            if (on_error != NULL) {
               on_error("Unfinished loops", buffer.index, on_error_data);
            }
            result = -1;
         }
      } else {
         if (on_error != NULL) {
            on_error(buffer.error, buffer.index, on_error_data);
         }
         result = -1;
      }
   } else {
      if (on_error != NULL) {
         on_error("Empty buffer", 0, on_error_data);
      }
      result = -1;
   }

   this->memory.free(this->open);
   this->memory.free(this->close);
   buffer_unwind_loops(&buffer);
   buffer.loops->free(buffer.loops);
   this->memory.free(buffer.data);
   return result;
}

static cad_stache_t fn = {
   (cad_stache_free_fn  )free_ ,
   (cad_stache_render_fn)render,
};

cad_stache_t *new_cad_stache(cad_memory_t memory, cad_stache_resolve_cb callback, void *data) {
   struct cad_stache_impl *result = memory.malloc(sizeof(struct cad_stache_impl));
   if (!result) return NULL;
   result->fn       = fn;
   result->memory   = memory;
   result->callback = callback;
   result->cb_data  = data;
   return (cad_stache_t*)result;
}
