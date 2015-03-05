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

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <poll.h>

#ifndef POLLRDHUP
#define POLLRDHUP 0
#endif

/**
 * @ingroup cad_events
 * @file
 *
 * This file contains the implementations of event loops using select(2).
 */

#include "cad_events.h"
#include "cad_array.h"

typedef struct {
     cad_events_t fn;
     cad_memory_t memory;
     union {
          struct {
               fd_set read, write, exception;
               int max;
          } selector;
          cad_array_t *poller;
     } fd;
     struct timespec timeout;
     void (*on_timeout)(void *);
     void (*on_read)(int, void *);
     void (*on_write)(int, void *);
     void (*on_exception)(int, void *);
} events_impl_t;

static void set_timeout_impl(events_impl_t *this, unsigned long timeout_us) {
     this->timeout.tv_sec = (long)(timeout_us / 1000000UL);
     this->timeout.tv_nsec = (long)(timeout_us % 1000000UL) * 1000L;
}

static void on_timeout_impl(events_impl_t *this, void (*action)(void *)) {
     this->on_timeout = action;
}

static void on_read_impl(events_impl_t *this, void (*action)(int, void *)) {
     this->on_read = action;
}

static void on_write_impl(events_impl_t *this, void (*action)(int, void *)) {
     this->on_write = action;
}

static void on_exception_impl(events_impl_t *this, void (*action)(int, void *)) {
     this->on_exception = action;
}

static void set_read_selector(events_impl_t *this, int fd) {
     FD_SET(fd, &(this->fd.selector.read));
     if (this->fd.selector.max < fd) {
          this->fd.selector.max = fd;
     }
}

static void set_write_selector(events_impl_t *this, int fd) {
     FD_SET(fd, &(this->fd.selector.write));
     if (this->fd.selector.max < fd) {
          this->fd.selector.max = fd;
     }
}

static void set_exception_selector(events_impl_t *this, int fd) {
     FD_SET(fd, &(this->fd.selector.exception));
     if (this->fd.selector.max < fd) {
          this->fd.selector.max = fd;
     }
}

static int wait_selector(events_impl_t *this, void *data) {
     int res;
     int i;
     fd_set r = this->fd.selector.read, w = this->fd.selector.write, x = this->fd.selector.exception;
     struct timespec t = this->timeout;
     res = pselect(this->fd.selector.max + 1, &r, &w, &x, &t, NULL);
     if (res == 0) {
          if (this->on_timeout != NULL) {
               this->on_timeout(data);
          }
     } else if (res > 0) {
          for (i = 0; i <= this->fd.selector.max; i++) {
               if (this->on_read != NULL && FD_ISSET(i, &(this->fd.selector.read)) && FD_ISSET(i, &r)) {
                    this->on_read(i, data);
               }
               if (this->on_write != NULL && FD_ISSET(i, &(this->fd.selector.write)) && FD_ISSET(i, &w)) {
                    this->on_write(i, data);
               }
               if (this->on_exception != NULL && FD_ISSET(i, &(this->fd.selector.exception)) && FD_ISSET(i, &x)) {
                    this->on_exception(i, data);
               }
          }
     } // else res < 0 => error (returned)
     FD_ZERO(&(this->fd.selector.read));
     FD_ZERO(&(this->fd.selector.write));
     FD_ZERO(&(this->fd.selector.exception));
     return res;
}

static void free_selector(events_impl_t *this) {
     this->memory.free(this);
}

static struct pollfd *find_pollfd(events_impl_t *this, int fd) {
     struct pollfd *result = NULL;
     struct pollfd *p;
     struct pollfd n;
     nfds_t count = this->fd.poller->count(this->fd.poller);
     int i;

     for (i = 0; result == NULL && i < count; i++) {
          p = this->fd.poller->get(this->fd.poller, i);
          if (p->fd == fd) {
               result = p;
          }
     }
     if (result == NULL) {
          n.fd = fd;
          n.events = n.revents = 0;
          result = this->fd.poller->insert(this->fd.poller, this->fd.poller->count(this->fd.poller), &n);
     }

     return result;
}

static void set_read_poller(events_impl_t *this, int fd) {
     struct pollfd *p = find_pollfd(this, fd);
     p->events |= POLLIN;
}

static void set_write_poller(events_impl_t *this, int fd) {
     struct pollfd *p = find_pollfd(this, fd);
     p->events |= POLLOUT;
}

static void set_exception_poller(events_impl_t *this, int fd) {
     struct pollfd *p = find_pollfd(this, fd);
     p->events |= POLLERR | POLLHUP | POLLRDHUP;
}

static int wait_poller(events_impl_t *this, void *data) {
     int res;
     int i, n = this->fd.poller->count(this->fd.poller);
     struct timespec t = this->timeout;
     struct pollfd *p = this->fd.poller->get(this->fd.poller, 0);
     res = ppoll(p, (nfds_t)n, &t, NULL);
     if (res == 0) {
          if (this->on_timeout != NULL) {
               this->on_timeout(data);
          }
     } else if (res > 0) {
          for (i = 0; i <= n; p++, i++) {
               if (p->revents) {
                    if (p->revents & POLLIN) {
                         this->on_read(p->fd, data);
                    }
                    if (p->revents & POLLOUT) {
                         this->on_write(p->fd, data);
                    }
                    if (p->revents & (POLLERR | POLLHUP | POLLRDHUP)) {
                         this->on_exception(p->fd, data);
                    }
               }
          }
     } // else res < 0 => error (returned)
     this->fd.poller->clear(this->fd.poller);
     return res;
}

static void free_poller(events_impl_t *this) {
     this->fd.poller->free(this->fd.poller);
     this->memory.free(this);
}

cad_events_t fn_selector = {
     .set_timeout   = (cad_events_set_timeout_fn)set_timeout_impl,
     .set_read      = (cad_events_set_read_fn)set_read_selector,
     .set_write     = (cad_events_set_write_fn)set_write_selector,
     .set_exception = (cad_events_set_exception_fn)set_exception_selector,
     .on_timeout    = (cad_events_on_timeout_fn)on_timeout_impl,
     .on_read       = (cad_events_on_read_fn)on_read_impl,
     .on_write      = (cad_events_on_write_fn)on_write_impl,
     .on_exception  = (cad_events_on_exception_fn)on_exception_impl,
     .wait          = (cad_events_wait_fn)wait_selector,
     .free          = (cad_events_free_fn)free_selector,
};

cad_events_t fn_poller = {
     .set_timeout   = (cad_events_set_timeout_fn)set_timeout_impl,
     .set_read      = (cad_events_set_read_fn)set_read_poller,
     .set_write     = (cad_events_set_write_fn)set_write_poller,
     .set_exception = (cad_events_set_exception_fn)set_exception_poller,
     .on_timeout    = (cad_events_on_timeout_fn)on_timeout_impl,
     .on_read       = (cad_events_on_read_fn)on_read_impl,
     .on_write      = (cad_events_on_write_fn)on_write_impl,
     .on_exception  = (cad_events_on_exception_fn)on_exception_impl,
     .wait          = (cad_events_wait_fn)wait_poller,
     .free          = (cad_events_free_fn)free_poller,
};

__PUBLIC__ cad_events_t *cad_new_events_selector(cad_memory_t memory) {
     events_impl_t *result = (events_impl_t *)memory.malloc(sizeof(events_impl_t));
     if (result) {
          result->fn = fn_selector;
          result->memory = memory;
          result->timeout.tv_sec = result->timeout.tv_nsec = 0;
          FD_ZERO(&(result->fd.selector.read));
          FD_ZERO(&(result->fd.selector.write));
          FD_ZERO(&(result->fd.selector.exception));
          result->fd.selector.max = 0;
     }
     return (cad_events_t *)result; // &(result->fn)
}

__PUBLIC__ cad_events_t *cad_new_events_poller(cad_memory_t memory) {
     events_impl_t *result = (events_impl_t *)memory.malloc(sizeof(events_impl_t));
     if (result) {
          result->fn = fn_poller;
          result->memory = memory;
          result->timeout.tv_sec = result->timeout.tv_nsec = 0;
          result->fd.poller = cad_new_array(stdlib_memory, sizeof(struct pollfd));
     }
     return (cad_events_t *)result; // &(result->fn)
}
