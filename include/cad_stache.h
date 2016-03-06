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

#ifndef _CAD_STACHE_H_
#define _CAD_STACHE_H_

/**
 * @ingroup cad_stache
 * @file
 *
 * My implementation of the Mustache template engine.
 */

#include "cad_shared.h"
#include "cad_stream.h"

/**
 * @addtogroup cad_stache
 * @{
 */

typedef struct cad_stache cad_stache_t;

typedef enum {
   Cad_stache_not_found = 0,
   Cad_stache_string,
   Cad_stache_list,
   Cad_stache_partial,
} cad_stache_lookup_type;

typedef union cad_stache_resolved cad_stache_resolved_t;
union cad_stache_resolved {
   /**
    * for {@ref Cad_stache_string}
    */
   struct {
      /**
       * must return the string data
       */
      const char *(*get)(cad_stache_resolved_t *this);
      /**
       * must return non-zero on success; the engine will not use the
       * object after this call, it may be freed
       */
      int (*free)(cad_stache_resolved_t *this);
   } string;
   /**
    * for {@ref Cad_stache_list}
    */
   struct {
      /**
       * must return 0 at end of list, non-zero otherwise
       */
      int (*get)(cad_stache_resolved_t *this, int index);
      /**
       * must return non-zero on success; the engine will not use the
       * object after this call, it may be freed.
       */
      int (*close)(cad_stache_resolved_t *this);
   } list;
   /**
    * for {@ref Cad_stache_partial}
    */
   struct {
      /**
       * must return NULL if not found, a stream otherwise
       */
      cad_input_stream_t *(*get)(cad_stache_resolved_t *this);
      /**
       * must return non-zero on success; the engine will not use the
       * object after this call, it may be freed.
       *
       * NOTE: this function is also in charge of freeing the input
       * stream returned by get()
       */
      int (*free)(cad_stache_resolved_t *this);
   } partial;
};

/**
 * Frees the 'stache
 *
 * @param[in] this the target 'stache
 */
typedef void (*cad_stache_free_fn)(cad_stache_t *this);

/**
 * Renders the input stream into the output stream using the registered variables
 *
 * @param[in] this the target 'stache
 * @param[in] input the input stream, that contains the 'stache template
 * @param[in] output the output stream, that will be written to
 * @param[in] on_error called on error with a message and the parse index at which the error occurs
 * @param[in] on_error_data passed to on_error
 *
 * @return 0 if OK, -1 if error
 */
typedef int (*cad_stache_render_fn)(cad_stache_t *this, cad_input_stream_t *input, cad_output_stream_t *output, void (*on_error)(const char *, int, void*), void *on_error_data);

/**
 * Callback implemented by the caller; used to resolve variables.
 *
 * Note: resolved data is guaranteed not to be shared between
 * different invocations of {@ref cad_stache_render_fn}.
 *
 * @param[in] stache the 'stache caller
 * @param[in] the variable name
 * @param[in] data given at the 'stache creation
 * @param[out] the resolved lookup to fill
 *
 * @return the resolved lookup type. If {@ref Cad_stache_not_found},
 * `resolved' will not be changed; otherwise it will be set with the
 * relevant data.
 */
typedef cad_stache_lookup_type (*cad_stache_resolve_cb)(cad_stache_t *stache, const char *name, void *data, cad_stache_resolved_t **resolved);

struct cad_stache {
   /**
    * @see cad_stache_free_fn
    */
   cad_stache_free_fn free;
   /*
    * @see cad_stache_render_fn
    */
   cad_stache_render_fn render;
};

/**
 * Creates a new 'stache using the memory manager and resolve
 * callback, and returns it.
 *
 * @param[in] memory the memory manager
 * @param[in] callback the resolve callback
 * @param[in] data forwarded to the callback
 *
 * @return a new 'stache
 */
__PUBLIC__ cad_stache_t *new_cad_stache(cad_memory_t memory, cad_stache_resolve_cb callback, void *data);

/**
 * @}
 */

#endif /* _CAD_STACHE_H_ */
