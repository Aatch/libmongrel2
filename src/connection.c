#include "connection.h"
#include "dbg.h"
#include <stdlib.h>

extern m2_context_t * global_ctx;


m2_connection_t *m2_connection_new(const_bstring sender_id,
        const_bstring sub_addr,
        const_bstring pub_addr) {

    check(global_ctx, "m2_init() not called, or re-entrant version intended");

    return m2_connection_new_r(global_ctx, sender_id, sub_addr, pub_addr);

error:
    return NULL;
}

m2_connection_t *m2_connection_new_r(m2_context_t * ctx,
        const_bstring sender_id,
        const_bstring sub_addr,
        const_bstring pub_addr) {

    void * zctx, * req_sock, * res_sock;
    zctx = req_sock = res_sock = 0;

    m2_connection_t * conn = 0;

    zctx = m2_get_zmq_ctx(ctx);
    check(zctx, "Error getting ZeroMQ context");

    req_sock = zmq_socket(zctx, ZMQ_PULL);
    check(req_sock, "Error creating request socket");

    res_sock = zmq_socket(zctx, ZMQ_PUB);
    check(res_sock, "Error creating response socket");

    if (sender_id) {
        check(zmq_setsockopt(res_sock, ZMQ_IDENTITY, bdata(sender_id), blength(sender_id)) >= 0,
                "Error setting response identity (`%s')", bdata(sender_id));
    }

    check(zmq_connect(req_sock, bdata(sub_addr)) >= 0, "Error connecting to Mongrel2 PUSH socket at `%s'", bdata(sub_addr));
    check(zmq_connect(res_sock, bdata(pub_addr)) >= 0, "Error connecting to Mongrel2 SUB socket at `%s'", bdata(sub_addr));

    conn = (m2_connection_t *)malloc(sizeof(m2_connection_t));
    check(conn, "Error allocating memory for connection");

    conn->sender_id = sender_id;
    conn->sub_addr = sub_addr;
    conn->pub_addr = pub_addr;
    conn->req_sock = req_sock;
    conn->res_sock = res_sock;

    return conn;

error:
    // Close them if they were opened
    if (req_sock)
        zmq_close(req_sock);
    if (res_sock)
        zmq_close(res_sock);

    return NULL;
}

/**
 * Frees the data associated with the connection and closes
 * the sockets.
 */
void m2_connection_destroy(m2_connection_t * conn) {
    zmq_close(conn->req_sock);
    zmq_close(conn->res_sock);

    free(conn);
}
