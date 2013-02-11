/**
 * @file mongrel2.h
 *
 * \brief A handler library for mongrel2.
 *
 * Follows a combination of ZeroMQ's and Mongrel2's
 * code style in order to present a minimal, simple
 * interface.
 */

#ifndef _MONGREL2_H_DEF
#define _MONGREL2_H_DEF

#include "bstring.h"

/**
 * Creates a new context for using this library.
 *
 * The context is thread-safe and can be passed to
 * threads.
 *
 * @returns A context for the library
 */
void * m2_ctx_new();

/**
 * Destroys a library context.
 *
 * All open connections should be closed first.
 *
 * @param ctx   The context to destroy
 */
void m2_ctx_destroy(void * ctx);

/**
 * Opens a new connection to a Mongrel2 instance.
 *
 * Connections are not thread safe and should be opened, used
 * and closed in the same thread.
 *
 * @param   ctx         A context created with m2_ctx_new()
 * @param   uuid        A unique id to identify this connection.
 *                      Can be set to NULL.
 * @param   recv_addr   The address to connect to, to receive
 *                      requests.
 * @param   send_addr   The address to connect to, to send replies.
 *
 * @returns A pointer to a connection. NULL if there was an error
 */
void * m2_connection_open(void * ctx, const_bstring uuid,
        const_bstring recv_addr,
        const_bstring send_addr);

/**
 * Closes the connection.
 *
 * Any pending messages will cause the connection
 * to block until they are sent or received.
 *
 * Always succeeds.
 *
 * @param   conn    An open connection.
 */
void m2_connection_close(void * conn);

/**
 * Request object
 */
typedef struct {
    /// The connection this request came in on
    void * conn;
    /// The raw data from the request
    struct {
        int len; /// The length of the data
        void * data; /// A pointer to the data
    } raw;
    /// The UUID from the Mongrel2 instance
    bstring uuid;
    /// The conn_id identifying the browser
    bstring conn_id;
    /// The path used to match
    bstring path;
    /// The headers from the request
    void * headers;
    /// The body of the request, NULL if there is no body
    bstring body;
} m2_request_t;

/**
 * Receives a request from the connection.
 *
 * @param   conn    The connection to receive on.
 *
 * @returns A request_t object representing the request.
 */
m2_request_t * m2_recv(void * conn);

/**
 * Frees a request.
 *
 * @param req   The request to free
 */
void m2_request_free(m2_request_t * req);

/**
 * Gets the header from the request, \a req by \a name.
 *
 * @param req       The request
 * @param name      The name of the header
 *
 * @returns a tnetstring value or NULL on error.
 *          The tnetstring can have its type tested with
 *          m2_tns_type() and the typed value
 *          can be extracted using \link m2_tns_get_string()
 *          m2_tns_get_* \endlink functions.
 *
 */
void * m2_request_get_header(const m2_request_t * req, const_bstring name);

/**
 * Sends a reply on the connection using the given values.
 *
 * @param conn     The connection to send on
 * @param uuid     The UUID of the sender
 * @param conn_id  The conn_id of the client. Can also be a list of
 *                 ids seperated by a space
 * @param msg      The message to send
 *
 * @returns The number of bytes sent or -1 on error
 */
int m2_send(void * conn, const_bstring uuid, const_bstring conn_id, const_bstring msg);

/**
 * Replies to the request with the given message.
 *
 * @param req   The request to reply to
 * @param msg   The message to send
 */
int m2_reply(const m2_request_t * req, const_bstring msg);

/// Possible TNetstring types
typedef enum m2_tns_type_e {
    M2_TNS_STRING   = ',',
    M2_TNS_NUMBER   = '#',
    M2_TNS_FLOAT    = '^',
    M2_TNS_BOOL     = '!',
    M2_TNS_NULL     = '~',
    M2_TNS_DICT     = '}',
    M2_TNS_LIST     = ']',
    M2_TNS_INVALID  = 'Z',
} m2_tns_type_tag;

/**
 * Gets the type of \a val.
 *
 * @param val   A TNetstring value
 *
 * @returns the type of the value, or m2_tns_type_tag#M2_TNS_INVALID if \a val is not a
 *          valid TNetstring
 */
m2_tns_type_tag m2_tns_type(const void * val);

/**
 * Get the value stored in \a val.
 *
 * @param val   A TNetstring value
 * @returns The value. NULL on error.
 */
bstring m2_tns_get_string(const void * val);
/**
 * Get the value stored in \a val.
 *
 * @param val   A TNetstring value
 * @returns The value. 0 on error and sets errno.
 */
long    m2_tns_get_number(const void * val);
/**
 * Get the value stored in \a val.
 *
 * @param val   A TNetstring value
 * @returns The value. 0 on error and sets errno.
 */
double  m2_tns_get_float (const void * val);
/**
 * Get the value stored in \a val.
 *
 * @param val   A TNetstring value
 * @returns The value. 0 on error and sets errno.
 */
int     m2_tns_get_bool  (const void * val);
/**
 * Get the value stored in \a val.
 *
 * @param val   A TNetstring value
 * @returns The value. NULL on error.
 */
void *  m2_tns_get_dict  (const void * val);
/**
 * Get the value stored in \a val.
 *
 * @param val   A TNetstring value
 * @returns The value. NULL on error.
 */
void *  m2_tns_get_list  (const void * val);

#endif//_MONGREL2_H_DEF
