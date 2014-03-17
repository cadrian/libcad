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

#ifndef __CAD_EVENT_QUEUE_H__
#define __CAD_EVENT_QUEUE_H__

/**
 * @ingroup cad_event_queue
 * @file
 *
 * Implements an asynchronous data queue. Data is available via its
 * file descriptor (useful for events).
 *
 * The user provides a function that allocates the items to enqueue;
 * and the user pulls the items from the queue to use them and
 * deallocate them when necessary.
 *
 * The queue itself is not responsible of the allocation and
 * deallocation of the user's items.
 */

#include "cad_shared.h"

/**
 * @addtogroup cad_event_queue
 * @{
 */

/**
 * The event queue public interface.
 */
typedef struct cad_event_queue_s cad_event_queue_t;

/**
 * The user must provide a function of this type. Each time this
 * function is called, its result will be queued.
 *
 * @param[in] data a payload data given at event queue startup
 *
 */
typedef void *(*provide_data_fn)(void *data);

/**
 * Gets the file descriptor used by this event queue. Useful for select(2).
 *
 * @param[in] this the target event queue
 *
 * @return the file descriptor.
 *
 */
typedef int (*event_queue_get_fd_fn)(cad_event_queue_t *this);

/**
 * Returns non-zero if the queue is running.
 *
 * @param[in] this the target event queue
 *
 * @return true or false.
 *
 */
typedef int (*event_queue_is_running_fn)(cad_event_queue_t *this);

/**
 * Pulls an item from the queue.
 *
 * @param[in] this the target event queue
 *
 * @return the pulled item, or NULL if impossible.
 *
 */
typedef void *(*event_queue_pull_fn)(cad_event_queue_t *this);

/**
 * Starts the queue.
 *
 * @param[in] this the target event queue
 * @param[in] data the payload data that is forwarded to the data provider function
 *
 */
typedef void (*event_queue_start_fn)(cad_event_queue_t *this, void *data);

/**
 * Stops the queue.
 *
 * @param[in] this the target event queue
 *
 */
typedef void (*event_queue_stop_fn)(cad_event_queue_t *this);

/**
 * Frees the queue.
 *
 * \a Note: this frees the queue and its internal structures, but not
 * the user-allocated items!
 *
 * @param[in] this the target event queue
 *
 */
typedef void (*event_queue_free_fn)(cad_event_queue_t *this);

struct cad_event_queue_s {
   /**
    * @see event_queue_get_fd_fn
    */
   event_queue_get_fd_fn     get_fd;
   /**
    * @see event_queue_is_running_fn
    */
   event_queue_is_running_fn is_running;
   /**
    * @see event_queue_pull_fn
    */
   event_queue_pull_fn       pull;
   /**
    * @see event_queue_start_fn
    */
   event_queue_start_fn      start;
   /**
    * @see event_queue_stop_fn
    */
   event_queue_stop_fn       stop;
   /**
    * @see event_queue_free_fn
    */
   event_queue_free_fn       free;
};

/**
 * Allocates and return a new event queue implemented using POSIX threads.
 *
 * @return the newly allocated event queue.
 *
 */
__PUBLIC__ cad_event_queue_t *cad_new_event_queue_pthread(provide_data_fn provider);

/**
 * @}
 */

#endif /* __CAD_EVENT_QUEUE_H__ */
