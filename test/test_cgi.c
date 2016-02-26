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

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "test.h"
#include "cad_cgi.h"

/**
 * The tested content must have been set by the test_cgi.sh script.
 */
static int handler(cad_cgi_t *cgi, cad_cgi_response_t *response) {
   cad_cgi_meta_t *meta = response->meta_variables(response);
   const char *verb = meta->request_method(meta);
   assert(verb != NULL && !strcmp(verb, "GET"));
   cad_hash_t *query = meta->query_string(meta);
   assert(query != NULL);
   assert(query->count(query) == 1);
   char *foo = query->get(query, "foo");
   assert(foo != NULL && !strcmp(foo, "bar"));
   cad_cgi_auth_type_e auth_type = meta->auth_type(meta);
   assert(auth_type == Auth_basic);
   const char *user = meta->remote_user(meta);
   assert(user != NULL && !strcmp(user, "test"));
   cad_output_stream_t *out = response->body(response);
   out->put(out, "Test Body.\n");
   return 0;
}

int main(int argc, char **argv) {
   const char *server_protocol = getenv("SERVER_PROTOCOL");
   if (server_protocol == NULL) {
      // called from Makefile; just execute the test script instead
      int n = strlen(argv[0]);
      char *arg = malloc(n + 1);
      strcpy(arg, argv[0]);
      arg[n-3] = 's';
      arg[n-2] = 'h';
      arg[n-1] = 0;
      char *args[] = { arg, NULL };
      printf("Script: (%d) '%s'\n", n, argv[0]);
      execv(arg, args);
      fprintf(stderr, "**** Could not launch test script: (%d) '%s'\n", n, arg);
      return 1;
   }

   int result = 1;
   cad_cgi_t *cgi = new_cad_cgi(stdlib_memory, handler);
   cad_cgi_response_t *response = cgi->run(cgi);
   if (response != NULL) {
      response->flush(response);
      response->free(response);
      result = 0;
   }
   cgi->free(cgi);

   return result;
}
