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

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "cad_event_queue.h"

typedef struct {
   cad_event_queue_t fn;
   cad_memory_t memory;
   provide_data_fn provider;
   void *data;
   pthread_mutex_t lock;
   pthread_t thread;
   int pipe[2];
   int running;
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
   int result = 0;
   if (0 == pthread_mutex_lock(&(this->lock))) {
      result = this->running;
      pthread_mutex_unlock(&(this->lock));
   }
   return result;
}

static void *pthread_main(event_queue_pthread_t *this) {
   void *data;

   while (!is_running_pthread(this)) { /* wait for init */
   }

   while (is_running_pthread(this)) {
      data = this->provider(this->data);
      if (data) {
         if (write(this->pipe[1], (void *)&data, sizeof(void *)) < (int)sizeof(void *)) {
            this->fn.stop(&(this->fn)); /* WHAT CAN I DO ?? */
         }
      }
      usleep(50000UL); /* 50 ms not to clog the CPU */
   }

   return NULL;
}

static void start_pthread(event_queue_pthread_t *this, void *data) {
   if (0 == pthread_mutex_lock(&(this->lock))) {
      if (!this->running) {
         this->data = data;
         if (!pthread_create(&(this->thread), NULL, (void *(*)(void *))pthread_main, this)) {
            this->running = 1;
         }
      }
      pthread_mutex_unlock(&(this->lock));
   }
}

static void stop_pthread(event_queue_pthread_t *this) {
   int running = 0;
   if (0 == pthread_mutex_lock(&(this->lock))) {
      running = this->running;
      this->running = 0;
      this->data = NULL;
      pthread_mutex_unlock(&(this->lock));
   }
   if (running) {
      pthread_join(this->thread, NULL);
   }
}

static void free_pthread(event_queue_pthread_t *this) {
   stop_pthread(this);
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

__PUBLIC__ cad_event_queue_t *cad_new_event_queue_pthread(cad_memory_t memory, provide_data_fn provider) {
   event_queue_pthread_t *result = (event_queue_pthread_t *)memory.malloc(sizeof(event_queue_pthread_t));
   if (result) {
      result->fn = fn_pthread;
      result->memory = memory;
      result->provider = provider;
      pthread_mutex_init(&(result->lock), NULL);
      result->running = 0;
      if (pipe(result->pipe) < 0) {
         memory.free(result);
         result = NULL;
      }
   }
   return (cad_event_queue_t *)result;
}
