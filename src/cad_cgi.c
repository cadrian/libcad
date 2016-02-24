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
 * @ingroup cad_cgi
 * @file
 *
 * This file contains the implementation of the CGI interface.
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "cad_cgi_internal.h"
#include "cad_hash.h"

typedef struct {
   cad_cgi_response_t fn;
   cad_memory_t memory;
   cad_cgi_cookies_t *cookies;
   char *body;
   cad_output_stream_t *body_stream;
   char *redirect_path;
   char *redirect_fragment;
   int status;
   char *content_type;
   cad_hash_t *headers;
} response_impl;

static void clean_header(void *hash, int index, const char *key, char *value, response_impl *response) {
   response->memory.free(value);
}

static void free_response(response_impl *this) {
   free_cookies(this->cookies);
   this->memory.free(this->body);
   this->body_stream->free(this->body_stream);
   this->memory.free(this->redirect_path);
   this->memory.free(this->redirect_fragment);
   this->memory.free(this->content_type);
   this->headers->clean(this->headers, (cad_hash_iterator_fn)clean_header, this);
   this->headers->free(this->headers);
   this->memory.free(this);
}

static cad_cgi_cookies_t *cookies(response_impl *this) {
   return this->cookies;
}

static int response_fd(response_impl *this) {
   return STDOUT_FILENO;
}

static cad_output_stream_t *body(response_impl *this) {
   return this->body_stream;
}

static int redirect(response_impl *this, const char *path, const char *fragment) {
   int result = -1;
   if (this->body == NULL && path != NULL && fragment != NULL) {
      this->memory.free(this->redirect_path);
      this->memory.free(this->redirect_fragment);
      this->redirect_path = this->memory.malloc(strlen(path) + 1);
      if (this->redirect_path) {
         strcpy(this->redirect_path, path);
         this->redirect_fragment = this->memory.malloc(strlen(fragment) + 1);
         if (this->redirect_fragment) {
            strcpy(this->redirect_fragment, fragment);
            result = 0;
         } else {
            this->memory.free(this->redirect_path);
            this->redirect_path = NULL;
         }
      } else {
         this->redirect_fragment = NULL;
      }
   }
   return result;
}

static int set_status(response_impl *this, int status) {
   this->status = status;
   return 0;
}

static int set_content_type(response_impl *this, const char *content_type) {
   int result = -1;
   if (content_type != NULL) {
      this->memory.free(this->content_type);
      this->content_type = this->memory.malloc(strlen(content_type) + 1);
      if (this->content_type) {
         strcpy(this->content_type, content_type);
         result = 0;
      }
   }
   return result;
}

static int set_header(response_impl *this, const char *field, const char *value) {
   int result = -1;
   if (field != NULL && value != NULL
       && !!strcasecmp(field, "Status") && !!strcasecmp(field, "Content-Type") && !!strcasecmp(field, "Location")
       && !!strcasecmp(field, "Cookie") && !!strcasecmp(field, "Set-Cookie")) {
      char *header = this->memory.malloc(strlen(value) + 1);
      if (header) {
         strcpy(header, value);
         char *old_header = this->headers->set(this->headers, field, header);
         this->memory.free(old_header);
         result = 0;
      }
   }
   return result;
}

static cad_cgi_response_t response_fn = {
   (cad_cgi_response_cookies_fn) cookies,
   (cad_cgi_response_fd_fn) response_fd,
   (cad_cgi_response_body_fn) body,
   (cad_cgi_response_redirect_fn) redirect,
   (cad_cgi_response_set_status_fn) set_status,
   (cad_cgi_response_set_content_type_fn) set_content_type,
   (cad_cgi_response_set_header_fn) set_header,
};

static response_impl *new_cad_cgi_response(cad_memory_t memory) {
   response_impl *result = memory.malloc(sizeof(response_impl));
   if (!result) return NULL;
   result->fn = response_fn;
   result->memory = memory;
   result->cookies = new_cookies(memory);
   result->body = NULL;
   result->body_stream = new_cad_output_stream_from_string(&(result->body), memory);
   result->redirect_path = NULL;
   result->redirect_fragment = NULL;
   result->status = 0;
   result->content_type = NULL;
   result->headers = cad_new_hash(memory, cad_hash_strings);
   return result;
}

static void flush_body(const char *body) {
   int status = 0;
   while (*body) {
      switch(status) {
      case 0:
         switch(*body) {
         case '\r':
            putchar('\r');
            status = 1;
            break;
         case '\n':
            putchar('\r');
            putchar('\n');
            break;
         default:
            putchar(*body);
            break;
         }
         break;
      case 1:
         switch(*body) {
         case '\r':
            putchar('\n');
            putchar('\r');
            break;
         case '\n':
            putchar('\n');
            status = 0;
            break;
         default:
            putchar('\n');
            putchar(*body);
            status = 0;
            break;
         }
         break;
      }
      body++;
   }
}

static void flush_header(void *hash, int index, const char *key, const char *value) {
   printf("%s: %s\r\n", key, value == NULL ? "" : value);
}

static void flush_response(response_impl *response) {
   printf("Content-Type: %s\r\n", response->content_type == NULL ? "text/plain" : response->content_type);
   printf("Status: %d\r\n", response->status == 0 ? 200 : response->status);
   response->headers->iterate(response->headers, (cad_hash_iterator_fn)flush_header, response);
   putchar('\r');
   putchar('\n');
   flush_body(response->body);
}

/* ---------------------------------------------------------------- */

typedef struct {
   cad_cgi_t fn;
   cad_memory_t memory;
   cad_cgi_handle_cb handler;
} cgi_impl;

static void free_cgi(cgi_impl *this) {
   this->memory.free(this);
}

static int run(cgi_impl *this) {
   int result = -1;
   response_impl *response = new_cad_cgi_response(this->memory);
   if (response) {
      const char *verb = getenv("REQUEST_METHOD");
      if (verb) {
         if ((this->handler)((cad_cgi_t*)this, verb, (cad_cgi_response_t*)response) == 0) {
            result = 0;
            flush_response(response);
         } else {
            printf("Status: 500\r\nContent-Type: text/plain\r\n\r\nInternal error: no verb\r\n");
         }
      }
      free_response(response);
   } else {
      printf("Status: 500\r\nContent-Type: text/plain\r\n\r\nInternal error: NULL response\r\n");
   }
   return result;
}

static int cgi_fd(cgi_impl *this) {
   return STDIN_FILENO;
}

static cad_cgi_t cgi_fn = {
   (cad_cgi_free_fn) free_cgi,
   (cad_cgi_run_fn) run,
   (cad_cgi_fd_fn) cgi_fd,
};

cad_cgi_t *new_cad_cgi(cad_memory_t memory, cad_cgi_handle_cb handler) {
   cgi_impl *result = memory.malloc(sizeof(cgi_impl));
   if (!result) return NULL;
   result->fn = cgi_fn;
   result->memory = memory;
   result->handler = handler;
   return (cad_cgi_t*)result;
}
