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
 * @ingroup cad_event_queue
 * @file
 *
 * This file contains the POSIX threads implementation of event
 * queues.
 */

#define _GNU_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <pthread.h>
#include <unistd.h>

#include "cad_event_queue.h"

#define STATE_INIT 0
#define STATE_RUN  1
#define STATE_STOP 2

typedef struct {
   cad_event_queue_t fn;
   cad_memory_t memory;
   provide_data_fn provider;
   void *data;
   pthread_rwlock_t lock;
   pthread_t thread;
   int pipe[2];
   int state;
} event_queue_pthread_t;

static int get_fd_pthread(event_queue_pthread_t *this) {
   return this->pipe[0];
}

static void *pull_pthread(event_queue_pthread_t *this) {
   void *buffer;
   int n = read(this->pipe[0], (void *)&buffer, sizeof(void *));
   void *result = NULL;
   if (n == sizeof(void *)) {
      result = buffer;
   }
   return result;
}

static int is_running_pthread(event_queue_pthread_t *this) {
   return ACCESS_ONCE(this->state) == STATE_RUN;
}

static void *pthread_main(event_queue_pthread_t *this) {
   void *data;
   struct pollfd w;

   while (ACCESS_ONCE(this->state) == STATE_INIT) { /* wait for init */
      poll(NULL, 0, 1);
   }

   w.fd = this->pipe[1];
   w.events = POLLOUT;
   while (ACCESS_ONCE(this->state) == STATE_RUN) {
      w.revents = 0;
      poll(&w, 1, 10);
      if (w.revents & POLLOUT) {
         if (0 == pthread_rwlock_rdlock(&(this->lock))) {
            data = this->provider(this->data);
            if (data) {
               if (write(this->pipe[1], (void *)&data, sizeof(void *)) < (int)sizeof(void *)) {
                  this->fn.stop(&(this->fn)); /* TODO error handling */
               }
            }
            pthread_rwlock_unlock(&(this->lock));
         }
         poll(NULL, 0, 10); /* 10 ms not to clog the CPU */
      }
   }

   return NULL;
}

static void start_pthread(event_queue_pthread_t *this, void *data) {
   if (ACCESS_ONCE(this->state) == STATE_INIT) {
      if (0 == pthread_rwlock_wrlock(&(this->lock))) {
         this->data = data;
         if (pthread_create(&(this->thread), NULL, (void *(*)(void *))pthread_main, this) == 0) {
            this->state = STATE_RUN;
         }
      }
      pthread_rwlock_unlock(&(this->lock));
   }
}

static void stop_pthread(event_queue_pthread_t *this) {
   int state = ACCESS_ONCE(this->state);
   if (0 == pthread_rwlock_wrlock(&(this->lock))) {
      this->state = STATE_STOP;
      this->data = NULL;
      pthread_rwlock_unlock(&(this->lock));
   }
   if (state == STATE_RUN) {
      pthread_join(this->thread, NULL);
   }
}

static void free_pthread(event_queue_pthread_t *this) {
   stop_pthread(this);
   pthread_rwlock_destroy(&(this->lock));
   this->memory.free(this);
}

static cad_event_queue_t fn_pthread = {
   .get_fd     = (cad_event_queue_get_fd_fn)get_fd_pthread,
   .is_running = (cad_event_queue_is_running_fn)is_running_pthread,
   .pull       = (cad_event_queue_pull_fn)pull_pthread,
   .start      = (cad_event_queue_start_fn)start_pthread,
   .stop       = (cad_event_queue_stop_fn)stop_pthread,
   .free       = (cad_event_queue_free_fn)free_pthread,
};

__PUBLIC__ cad_event_queue_t *cad_new_event_queue_pthread(cad_memory_t memory, provide_data_fn provider, size_t capacity) {
   event_queue_pthread_t *result = (event_queue_pthread_t *)memory.malloc(sizeof(event_queue_pthread_t));
   if (result) {
      result->fn = fn_pthread;
      result->memory = memory;
      result->provider = provider;
      pthread_rwlock_init(&(result->lock), NULL);
      result->state = STATE_INIT;
      if (pipe(result->pipe) < 0) {
         perror("cad_new_event_queue_pthread:pipe(2)");
         memory.free(result);
         result = NULL;
      }
#ifdef F_SETPIPE_SZ
      if (fcntl(result->pipe[1], F_SETPIPE_SZ, (int)(capacity * sizeof(void*))) < 0) {
         perror("cad_new_event_queue_pthread:fcntl(2)");
         memory.free(result);
         result = NULL;
      }
#endif
   }
   return (cad_event_queue_t *)result;
}
