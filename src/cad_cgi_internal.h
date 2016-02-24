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
 * This file contains the internal header for the CGI implementation
 */

#include "cad_cgi.h"

cad_cgi_cookies_t *new_cookies(cad_memory_t memory);
void flush_cookies(cad_cgi_cookies_t *cookies);
void free_cookies(cad_cgi_cookies_t *this);
