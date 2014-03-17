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

#include <stdlib.h>
#include <sys/select.h>

/**
 * @ingroup cad_events
 * @file
 *
 * This file contains the implementations of event loops using select(2).
 */

#include "cad_events.h"

typedef struct {
   cad_events_t fn;
   fd_set fd_read, fd_write, fd_exception;
   struct timespec timeout;
   void (*on_timeout)(void *);
   void (*on_read)(int, void *);
   void (*on_write)(int, void *);
   void (*on_exception)(int, void *);
   int max_fd;
} events_selector_t;

static void set_timeout_selector(events_selector_t *this, unsigned long timeout_us) {
   this->timeout.tv_sec = (long)(timeout_us / 1000000UL);
   this->timeout.tv_nsec = (long)(timeout_us % 1000000UL) * 1000L;
}

static void set_read_selector(events_selector_t *this, int fd) {
   FD_SET(fd, &(this->fd_read));
   if (this->max_fd < fd) {
      this->max_fd = fd;
   }
}

static void set_write_selector(events_selector_t *this, int fd) {
   FD_SET(fd, &(this->fd_write));
   if (this->max_fd < fd) {
      this->max_fd = fd;
   }
}

static void set_exception_selector(events_selector_t *this, int fd) {
   FD_SET(fd, &(this->fd_exception));
   if (this->max_fd < fd) {
      this->max_fd = fd;
   }
}

static void on_timeout_selector(events_selector_t *this, void (*action)(void *)) {
   this->on_timeout = action;
}

static void on_read_selector(events_selector_t *this, void (*action)(int, void *)) {
   this->on_read = action;
}

static void on_write_selector(events_selector_t *this, void (*action)(int, void *)) {
   this->on_write = action;
}

static void on_exception_selector(events_selector_t *this, void (*action)(int, void *)) {
   this->on_exception = action;
}

static int wait_selector(events_selector_t *this, void *data) {
   int res;
   int i;
   fd_set r = this->fd_read, w = this->fd_write, x = this->fd_exception;
   struct timespec t = this->timeout;
   res = pselect(this->max_fd + 1, &r, &w, &x, &t, NULL);
   if (res == 0) {
      if (this->on_timeout != NULL) {
         this->on_timeout(data);
      }
   } else if (res > 0) {
      for (i = 0; i <= this->max_fd; i++) {
         if (this->on_read != NULL && FD_ISSET(i, &(this->fd_read)) && FD_ISSET(i, &r)) {
            this->on_read(i, data);
         }
         if (this->on_write != NULL && FD_ISSET(i, &(this->fd_write)) && FD_ISSET(i, &w)) {
            this->on_write(i, data);
         }
         if (this->on_exception != NULL && FD_ISSET(i, &(this->fd_exception)) && FD_ISSET(i, &x)) {
            this->on_exception(i, data);
         }
      }
   } // else res < 0 => error (returned)
   return res;
}

static void free_selector(events_selector_t *this) {
   free(this);
}

cad_events_t fn_selector = {
   .set_timeout   = (cad_events_set_timeout_fn)set_timeout_selector,
   .set_read      = (cad_events_set_read_fn)set_read_selector,
   .set_write     = (cad_events_set_write_fn)set_write_selector,
   .set_exception = (cad_events_set_exception_fn)set_exception_selector,
   .on_timeout    = (cad_events_on_timeout_fn)on_timeout_selector,
   .on_read       = (cad_events_on_read_fn)on_read_selector,
   .on_write      = (cad_events_on_write_fn)on_write_selector,
   .on_exception  = (cad_events_on_exception_fn)on_exception_selector,
   .wait          = (cad_events_wait_fn)wait_selector,
   .free          = (cad_events_free_fn)free_selector,
};

__PUBLIC__ cad_events_t *cad_new_events_selector(void) {
   events_selector_t *result = (events_selector_t *)malloc(sizeof(events_selector_t));
   if (result) {
      result->fn = fn_selector;
      result->timeout.tv_sec = result->timeout.tv_nsec = 0;
      FD_ZERO(&(result->fd_read));
      FD_ZERO(&(result->fd_write));
      FD_ZERO(&(result->fd_exception));
   }
   return (cad_events_t *)result; // &(result->fn)
}
