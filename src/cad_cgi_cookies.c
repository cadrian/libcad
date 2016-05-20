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
 * This file contains the implementation of the CGI cookies.
 */

#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "cad_cgi_internal.h"

/* ---------------------------------------------------------------- */

typedef struct {
   cad_cgi_cookie_t fn;
   cad_memory_t memory;
   int changed;
   char *name;
   char *value;
   time_t expires;
   int max_age;
   cad_cgi_cookie_flag_e flag;
   char *domain;
   char *path;
} cookie_impl;

static void free_cookie(cookie_impl *this) {
   this->memory.free(this->domain);
   this->memory.free(this->path);
   this->memory.free(this);
}

static const char *name(cookie_impl *this) {
   return this->name;
}

static const char *value(cookie_impl *this) {
   return this->value;
}

static int set_value(cookie_impl *this, const char *value) {
   int result = -1;
   this->memory.free(this->value);
   this->value = this->memory.malloc(strlen(value) + 1);
   if (this->value) {
      strcpy(this->value, value);
      this->changed = 1;
      result = 0;
   }
   return result;
}

static time_t expires(cookie_impl *this) {
   return this->expires;
}

static int set_expires(cookie_impl *this, time_t expires) {
   int result = -1;
   if (this->max_age == 0) {
      this->expires = expires;
      this->changed = 1;
      result = 0;
   }
   return result;
}

static int max_age(cookie_impl *this) {
   return this->max_age;
}

static int set_max_age(cookie_impl *this, int max_age) {
   int result = -1;
   if (this->expires == 0) {
      this->max_age = max_age;
      this->changed = 1;
      result = 0;
   }
   return result;
}

static const char *domain(cookie_impl *this) {
   return this->domain;
}

static int set_domain(cookie_impl *this, const char *domain) {
   int result = -1;
   this->memory.free(this->domain);
   this->domain = this->memory.malloc(strlen(domain) + 1);
   if (this->domain) {
      strcpy(this->domain, domain);
      this->changed = 1;
      result = 0;
   }
   return result;
}

static const char *path(cookie_impl *this) {
   return this->path;
}

static int set_path(cookie_impl *this, const char *path) {
   int result = -1;
   this->memory.free(this->path);
   this->path = this->memory.malloc(strlen(path) + 1);
   if (this->path) {
      strcpy(this->path, path);
      this->changed = 1;
      result = 0;
   }
   return result;
}

static cad_cgi_cookie_flag_e flag(cookie_impl *this) {
   return this->flag;
}

static int set_flag(cookie_impl *this, cad_cgi_cookie_flag_e flag) {
   this->flag = flag;
   this->changed = 1;
   return 0;
}

cad_cgi_cookie_t cookie_fn = {
   (cad_cgi_cookie_free_fn) free_cookie,
   (cad_cgi_cookie_name_fn) name,
   (cad_cgi_cookie_value_fn) value,
   (cad_cgi_cookie_set_value_fn) set_value,
   (cad_cgi_cookie_expires_fn) expires,
   (cad_cgi_cookie_set_expires_fn) set_expires,
   (cad_cgi_cookie_max_age_fn) max_age,
   (cad_cgi_cookie_set_max_age_fn) set_max_age,
   (cad_cgi_cookie_domain_fn) domain,
   (cad_cgi_cookie_set_domain_fn) set_domain,
   (cad_cgi_cookie_path_fn) path,
   (cad_cgi_cookie_set_path_fn) set_path,
   (cad_cgi_cookie_flag_fn) flag,
   (cad_cgi_cookie_set_flag_fn) set_flag,
};

cad_cgi_cookie_t *new_cad_cgi_cookie(cad_memory_t memory, const char *name) {
   cookie_impl *result = memory.malloc(sizeof(cookie_impl) + strlen(name) + 1);
   if (!result) return NULL;
   result->fn = cookie_fn;
   result->memory = memory;
   result->changed = 0;
   result->name = (char*)(result + 1);
   strcpy(result->name, name);
   result->expires = 0;
   result->max_age = 0;
   result->flag = Cookie_default;
   result->domain = NULL;
   result->path = NULL;
   return (cad_cgi_cookie_t*)result;
}

/* ---------------------------------------------------------------- */

typedef struct {
   cad_cgi_cookies_t fn;
   cad_memory_t memory;
   cad_hash_t *jar;
} cookies_impl;

static void clean_cookie(void *hash, int index, const char *key, cad_cgi_cookie_t *cookie, cookies_impl *cookies) {
   cookie->free(cookie);
}

void free_cookies(cad_cgi_cookies_t *this_) {
   cookies_impl *this = (cookies_impl*)this_;
   this->jar->clean(this->jar, (cad_hash_iterator_fn)clean_cookie, this);
   this->jar->free(this->jar);
   this->memory.free(this);
}

static cad_cgi_cookie_t *get(cookies_impl *this, const char *name) {
   return this->jar->get(this->jar, name);
}

static int set(cookies_impl *this, cad_cgi_cookie_t *cookie) {
   cad_cgi_cookie_t *old = this->jar->set(this->jar, cookie->name(cookie), cookie);
   if (old) {
      old->free(old);
   }
   return 0;
}

static cad_cgi_cookies_t cookies_fn = {
   (cad_cgi_cookies_get_fn) get,
   (cad_cgi_cookies_set_fn) set,
};

static char *parse_cookie_name(cad_memory_t memory, const char *start, const char *end) {
   int n = end - start;
   char *result = memory.malloc(n + 1);
   strncpy(result, start, n);
   result[n] = 0;
   return result;
}

static char *parse_cookie_value(cad_memory_t memory, const char *start, const char *end) {
   int n = 0, d, s = 0;
   char *result;
   const char *c = start;
   while (c != end) {
      switch (*c) {
      case '%':
         c += 2;
         break;
      }
      n++;
      c++;
   }
   result = memory.malloc(n + 1);
   c = start;
   while (c != end) {
      switch(s) {
      case 0:
         switch (*c) {
         case '%':
            d = 0;
            s = 1;
            break;
         default:
            *result++ = *c;
         }
         break;
      case 1:
         switch (*c) {
         case '0': case '1': case '2': case '3': case '4':
         case '5': case '6': case '7': case '8': case '9':
            d = *c - '0';
            break;
         case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            d = *c + 10 - 'A';
            break;
         case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            d = *c + 10 - 'a';
            break;
         }
         s = 2;
         break;
      case 2:
         switch (*c) {
         case '0': case '1': case '2': case '3': case '4':
         case '5': case '6': case '7': case '8': case '9':
            d += *c - '0';
            break;
         case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            d += *c + 10 - 'A';
            break;
         case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
            d += *c + 10 - 'a';
            break;
         }
         *result++ = (char)d;
         s = 0;
         break;
      }
      c++;
   }
   return result;
}

static void parse_cookies(cad_hash_t *jar, const char *http_cookie, cad_memory_t memory) {
   const char *name = http_cookie, *value = NULL;
   char *n, *v;
   int state = 0;
   cad_cgi_cookie_t *cookie;
   while (*http_cookie) {
      switch (state) {
      case 0: // reading cookie name
         switch (*http_cookie) {
         case '=':
            state = 1;
            value = http_cookie + 1;
            break;
         case ';':
            n = parse_cookie_name(memory, name, http_cookie);
            cookie = new_cad_cgi_cookie(memory, n);
            jar->set(jar, cookie->name(cookie), cookie);
            memory.free(n);
            state = 2;
            break;
         }
         break;
      case 1: // reading cookie value
         switch (*http_cookie) {
         case ';':
            n = parse_cookie_name(memory, name, value-1);
            v = parse_cookie_value(memory, value, http_cookie);
            cookie = new_cad_cgi_cookie(memory, n);
            cookie->set_value(cookie, v);
            jar->set(jar, cookie->name(cookie), cookie);
            memory.free(v);
            memory.free(n);
            state = 2;
            break;
         }
         break;
      case 2: // at the end of a cookie, waiting for the next
         switch (*http_cookie) {
         case ' ': // ignore blank
            break;
         default:
            name = http_cookie;
            value = NULL;
            state = 0;
            break;
         }
      }
      http_cookie++;
   }
}

cad_cgi_cookies_t *new_cookies(cad_memory_t memory) {
   cookies_impl *result = memory.malloc(sizeof(cookies_impl));
   if (!result) return NULL;
   result->fn = cookies_fn;
   result->memory = memory;
   result->jar = cad_new_hash(memory, cad_hash_strings);

   const char *http_cookie = getenv("HTTP_COOKIE");
   if (http_cookie != NULL) {
      parse_cookies(result->jar, http_cookie, memory);
   }

   return (cad_cgi_cookies_t*)result;
}

static const char *DAY_NAMES[] =
  { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char *MONTH_NAMES[] =
  { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

static char *rfc1123(time_t t, char *buf, size_t buflen)
{
    struct tm tm;
    gmtime_r(&t, &tm);
    strftime(buf, buflen, "---, %d --- %Y %H:%M:%S GMT", &tm);
    memcpy(buf, DAY_NAMES[tm.tm_wday], 3);
    memcpy(buf+8, MONTH_NAMES[tm.tm_mon], 3);
    return buf;
}

static char *encode_value(const char *value, cad_memory_t memory) {
   const char *c = value;
   char *v;
   int count = 1;
   while (*c) {
      switch (*c) {
      case '%': case '=': case ';':
         count += 3;
         break;
      default:
         count++;
      }
      c++;
   }
   char *result = memory.malloc(strlen(value) + 1);
   c = value;
   v = result;
   while (*c) {
      switch (*c) {
      case '%':
         *v++ = '%';
         *v++ = '2';
         *v++ = '5';
         break;
      case '=':
         *v++ = '%';
         *v++ = '3';
         *v++ = 'd';
         break;
      case ';':
         *v++ = '%';
         *v++ = '3';
         *v++ = 'b';
         break;
      default:
         *v++ = *c;
      }
      c++;
   }
   *v = 0;

   return result;
}

static void flush_cookie(void *hash, int index, const char *key, cookie_impl *cookie) {
   if (cookie->changed) {
      char *encoded_value = encode_value(cookie->value, cookie->memory);
      printf("Set-Cookie: %s=%s", cookie->name, cookie->value == NULL ? "" : encoded_value);
      cookie->memory.free(encoded_value);

      if (cookie->expires > 0) {
         char buf[30];
         rfc1123(cookie->expires, buf, 30);
         printf("; Expires=%s", buf);
      }
      if (cookie->max_age > 0) {
         printf("; Max-Age=%d", cookie->max_age);
      }
      if (cookie->domain != NULL) {
         printf("; Domain=%s", cookie->domain);
      }
      if (cookie->path != NULL) {
         printf("; Path=%s", cookie->path);
      }
      if (cookie->flag & Cookie_secure) {
         printf("; Secure");
      }
      if (cookie->flag & Cookie_http_only) {
         printf("; HttpOnly");
      }
      putchar('\r');
      putchar('\n');
   }
   cookie->changed = 0;
}

void flush_cookies(cad_cgi_cookies_t *cookies) {
   cookies_impl *this = (cookies_impl*)cookies;
   this->jar->iterate(this->jar, (cad_hash_iterator_fn)flush_cookie, this);
}
