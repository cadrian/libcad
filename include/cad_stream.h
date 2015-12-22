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

#ifndef _CAD_STREAM_H_
#define _CAD_STREAM_H_

/**
 * @ingroup cad_stream
 * @file
 *
 * Input and output streams.
 */

#include <stdio.h>
#include "cad_shared.h"

/**
 * @addtogroup cad_stream
 * @{
 */

/**
 * @addtogroup cad_in_stream
 * @{
 */

/**
 * The input stream public interface.
 *
 * The abstraction is a data cursor: the "current" byte is always
 * available via item(); use next() to advance the cursor.
 */
typedef struct cad_input_stream cad_input_stream_t;

/**
 * Frees the input stream
 *
 * @param[in] this the target input stream
 */
typedef void (*cad_input_stream_free_fn)(cad_input_stream_t *this);

/**
 * Asks the stream to read the next byte.
 *
 * @param[in] this the target input stream
 *
 * @return 0 if OK, -1 if error
 */
typedef int (*cad_input_stream_next_fn)(cad_input_stream_t *this);

/**
 * Asks the input stream the last read byte.
 *
 * @param[in] this the target input stream
 *
 * @return the current byte in the stream.
 */
typedef int (*cad_input_stream_item_fn)(cad_input_stream_t *this);

struct cad_input_stream {
     /**
      * @see cad_input_stream_free_fn
      */
     cad_input_stream_free_fn free;
     /**
      * @see cad_input_stream_next_fn
      */
     cad_input_stream_next_fn next;
     /**
      * @see cad_input_stream_item_fn
      */
     cad_input_stream_item_fn item;
};

/**
 * Creates a new input stream that reads bytes from the given C
 * string (will stop at '\\0'), using the given memory manager and
 * returns it.
 *
 * @param[in] string the C string to read bytes from
 * @param[in] memory the memory manager
 *
 * @return a stream that reads bytes from the given string.
 */
__PUBLIC__ cad_input_stream_t *new_cad_input_stream_from_string         (const char *string, cad_memory_t memory);

/**
 * Creates a new input stream that reads bytes from the given
 * file, using the given memory manager and returns it.
 *
 * @param[in] file the file to read from (must be open for reading)
 * @param[in] memory the memory manager
 *
 * @return a stream that reads bytes from the given file.
 */
__PUBLIC__ cad_input_stream_t *new_cad_input_stream_from_file           (FILE *file,   cad_memory_t memory);

/**
 * Creates a new input stream that reads bytes from the given
 * file descriptor, using the given memory manager and returns it.
 *
 * @param[in] fd the file descriptor to read from (must be open for reading)
 * @param[in] memory the memory manager
 *
 * @return a stream that reads bytes from the given file descriptor.
 */
__PUBLIC__ cad_input_stream_t *new_cad_input_stream_from_file_descriptor(int fd,       cad_memory_t memory);

/**
 * @}
 */

/**
 * @addtogroup cad_out_stream
 * @{
 */

/**
 * The output stream interface.
 *
 * The abstraction is a byte bucket.
 */
typedef struct cad_output_stream cad_output_stream_t;

/**
 * Frees the output stream
 *
 * @param[in] this the target output stream
 */
typedef void (*cad_output_stream_free_fn)(cad_output_stream_t *this);

/**
 * Puts bytes to the output stream
 *
 * @param[in] this the target output stream
 * @param[in] format the format of the bytes to put, compatible with printf()
 * @param[in] ... the arguments of the format
 */
typedef void (*cad_output_stream_put_fn  )(cad_output_stream_t *this, const char *format, ...) __PRINTF__;

/**
 * Flushes bytes to the underlying stream
 *
 * @param[in] this the target output stream
 */
typedef void (*cad_output_stream_flush_fn)(cad_output_stream_t *this);

struct cad_output_stream {
     /**
      * @see cad_output_stream_free_fn
      */
     cad_output_stream_free_fn  free ;
     /**
      * @see cad_output_stream_put_fn
      */
     cad_output_stream_put_fn   put  ;
     /**
      * @see cad_output_stream_flush_fn
      */
     cad_output_stream_flush_fn flush;
};

/**
 * Creates a new output stream using the memory manager and
 * returns it. That string is allocated as needed (and may be
 * allocated more than once); the address of the string is always
 * stored at the provided `string` location.
 *
 * @param[in,out] string the address of the string
 * @param[in] memory the memory manager
 *
 * @return a stream that writes bytes to a string.
 */
__PUBLIC__ cad_output_stream_t *new_cad_output_stream_from_string         (char **string, cad_memory_t memory);

/**
 * Creates a new output stream using the memory manager and
 * returns it.
 *
 * @param[in] file the file to write bytes to (must be open for writing or appending)
 * @param[in] memory the memory manager
 *
 * @return a stream that writes bytes into the given file.
 */
__PUBLIC__ cad_output_stream_t *new_cad_output_stream_from_file           (FILE *file,    cad_memory_t memory);

/**
 * Creates a new output stream using the memory manager and
 * returns it.
 *
 * @param[in] fd the file descriptor to write bytes to (must be open for writing or appending)
 * @param[in] memory the memory manager
 *
 * @return a stream that writes bytes into the given file descriptor.
 */
__PUBLIC__ cad_output_stream_t *new_cad_output_stream_from_file_descriptor(int fd,        cad_memory_t memory);

/**
 * @}
 */

/**
 * @}
 */

#endif /* _CAD_STREAM_H_ */
