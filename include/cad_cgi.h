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

#ifndef _CAD_CGI_H_
#define _CAD_CGI_H_

#include "cad_hash.h"
#include "cad_shared.h"
#include "cad_stream.h"

typedef struct cad_cgi cad_cgi_t;
typedef struct cad_cgi_meta cad_cgi_meta_t;
typedef struct cad_cgi_response cad_cgi_response_t;
typedef struct cad_cgi_cookies cad_cgi_cookies_t;
typedef struct cad_cgi_cookie cad_cgi_cookie_t;

/* ---------------------------------------------------------------- */

/**
 * Frees the CGI context
 *
 * @param[in] this the target CGI context
 */
typedef void (*cad_cgi_free_fn)(cad_cgi_t *this);

/**
 * Runs the CGI script, by reading the CGI input and calling the handler.
 * Usually called only once.
 *
 * @param[in] this the target CGI context
 *
 * @return the response to flush on success, NULL on error
 */
typedef cad_cgi_response_t *(*cad_cgi_run_fn)(cad_cgi_t *this);

/**
 * Get the file descriptor of the input, needed for multiplexing
 * (e.g. libuv...)
 *
 * @param[in] this the target CGI context
 *
 * @return the file descriptor, or -1 on error
 */
typedef int (*cad_cgi_fd_fn)(cad_cgi_t *this);

struct cad_cgi {
   /**
    * @see cad_cgi_free_fn
    */
   cad_cgi_free_fn free;
   /**
    * @see cad_cgi_run_fn
    */
   cad_cgi_run_fn run;
   /**
    * @see cad_cgi_fd_fn
    */
   cad_cgi_fd_fn fd;
};

/* ---------------------------------------------------------------- */

typedef enum {
   Auth_invalid = -1,
   Auth_none = 0,
   Auth_basic,
   Auth_digest,
} cad_cgi_auth_type_e;

typedef struct {
   int major;
   int minor;
} cad_cgi_gateway_interface_t;

typedef struct {
   int major;
   int minor;
   const char *protocol;
} cad_cgi_server_protocol_t;

typedef struct {
   const char *type;
   const char *subtype;
   cad_hash_t *parameters;
} cad_cgi_content_type_t;

typedef cad_cgi_auth_type_e (*cad_cgi_meta_auth_type_fn)(cad_cgi_meta_t *this);
typedef size_t (*cad_cgi_meta_content_length_fn)(cad_cgi_meta_t *this);
typedef cad_cgi_content_type_t *(*cad_cgi_meta_content_type_fn)(cad_cgi_meta_t *this);
typedef cad_cgi_gateway_interface_t *(*cad_cgi_meta_gateway_interface_fn)(cad_cgi_meta_t *this);
typedef const char *(*cad_cgi_meta_path_info_fn)(cad_cgi_meta_t *this);
typedef const char *(*cad_cgi_meta_path_translated_fn)(cad_cgi_meta_t *this);
typedef cad_hash_t *(*cad_cgi_meta_query_string_fn)(cad_cgi_meta_t *this);
typedef cad_hash_t *(*cad_cgi_meta_input_as_form_fn)(cad_cgi_t *this);
typedef const char *(*cad_cgi_meta_remote_addr_fn)(cad_cgi_meta_t *this);
typedef const char *(*cad_cgi_meta_remote_host_fn)(cad_cgi_meta_t *this);
typedef const char *(*cad_cgi_meta_remote_ident_fn)(cad_cgi_meta_t *this);
typedef const char *(*cad_cgi_meta_remote_user_fn)(cad_cgi_meta_t *this);
typedef const char *(*cad_cgi_meta_request_method_fn)(cad_cgi_meta_t *this);
typedef const char *(*cad_cgi_meta_script_name_fn)(cad_cgi_meta_t *this);
typedef const char *(*cad_cgi_meta_server_name_fn)(cad_cgi_meta_t *this);
typedef int (*cad_cgi_meta_server_port_fn)(cad_cgi_meta_t *this);
typedef cad_cgi_server_protocol_t *(*cad_cgi_meta_server_protocol_fn)(cad_cgi_meta_t *this);
typedef const char *(*cad_cgi_meta_server_software_fn)(cad_cgi_meta_t *this);

struct cad_cgi_meta {
   /**
    * @see cad_cgi_meta_auth_type_fn
    */
   cad_cgi_meta_auth_type_fn auth_type;
   /**
    * @see cad_cgi_meta_content_length_fn
    */
   cad_cgi_meta_content_length_fn content_length;
   /**
    * @see cad_cgi_meta_content_type_fn
    */
   cad_cgi_meta_content_type_fn content_type;
   /**
    * @see cad_cgi_meta_gateway_interface_fn
    */
   cad_cgi_meta_gateway_interface_fn gateway_interface;
   /**
    * @see cad_cgi_meta_path_info_fn
    */
   cad_cgi_meta_path_info_fn path_info;
   /**
    * @see cad_cgi_meta_path_translated_fn
    */
   cad_cgi_meta_path_translated_fn path_translated;
   /**
    * @see cad_cgi_meta_query_string_fn
    */
   cad_cgi_meta_query_string_fn query_string;
   /**
    * @see cad_cgi_meta_input_as_form_fn
    */
   cad_cgi_meta_input_as_form_fn input_as_form;
   /**
    * @see cad_cgi_meta_remote_addr_fn
    */
   cad_cgi_meta_remote_addr_fn remote_addr;
   /**
    * @see cad_cgi_meta_remote_host_fn
    */
   cad_cgi_meta_remote_host_fn remote_host;
   /**
    * @see cad_cgi_meta_remote_ident_fn
    */
   cad_cgi_meta_remote_ident_fn remote_ident;
   /**
    * @see cad_cgi_meta_remote_user_fn
    */
   cad_cgi_meta_remote_user_fn remote_user;
   /**
    * @see cad_cgi_meta_request_method_fn
    */
   cad_cgi_meta_request_method_fn request_method;
   /**
    * @see cad_cgi_meta_script_name_fn
    */
   cad_cgi_meta_script_name_fn script_name;
   /**
    * @see cad_cgi_meta_server_name_fn
    */
   cad_cgi_meta_server_name_fn server_name;
   /**
    * @see _cgi_meta_server_port_fn
    */
   cad_cgi_meta_server_port_fn server_port;
   /**
    * @see _cgi_meta_server_protocol_fn
    */
   cad_cgi_meta_server_protocol_fn server_protocol;
   /**
    * @see _cgi_meta_server_software
    */
   cad_cgi_meta_server_software_fn server_software;
};

/* ---------------------------------------------------------------- */

/**
 * Get the CGI cookies
 *
 * @param[in] this the target CGI response
 *
 * @return the CGI cookies
 */
typedef cad_cgi_cookies_t *(*cad_cgi_response_cookies_fn)(cad_cgi_response_t *this);

/**
 * Get the CGI meta variables
 *
 * @param[in] this the target CGI response
 *
 * @return the CGI meta variables
 */
typedef cad_cgi_meta_t *(*cad_cgi_response_meta_variables_fn)(cad_cgi_response_t *this);

/**
 * Flush the response
 *
 * @param[in] this the target CGI response
 *
 * @return 0 on success, -1 on error
 */
typedef int (*cad_cgi_response_flush_fn)(cad_cgi_response_t *this);

/**
 * Get the file descriptor of the output, needed for multiplexing
 * (e.g. libuv...)
 *
 * @param[in] this the target CGI response
 *
 * @return the file descriptor, or -1 on error
 */
typedef int (*cad_cgi_response_fd_fn)(cad_cgi_response_t *this);

/**
 * Get the response body stream, used to put the actual body. Not
 * always available.
 *
 * @param[in] this the target CGI response
 *
 * @return the body stream, or NULL if not available
 */
typedef cad_output_stream_t *(*cad_cgi_response_body_fn)(cad_cgi_response_t *this);

/**
 * Mark the response as being a redirect.
 *
 * @param[in] this the target CGI response
 * @param[in] path the redirect path, must be non-NULL
 * @param[in] fragment the redirect fragment, must be non-NULL
 *
 * @return 0 on success, -1 on error
 */
typedef int (*cad_cgi_response_redirect_fn)(cad_cgi_response_t *this, const char *path, const char *fragment);

/**
 * Set the status (default is 200)
 *
 * @param[in] this the target CGI response
 * @param[in] status the status
 *
 * @return 0 on success, -1 on error
 */
typedef int (*cad_cgi_response_set_status_fn)(cad_cgi_response_t *this, int status);

/**
 * Set the context type (default is text/plain)
 *
 * @param[in] this the target CGI response
 * @param[in] content_type content_type
 *
 * @return 0 on success, -1 on error
 */
typedef int (*cad_cgi_response_set_content_type_fn)(cad_cgi_response_t *this, const char *content_type);

/**
 * Set the header field
 *
 * @param[in] this the target CGI response
 * @param[in] field the header field name, must be non-NULL
 * @param[in] value the header field value, must be non-NULL
 *
 * @return 0 on success, -1 on error
 */
typedef int (*cad_cgi_response_set_header_fn)(cad_cgi_response_t *this, const char *field, const char *value);

/**
 * Free the response
 *
 * @param[in] this the target CGI response
 *
 * @return 0 on success, -1 on error
 */
typedef int (*cad_cgi_response_free_fn)(cad_cgi_response_t *this);

struct cad_cgi_response {
   /**
    * @see cad_cgi_response_cookies_fn
    */
   cad_cgi_response_cookies_fn cookies;
   /**
    * @see cad_cgi_response_meta_variables_fn
    */
   cad_cgi_response_meta_variables_fn meta_variables;
   /**
    * @see cad_cgi_response_flush_fn
    */
   cad_cgi_response_flush_fn flush;
   /**
    * @see cad_cgi_response_fd_fn
    */
   cad_cgi_response_fd_fn fd;
   /**
    * @see cad_cgi_response_body_fn
    */
   cad_cgi_response_body_fn body;
   /**
    * @see cad_cgi_response_redirect_fn
    */
   cad_cgi_response_redirect_fn redirect;
   /**
    * @see cad_cgi_response_set_status_fn
    */
   cad_cgi_response_set_status_fn set_status;
   /**
    * @see cad_cgi_response_set_content_type_fn
    */
   cad_cgi_response_set_content_type_fn set_content_type;
   /**
    * @see cad_cgi_response_set_header_fn
    */
   cad_cgi_response_set_header_fn set_header;
   /**
    * @see cad_cgi_response_free_fn
    */
   cad_cgi_response_free_fn free;
};

/* ---------------------------------------------------------------- */

/**
 * Get a cookie
 *
 * @param[in] this the target CGI cookies
 * @param[in] name the cookie name
 *
 * @return the cookie, NULL if not found
 */
typedef cad_cgi_cookie_t *(*cad_cgi_cookies_get_fn)(cad_cgi_cookies_t *this, const char *name);

/**
 * Set a cookie
 *
 * @param[in] this the target CGI cookies
 * @param[in] cookie the cookie
 *
 * @return 0 on success, -1 on error
 */
typedef int (*cad_cgi_cookies_set_fn)(cad_cgi_cookies_t *this, cad_cgi_cookie_t *cookie);

struct cad_cgi_cookies {
   /**
    * @see _cookies_get_fn
    */
   cad_cgi_cookies_get_fn get;
   /**
    * @see cad_cgi_cookies_set_fn
    */
   cad_cgi_cookies_set_fn set;
};

/* ---------------------------------------------------------------- */

/**
 * Frees the cookie
 *
 * @param[in] this the target cookie
 */
typedef void (*cad_cgi_cookie_free_fn)(cad_cgi_cookie_t *this);

/**
 * Get the name of the cookie
 *
 * @param[in] this the target cookie
 *
 * @return the name
 */
typedef const char *(*cad_cgi_cookie_name_fn)(cad_cgi_cookie_t *this);

/**
 * Get the value of the cookie
 *
 * @param[in] this the target cookie
 *
 * @return the value
 */
typedef const char *(*cad_cgi_cookie_value_fn)(cad_cgi_cookie_t *this);

/**
 * Set the value of the cookie
 *
 * @param[in] this the target cookie
 * @param[in] value the value to set
 *
 * @return 0 on success, -1 on error
 */
typedef int (*cad_cgi_cookie_set_value_fn)(cad_cgi_cookie_t *this, const char *value);

/**
 * Get the expiration time of the cookie
 *
 * @param[in] this the target cookie
 *
 * @return the expiration time, 0 if not set
 */
typedef time_t (*cad_cgi_cookie_expires_fn)(cad_cgi_cookie_t *this);

/**
 * Set the expiration time of the cookie
 *
 * @param[in] this the target cookie
 * @param[in] expires the expiration date
 *
 * @return 0 on success, -1 on error
 */
typedef int (*cad_cgi_cookie_set_expires_fn)(cad_cgi_cookie_t *this, time_t expires);

/**
 * Get the max age of the cookie
 *
 * @param[in] this the target cookie
 *
 * @return the max age, 0 if not set
 */
typedef int (*cad_cgi_cookie_max_age_fn)(cad_cgi_cookie_t *this);

/**
 * Set the max age of the cookie
 *
 * @param[in] this the target cookie
 * @param[in] max_age the max age
 *
 * @return 0 on success, -1 on error
 */
typedef int (*cad_cgi_cookie_set_max_age_fn)(cad_cgi_cookie_t *this, int max_age);

/**
 * Get the domain of the cookie
 *
 * @param[in] this the target cookie
 *
 * @return the domain, NULL if not set
 */
typedef const char *(*cad_cgi_cookie_domain_fn)(cad_cgi_cookie_t *this);

/**
 * Set the domain of the cookie
 *
 * @param[in] this the target cookie
 * @param[in] domain the domain
 *
 * @return 0 on success, -1 on error
 */
typedef int (*cad_cgi_cookie_set_domain_fn)(cad_cgi_cookie_t *this, const char *domain);

/**
 * Get the path of the cookie
 *
 * @param[in] this the target cookie
 *
 * @return the path, NULL if not set
 */
typedef const char *(*cad_cgi_cookie_path_fn)(cad_cgi_cookie_t *this);

/**
 * Set the path of the cookie
 *
 * @param[in] this the target cookie
 * @param[in] path the path
 *
 * @return 0 on success, -1 on error
 */
typedef int (*cad_cgi_cookie_set_path_fn)(cad_cgi_cookie_t *this, const char *path);

typedef enum {
   Cookie_default = 0,
   Cookie_secure = 1,
   Cookie_http_only = 2,
} cad_cgi_cookie_flag_e;

/**
 * Get the flag of the cookie
 *
 * @param[in] this the target cookie
 *
 * @return the flag
 */
typedef cad_cgi_cookie_flag_e (*cad_cgi_cookie_flag_fn)(cad_cgi_cookie_t *this);

/**
 * Set the flag of the cookie
 *
 * @param[in] this the target cookie
 * @param[in] flag the flag
 *
 * @return 0 on success, -1 on error
 */
typedef int (*cad_cgi_cookie_set_flag_fn)(cad_cgi_cookie_t *this, cad_cgi_cookie_flag_e flag);

typedef struct cad_cgi_cookie {
   /**
    * @see free_fn
    */
   cad_cgi_cookie_free_fn free;
   /**
    * @see cad_cgi_cookie_name_fn
    */
   cad_cgi_cookie_name_fn name;
   /**
    * @see cad_cgi_cookie_value_fn
    */
   cad_cgi_cookie_value_fn value;
   /**
    * @see cad_cgi_cookie_set_value_fn
    */
   cad_cgi_cookie_set_value_fn set_value;
   /**
    * @see cad_cgi_cookie_expires_fn
    */
   cad_cgi_cookie_expires_fn expires;
   /**
    * @see cad_cgi_cookie_set_expires_fn
    */
   cad_cgi_cookie_set_expires_fn set_expires;
   /**
    * @see cad_cgi_cookie_max_age_fn
    */
   cad_cgi_cookie_max_age_fn max_age;
   /**
    * @see cad_cgi_cookie_set_max_age_fn
    */
   cad_cgi_cookie_set_max_age_fn set_max_age;
   /**
    * @see cad_cgi_cookie_domain_fn
    */
   cad_cgi_cookie_domain_fn domain;
   /**
    * @see cad_cgi_cookie_set_domain_fn
    */
   cad_cgi_cookie_set_domain_fn set_domain;
   /**
    * @see cad_cgi_cookie_path_fn
    */
   cad_cgi_cookie_path_fn path;
   /**
    * @see cad_cgi_cookie_set_path_fn
    */
   cad_cgi_cookie_set_path_fn set_path;
   /**
    * @see cad_cgi_cookie_flag_fn
    */
   cad_cgi_cookie_flag_fn flag;
   /**
    * @see cad_cgi_cookie_set_flag_fn
    */
   cad_cgi_cookie_set_flag_fn set_flag;
} cad_cgi_cookie_t;

/* ---------------------------------------------------------------- */

/**
 * Used by the CGI context to handle a request. The handle is not
 * expected to write directly on stdout; but use the provided
 * functions to prepare the response.
 *
 * @param[in] cgi the calling CGI context
 * @param[in] response the response to prepare
 *
 * @return 0 on success, anything else otherwise
 */
typedef int (*cad_cgi_handle_cb)(cad_cgi_t *cgi, cad_cgi_response_t *response);

/**
 * Creates a new CGI context using the provided handler.
 *
 * @param[in] memory the memory manager
 * @param[in] handler the CGI handler
 *
 * @return a new CGI context
 */
__PUBLIC__ cad_cgi_t *new_cad_cgi(cad_memory_t memory, cad_cgi_handle_cb handler);

/**
 * Creates a new cookie with the given name.
 *
 * @param[in] memory the memory manager
 * @param[in] name the cookie name
 *
 * @return a new cookie
 */
__PUBLIC__ cad_cgi_cookie_t *new_cad_cgi_cookie(cad_memory_t memory, const char *name);

// TODO: fastCGI

#endif /* _CAD_CGI_H_ */
