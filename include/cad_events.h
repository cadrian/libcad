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

#ifndef __CAD_EVENTS_H__
#define __CAD_EVENTS_H__

/**
 * @ingroup cad_events
 * @file
 *
 * An events loop that waits for events on file descriptors and starts
 * actions accordingly.
 *
 * Supported events:
 *  - file descriptor ready to be read from
 *  - file descriptor ready to be written to
 *  - file descriptor in exception state
 *  - time out
 *
 * The underlying implementation typically uses select(2).
 *
 * Ideas: from Liberty Eiffel's sequencer library, thanks to Philippe
 * Ribet.
 * http://www.liberty-eiffel.org
 *
 */

#include "cad_shared.h"

/**
 * @addtogroup cad_events
 * @{
 */

/**
 * The events loop public interface.
 */
typedef struct cad_events_s cad_events_t;

/**
 * The user may supply an action to call on event timeout.
 *
 * @param[in] data a payload data given at wait
 *
 */
typedef void (*cad_on_timeout_action)(void *data);

/**
 * The user may supply an action to call on read, write, or exception.
 *
 * @param[in] fd the file descriptor to act upon
 * @param[in] data a payload data given at wait
 *
 */
typedef void (*cad_on_descriptor_action)(int fd, void *data);

/**
 * Sets the wait time-out.
 *
 * @param[in] this the target event loop
 * @param[in] timeout_us the time-out in microseconds
 *
 */
typedef void (*cad_events_set_timeout_fn)(cad_events_t *this, unsigned long timeout_us);

/**
 * Sets a read file descriptor that must be waited upon.
 *
 * @param[in] this the target event loop
 * @param[in] fd the file descriptor to read
 *
 */
typedef void (*cad_events_set_read_fn)(cad_events_t *this, int fd);

/**
 * Sets a write file descriptor that must be waited upon.
 *
 * @param[in] this the target event loop
 * @param[in] fd the file descriptor to write
 *
 */
typedef void (*cad_events_set_write_fn)(cad_events_t *this, int fd);

/**
 * Sets an exceptin file descriptor that must be waited upon.
 *
 * @param[in] this the target event loop
 * @param[in] fd the file descriptor to wait for an exception
 *
 */
typedef void (*cad_events_set_exception_fn)(cad_events_t *this, int fd);

/**
 * Sets the time-out action
 *
 * @param[in] this the target event loop
 * @param[in] action the action to set
 *
 */
typedef void (*cad_events_on_timeout_fn)(cad_events_t *this, cad_on_timeout_action action);

/**
 * Sets the when-read action
 *
 * @param[in] this the target event loop
 * @param[in] action the action to set
 *
 */
typedef void (*cad_events_on_read_fn)(cad_events_t *this, cad_on_descriptor_action action);

/**
 * Sets the when-write action
 *
 * @param[in] this the target event loop
 * @param[in] action the action to set
 *
 */
typedef void (*cad_events_on_write_fn)(cad_events_t *this, cad_on_descriptor_action action);

/**
 * Sets the when-exception action
 *
 * @param[in] this the target event loop
 * @param[in] action the action to set
 *
 */
typedef void (*cad_events_on_exception_fn)(cad_events_t *this, cad_on_descriptor_action action);

/**
 * Waits for file descriptors or time-out, and calls actions if relevant.
 *
 * \a Note: file descriptors and time-out stay set, one may keep calling wait() in a loop.
 *
 * @param[in] this the target event loop
 * @param[in] a payload data forwarded to the actions
 *
 */
typedef int  (*cad_events_wait_fn)(cad_events_t *this, void *data);

/**
 * Frees the event loop.
 *
 * @param[in] this the target event loop
 *
 */
typedef void (*cad_events_free_fn)(cad_events_t *this);

struct cad_events_s {
   /**
    * @see events_set_timeout_fn
    */
   cad_events_set_timeout_fn    set_timeout;
   /**
    * @see events_set_read_fn
    */
   cad_events_set_read_fn       set_read;
   /**
    * @see events_set_write_fn
    */
   cad_events_set_write_fn      set_write;
   /**
    * @see events_set_exception_fn
    */
   cad_events_set_exception_fn  set_exception;
   /**
    * @see events_on_timeout_fn
    */
   cad_events_on_timeout_fn     on_timeout;
   /**
    * @see events_on_read_fn
    */
   cad_events_on_read_fn        on_read;
   /**
    * @see events_on_write_fn
    */
   cad_events_on_write_fn       on_write;
   /**
    * @see events_on_exception_fn
    */
   cad_events_on_exception_fn   on_exception;
   /**
    * @see events_wait_fn
    */
   cad_events_wait_fn           wait;
   /**
    * @see events_free_fn
    */
   cad_events_free_fn           free;
};

/**
 * Allocates and return a new events loop implemented using pselect(2).
 *
 * @return the newly allocated events loop
 *
 */
__PUBLIC__ cad_events_t *cad_new_events_selector(cad_memory_t memory);

/**
 * Allocates and return a new events loop implemented using poll(2).
 *
 * @return the newly allocated events loop
 *
 */
__PUBLIC__ cad_events_t *cad_new_events_poller(cad_memory_t memory);

/**
 * @}
 */

#endif /* __CAD_EVENTS_H__ */
