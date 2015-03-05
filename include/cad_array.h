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

#ifndef _CAD_ARRAY_H_
#define _CAD_ARRAY_H_

/**
 * @ingroup cad_array
 * @file
 *
 * An array. Accepts any kinds values as long as they have the same size.
 */

#include "cad_shared.h"

/**
 * @addtogroup cad_array
 * @{
 */

/**
 * The array public interface.
 */
typedef struct cad_array_s cad_array_t;

/**
 * Frees the array.
 *
 * \a Note: does not free its content!
 *
 * @param[in] this the target array
 *
 */
typedef void (*cad_array_free_fn) (cad_array_t *this);

/**
 * Counts the number of elements in the array.
 *
 * @param[in] this the target array
 *
 * @return the number of elements.
 *
 */
typedef unsigned int (*cad_array_count_fn) (cad_array_t *this);

/**
 * Retrieves the pointer to the `index`-th value.
 *
 * Note: this being an array, the elements are sure to be continuous.
 *
 * @param[in] this the target array
 * @param[in] index the index to lookup
 *
 * @return the pointer to the `index`-th value, `NULL` if out of bounds.
 *
 */
typedef void *(*cad_array_get_fn) (cad_array_t *this, unsigned int index);

/**
 * Inserts the `index`-th `value`. Will expand the array as needed.
 *
 * @param[in] this the target array
 * @param[in] index the index of the value to set
 * @param[in] value the value
 *
 * @return the pointer to the inserted value.
 *
 */
typedef void *(*cad_array_insert_fn) (cad_array_t *this, unsigned int index, void *value);

/**
 * Replaces the `index`-th `value`. Will expand the array as needed.
 *
 * @param[in] this the target array
 * @param[in] index the index of the value to set
 * @param[in] value the value
 *
 * @return the pointer to the updated value.
 *
 */
typedef void *(*cad_array_update_fn) (cad_array_t *this, unsigned int index, void *value);

/**
 * Removes both the `index`-th value.
 *
 * @param[in] this the target array
 * @param[in] index the index of the value to delete
 *
 * @return the pointer to the deleted value, `NULL` if out of bounds.
 *
 */
typedef void *(*cad_array_del_fn) (cad_array_t *this, unsigned int index);

/**
 * The comparator used to sort the array.
 *
 * @see cad_array_sort_fn
 */
typedef int(*comparator_fn)(const void*,const void*);

/**
 * Sorts the array using the `comparator`.
 *
 * Note: if the values were malloc()ed the client must free() them beforehand.
 *
 * @param[in] this the target array
 * @param[in] comparator the values comparator
 *
 */
typedef void (*cad_array_sort_fn) (cad_array_t *this, comparator_fn comparator);

/**
 * Empties the array.
 *
 * @param[in] this the target array
 *
 */
typedef void (*cad_array_clear_fn) (cad_array_t *this);

struct cad_array_s {
     /**
      * @see array_free_fn
      */
     cad_array_free_fn    free;
     /**
      * @see array_count_fn
      */
     cad_array_count_fn   count;
     /**
      * @see array_get_fn
      */
     cad_array_get_fn     get;
     /**
      * @see array_insert_fn
      */
     cad_array_insert_fn  insert;
     /**
      * @see array_update_fn
      */
     cad_array_update_fn  update;
     /**
      * @see array_del_fn
      */
     cad_array_del_fn     del;
     /**
      * @see cad_array_sort_fn
      */
     cad_array_sort_fn sort;
     /**
      * @see cad_array_clear_fn
      */
     cad_array_clear_fn clear;
};

/**
 * Allocates and returns a new array.
 *
 * @return the newly allocated array.
 */
__PUBLIC__ cad_array_t *cad_new_array(cad_memory_t memory, size_t size);

/**
 * @}
 */

#endif /* _CAD_ARRAY_H_ */
