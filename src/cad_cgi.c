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

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "cad_cgi_internal.h"

/* ---------------------------------------------------------------- */

typedef struct {
   cad_cgi_meta_t fn;
   cad_memory_t memory;
   int auth_type;
   size_t content_length;
   cad_cgi_content_type_t content_type;
   cad_cgi_gateway_interface_t gateway_interface;
   char *path_info;
   char *path_translated;
   cad_hash_t *query_string;
   cad_hash_t *input_as_form;
   char *remote_addr;
   char *remote_host;
   char *remote_ident;
   char *remote_user;
   char *request_method;
   char *script_name;
   char *server_name;
   int server_port;
   cad_cgi_server_protocol_t server_protocol;
   char *server_software;
} meta_impl;

static cad_cgi_auth_type_e auth_type(meta_impl *this) {
   int result = this->auth_type;
   if (result == -2) {
      const char *auth_type = getenv("AUTH_TYPE");
      if (auth_type == NULL || !*auth_type) {
         result = Auth_none;
      } else if (!strcmp(auth_type, "Basic")) {
         result = Auth_basic;
      } else if (!strcmp(auth_type, "Digest")) {
         result = Auth_digest;
      } else {
         result = Auth_invalid;
      }
   }
   return (cad_cgi_auth_type_e)result;
}

static size_t content_length(meta_impl *this) {
   return this->content_length;
}

static cad_cgi_content_type_t *content_type(meta_impl *this) {
   cad_cgi_content_type_t *result = &(this->content_type);
   if (result->type == NULL) {
      const char *content_type = getenv("CONTENT_TYPE");
      if (content_type == NULL || !*content_type) {
         result->type = this->memory.malloc(1);
         ((char*)(result->type))[0] = 0;
         result->subtype = this->memory.malloc(1);
         ((char*)(result->subtype))[0] = 0;
      } else {
         const char *c = content_type;
         int s = 0;
         cad_output_stream_t *out = new_cad_output_stream_from_string((char**)&(result->type), this->memory);
         char *attribute = NULL;
         char *value = NULL;
         while (*c) {
            switch(s) {
            case 0: // reading type
               if (*c == '/') {
                  out->free(out);
                  out = new_cad_output_stream_from_string((char**)&(result->subtype), this->memory);
                  s = 1;
               } else {
                  out->put(out, "%c", *c);
               }
               break;
            case 1: // reading subtype
               if (*c == ';') {
                  out->free(out);
                  out = new_cad_output_stream_from_string(&attribute, this->memory);
                  s = 2;
               } else {
                  out->put(out, "%c", *c);
               }
               break;
            case 2: // reading attribute
               if (*c == '=') {
                  out->free(out);
                  out = new_cad_output_stream_from_string(&value, this->memory);
                  s = 3;
               } else {
                  out->put(out, "%c", *c);
               }
               break;
            case 3: // reading value
               if (*c == ';') {
                  result->parameters->set(result->parameters, attribute, value);
                  out->free(out);
                  this->memory.free(attribute);
                  attribute = value = NULL;
                  out = new_cad_output_stream_from_string(&attribute, this->memory);
                  s = 2;
               } else if (*c == '"') {
                  if (attribute == NULL) {
                     s = 4;
                  }
               } else {
                  out->put(out, "%c", *c);
               }
            case 4: // reading value in quoted string
               if (*c == '"') {
                  s = 5;
               } else {
                  out->put(out, "%c", *c);
               }
               break;
            case 5: // reading value after quoted string
               if (*c == ';') {
                  result->parameters->set(result->parameters, attribute, value);
                  out->free(out);
                  this->memory.free(attribute);
                  attribute = value = NULL;
                  out = new_cad_output_stream_from_string(&attribute, this->memory);
                  s = 2;
               }
               break;
            }
            c++;
         }
         switch(s) {
         case 3: case 5:
            result->parameters->set(result->parameters, attribute, value);
            break;
         case 2:
            this->memory.free(attribute);
            break;
         case 4:
            this->memory.free(attribute);
            this->memory.free(value);
            break;
         }
         out->free(out);
      }
   }
   return result;
}

static cad_cgi_gateway_interface_t *gateway_interface(meta_impl *this) {
   cad_cgi_gateway_interface_t *result = &(this->gateway_interface);
   if (result->major == 0) {
      const char *gateway_interface = getenv("GATEWAY_INTERFACE");
      if (gateway_interface != NULL) {
         const char *c = gateway_interface;
         int s = 0;
         while (*c && s >= 0) {
            switch(s) {
            case 0: // C
               if (*c == 'C') {
                  s = 1;
               } else {
                  s = -1;
               }
               break;
            case 1: // G
               if (*c == 'G') {
                  s = 2;
               } else {
                  s = -1;
               }
               break;
            case 2: // I
               if (*c == 'I') {
                  s = 3;
               } else {
                  s = -1;
               }
               break;
            case 3: // /
               if (*c == '/') {
                  s = 4;
               } else {
                  s = -1;
               }
               break;
            case 4: // major
               if (*c == '.') {
                  s = 5;
               } else {
                  result->major = result->major * 10 + *c - '0';
                  if (result->major < 0) {
                     s = -1;
                  }
               }
               break;
            case 5: // minor
               result->minor = result->minor * 10 + *c - '0';
               if (result->minor < 0) {
                  s = -1;
               }
               break;
            default:
               s = -1;
            }
            c++;
         }
      }
   }
   return result;
}

static const char *path_info(meta_impl *this) {
   char *result = this->path_info;
   if (result == NULL) {
      char *path_info = getenv("PATH_INFO");
      if (path_info == NULL) {
         result = this->memory.malloc(1);
         *result = 0;
      } else {
         result = this->memory.malloc(strlen(path_info) + 1);
         strcpy(result, path_info);
      }
      this->path_info = result;
   }
   return result;
}

static const char *path_translated(meta_impl *this) {
   char *result = this->path_translated;
   if (result == NULL) {
      char *path_translated = getenv("PATH_TRANSLATED");
      if (path_translated == NULL) {
         result = this->memory.malloc(1);
         *result = 0;
      } else {
         result = this->memory.malloc(strlen(path_translated) + 1);
         strcpy(result, path_translated);
      }
      this->path_translated = result;
   }
   return result;
}

static cad_hash_t *parse_query_or_form(meta_impl *this, cad_input_stream_t *in) {
   cad_hash_t *result = cad_new_hash(this->memory, cad_hash_strings);
   int s = 0;
   char *attribute = NULL;
   char *value = NULL;
   cad_output_stream_t *out = new_cad_output_stream_from_string(&attribute, this->memory);
   char encoded = 0;
   int c = in->item(in);
   while (c != -1 && s >= 0) {
      switch(s) {
      case 0: // reading attribute
         switch(c) {
         case '=':
            out->free(out);
            out = new_cad_output_stream_from_string(&value, this->memory);
            s = 10;
            break;
         case '%':
            encoded = 0;
            s = 1;
            break;
         case '+':
            out->put(out, " ");
            break;
         default:
            out->put(out, "%c", c);
            break;
         }
         break;
      case 10: // reading value
         switch(c) {
         case '&':
         case '\n':
            result->set(result, attribute, value);
            this->memory.free(attribute);
            attribute = value = NULL;
            out->free(out);
            out = new_cad_output_stream_from_string(&attribute, this->memory);
            s = 0;
            break;
         case '%':
            encoded = 0;
            s = 11;
            break;
         case '+':
            out->put(out, " ");
            break;
         default:
            out->put(out, "%c", c);
            break;
         }
         break;
      case 1: case 11: // reading first hex
         switch(c) {
         case '0': case '1': case '2': case '3': case '4':
         case '5': case '6': case '7': case '8': case '9':
            encoded = c - '0';
            s++;
            break;
         case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            encoded = c + 10 - 'A';
            s++;
            break;
         case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            encoded = c + 10 - 'a';
            s++;
            break;
         default:
            s = -1;
         }
         break;
      case 2: case 12: // reading second hex
         switch(c) {
         case '0': case '1': case '2': case '3': case '4':
         case '5': case '6': case '7': case '8': case '9':
            encoded *= 0x10;
            encoded += c - '0';
            out->put(out, "%c", encoded);
            s -= 2;
            break;
         case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            encoded *= 0x10;
            encoded += c + 10 - 'A';
            out->put(out, "%c", encoded);
            s -= 2;
            break;
         case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            encoded *= 0x10;
            encoded += c + 10 - 'a';
            out->put(out, "%c", encoded);
            s -= 2;
            break;
         default:
            s = -1;
         }
         break;
      }
      if (in->next(in)) {
         s = -1;
      } else {
         c = in->item(in);
      }
   }
   if (s == 10) {
      result->set(result, attribute, value);
      this->memory.free(attribute);
   } else {
      this->memory.free(attribute);
      this->memory.free(value);
   }
   out->free(out);
   return result;
}

static cad_hash_t *query_string(meta_impl *this) {
   cad_hash_t *result = this->query_string;
   if (result == NULL) {
      const char *query_string = getenv("QUERY_STRING");
      if (query_string != NULL) {
         cad_input_stream_t *in = new_cad_input_stream_from_string(query_string, this->memory);
         if (in != NULL) {
            result = this->query_string = parse_query_or_form(this, in);
            in->free(in);
         }
      }
   }
   return result;
}

static cad_hash_t *input_as_form(meta_impl *this) {
   cad_hash_t *result = this->input_as_form;
   if (result == NULL) {
      cad_input_stream_t *in = new_cad_input_stream_from_file_descriptor(STDIN_FILENO, this->memory);
      if (in != NULL) {
         result = this->input_as_form = parse_query_or_form(this, in);
         in->free(in);
      }
   }
   return result;
}

static const char *remote_addr(meta_impl *this) {
   char *result = this->remote_addr;
   if (result == NULL) {
      char *remote_addr = getenv("REMOTE_ADDR");
      if (remote_addr == NULL) {
         result = this->memory.malloc(1);
         *result = 0;
      } else {
         result = this->memory.malloc(strlen(remote_addr) + 1);
         strcpy(result, remote_addr);
      }
      this->remote_addr = result;
   }
   return result;
}

static const char *remote_host(meta_impl *this) {
   char *result = this->remote_host;
   if (result == NULL) {
      char *remote_host = getenv("REMOTE_HOST");
      if (remote_host == NULL) {
         result = this->memory.malloc(1);
         *result = 0;
      } else {
         result = this->memory.malloc(strlen(remote_host) + 1);
         strcpy(result, remote_host);
      }
      this->remote_host = result;
   }
   return result;
}

static const char *remote_ident(meta_impl *this) {
   char *result = this->remote_ident;
   if (result == NULL) {
      char *remote_ident = getenv("REMOTE_IDENT");
      if (remote_ident == NULL) {
         result = this->memory.malloc(1);
         *result = 0;
      } else {
         result = this->memory.malloc(strlen(remote_ident) + 1);
         strcpy(result, remote_ident);
      }
      this->remote_ident = result;
   }
   return result;
}

static const char *remote_user(meta_impl *this) {
   char *result = this->remote_user;
   if (result == NULL) {
      char *remote_user = getenv("REMOTE_USER");
      if (remote_user == NULL) {
         result = this->memory.malloc(1);
         *result = 0;
      } else {
         result = this->memory.malloc(strlen(remote_user) + 1);
         strcpy(result, remote_user);
      }
      this->remote_user = result;
   }
   return result;
}

static const char *request_method(meta_impl *this) {
   char *result = this->request_method;
   if (result == NULL) {
      char *request_method = getenv("REQUEST_METHOD");
      if (request_method == NULL) {
         result = this->memory.malloc(1);
         *result = 0;
      } else {
         result = this->memory.malloc(strlen(request_method) + 1);
         strcpy(result, request_method);
      }
      this->request_method = result;
   }
   return result;
}

static const char *script_name(meta_impl *this) {
   char *result = this->script_name;
   if (result == NULL) {
      char *script_name = getenv("SCRIPT_NAME");
      if (script_name == NULL) {
         result = this->memory.malloc(1);
         *result = 0;
      } else {
         result = this->memory.malloc(strlen(script_name) + 1);
         strcpy(result, script_name);
      }
      this->script_name = result;
   }
   return result;
}

static const char *server_name(meta_impl *this) {
   char *result = this->server_name;
   if (result == NULL) {
      char *server_name = getenv("SERVER_NAME");
      if (server_name == NULL) {
         result = this->memory.malloc(1);
         *result = 0;
      } else {
         result = this->memory.malloc(strlen(server_name) + 1);
         strcpy(result, server_name);
      }
      this->server_name = result;
   }
   return result;
}

static int server_port(meta_impl *this) {
   int result = this->server_port;
   if (result == -1) {
      const char *server_port = getenv("SERVER_PORT");
      if (server_port != NULL) {
         long int sp = strtol(server_port, NULL, 10);
         if (sp > INT_MAX) {
            result = this->server_port = INT_MAX;
         } else if (sp > 0) {
            result = this->server_port = (int)sp;
         }
      }
   }
   return result;
}

static cad_cgi_server_protocol_t *server_protocol(meta_impl *this) {
   cad_cgi_server_protocol_t *result = &(this->server_protocol);
   if (result->protocol == NULL) {
      const char *server_protocol = getenv("SERVER_PROTOCOL");
      if (server_protocol == NULL) {
         result->protocol = this->memory.malloc(1);
         ((char*)(result->protocol))[0] = 0;
      } else {
         const char *c = strchr(server_protocol, '/');
         if (c != NULL) {
            *(char*)c = 0;
            c++;
            int s = 0;
            result->major = result->minor = 0;
            while (*c && s >= 0) {
               switch(s) {
               case 0: // major
                  if (*c == '.') {
                     s = 1;
                  } else {
                     result->major = result->major * 10 + *c - '0';
                     if (result->major < 0) {
                        s = -1;
                     }
                  }
                  break;
               case 1: // minor
                  result->minor = result->minor * 10 + *c - '0';
                  if (result->minor < 0) {
                     s = -1;
                  }
                  break;
               }
               c++;
            }
         }
         result->protocol = this->memory.malloc(strlen(server_protocol) + 1);
         strcpy((char*)(result->protocol), server_protocol);
      }
   }
   return result;
}

static const char *server_software(meta_impl *this) {
   char *result = this->server_software;
   if (result == NULL) {
      char *server_software = getenv("SERVER_SOFTWARE");
      if (server_software == NULL) {
         result = this->memory.malloc(1);
         *result = 0;
      } else {
         result = this->memory.malloc(strlen(server_software) + 1);
         strcpy(result, server_software);
      }
      this->server_software = result;
   }
   return result;
}

static cad_cgi_meta_t meta_fn = {
   (cad_cgi_meta_auth_type_fn) auth_type,
   (cad_cgi_meta_content_length_fn) content_length,
   (cad_cgi_meta_content_type_fn) content_type,
   (cad_cgi_meta_gateway_interface_fn) gateway_interface,
   (cad_cgi_meta_path_info_fn) path_info,
   (cad_cgi_meta_path_translated_fn) path_translated,
   (cad_cgi_meta_query_string_fn) query_string,
   (cad_cgi_meta_input_as_form_fn) input_as_form,
   (cad_cgi_meta_remote_addr_fn) remote_addr,
   (cad_cgi_meta_remote_host_fn) remote_host,
   (cad_cgi_meta_remote_ident_fn) remote_ident,
   (cad_cgi_meta_remote_user_fn) remote_user,
   (cad_cgi_meta_request_method_fn) request_method,
   (cad_cgi_meta_script_name_fn) script_name,
   (cad_cgi_meta_server_name_fn) server_name,
   (cad_cgi_meta_server_port_fn) server_port,
   (cad_cgi_meta_server_protocol_fn) server_protocol,
   (cad_cgi_meta_server_software_fn) server_software,
};

static void clean_string(void *hash, int index, const char *key, char *value, meta_impl *this) {
   this->memory.free(value);
}

static void free_meta(meta_impl *this) {
   this->memory.free((char*)this->content_type.type);
   this->memory.free((char*)this->content_type.subtype);
   this->content_type.parameters->clean(this->content_type.parameters, (cad_hash_iterator_fn)clean_string, this);
   this->content_type.parameters->free(this->content_type.parameters);
   this->memory.free(this->path_info);
   this->memory.free(this->path_translated);
   if (this->query_string != NULL) {
      this->query_string->clean(this->query_string, (cad_hash_iterator_fn)clean_string, this);
      this->query_string->free(this->query_string);
   }
   if (this->input_as_form != NULL) {
      this->input_as_form->clean(this->input_as_form, (cad_hash_iterator_fn)clean_string, this);
      this->input_as_form->free(this->input_as_form);
   }
   this->memory.free(this->remote_addr);
   this->memory.free(this->remote_host);
   this->memory.free(this->remote_ident);
   this->memory.free(this->remote_user);
   this->memory.free(this->request_method);
   this->memory.free(this->script_name);
   this->memory.free((char*)this->server_protocol.protocol);
   this->memory.free(this->server_software);
}

static meta_impl *new_meta(cad_memory_t memory) {
   meta_impl *result = memory.malloc(sizeof(meta_impl));
   if (!result) return NULL;

   const char *szcl = getenv("CONTENT_LENGTH");
   int content_length = 0;
   if (szcl != NULL) {
      long int cl = strtol(szcl, NULL, 10);
      if (cl > INT_MAX) {
         content_length = INT_MAX;
      } else if (cl > 0) {
         content_length = (int)cl;
      }
   }

   result->fn = meta_fn;
   result->memory = memory;
   result->auth_type = -2;
   result->content_length = content_length;
   result->content_type.type = NULL;
   result->content_type.subtype = NULL;
   result->content_type.parameters = cad_new_hash(memory, cad_hash_strings);
   result->gateway_interface.major = 0;
   result->gateway_interface.minor = 0;
   result->path_info = NULL;
   result->path_translated = NULL;
   result->query_string = NULL;
   result->remote_addr = NULL;
   result->remote_host = NULL;
   result->remote_ident = NULL;
   result->remote_user = NULL;
   result->request_method = NULL;
   result->script_name = NULL;
   result->server_port = -1;
   result->server_protocol.major = 0;
   result->server_protocol.minor = 0;
   result->server_protocol.protocol = NULL;
   result->server_software = NULL;

   return result;
}

/* ---------------------------------------------------------------- */

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
   meta_impl *meta;
} response_impl;

static void clean_header(void *hash, int index, const char *key, char *value, response_impl *response) {
   response->memory.free(value);
}

static void free_response(response_impl *this) {
   free_cookies(this->cookies);
   free_meta(this->meta);
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

static cad_cgi_meta_t *meta_variables(response_impl *this) {
   return (cad_cgi_meta_t*)this->meta;
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
   flush_cookies(response->cookies);
   putchar('\r');
   putchar('\n');
   flush_body(response->body);
}

static cad_cgi_response_t response_fn = {
   (cad_cgi_response_free_fn) free_response,
   (cad_cgi_response_cookies_fn) cookies,
   (cad_cgi_response_meta_variables_fn) meta_variables,
   (cad_cgi_response_flush_fn) flush_response,
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
   result->meta = new_meta(memory);
   return result;
}

/* ---------------------------------------------------------------- */

typedef struct {
   cad_cgi_t fn;
   cad_memory_t memory;
   cad_cgi_handle_cb handler;
   void *data;
} cgi_impl;

static void free_cgi(cgi_impl *this) {
   this->memory.free(this);
}

static response_impl *run(cgi_impl *this) {
   response_impl *result = new_cad_cgi_response(this->memory);
   if (result) {
      int status = (this->handler)((cad_cgi_t*)this, (cad_cgi_response_t*)result, this->data);
      if (status != 0) {
         printf("Status: 500\r\nContent-Type: text/plain\r\n\r\nInternal error: handler failed with status %d\r\n", status);
         free_response(result);
         result = NULL;
      }
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

cad_cgi_t *new_cad_cgi(cad_memory_t memory, cad_cgi_handle_cb handler, void *data) {
   cgi_impl *result = memory.malloc(sizeof(cgi_impl));
   if (!result) return NULL;
   result->fn = cgi_fn;
   result->memory = memory;
   result->handler = handler;
   result->data = data;
   return (cad_cgi_t*)result;
}
