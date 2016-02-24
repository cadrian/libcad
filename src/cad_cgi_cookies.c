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
#include "cad_hash.h"

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

cad_cgi_cookies_t *new_cookies(cad_memory_t memory) {
   cookies_impl *result = memory.malloc(sizeof(cookies_impl));
   if (!result) return NULL;
   result->fn = cookies_fn;
   result->memory = memory;
   result->jar = cad_new_hash(memory, cad_hash_strings);
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


static void flush_cookie(void *hash, int index, const char *key, cookie_impl *cookie) {
   if (cookie->changed) {
      printf("Set-Cookie: %s=%s", cookie->name, cookie->value == NULL ? "" : cookie->value);
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
