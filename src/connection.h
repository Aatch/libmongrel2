#ifndef _CONNECTION_H_DEF
#define _CONNECTION_H_DEF

#include <zmq.h>
#include "bstring.h"
#include "core.h"
#include "request.h"

typedef struct m2_connection {
    const_bstring sender_id;
    const_bstring sub_addr;
    const_bstring pub_addr;
    void * req_sock;
    void * res_sock;
} m2_connection_t;

m2_connection_t *m2_connection_new(const_bstring sender_id,
        const_bstring sub_addr,
        const_bstring pub_addr);

m2_connection_t *m2_connection_new_r(m2_context_t * ctx,
        const_bstring sender_id,
        const_bstring sub_addr,
        const_bstring pub_addr);

void m2_connection_destroy(m2_connection_t * connection);

m2_request_t * m2_connection_recv(const m2_connection_t * connection);

int m2_connection_send(const_bstring uuid, const_bstring conn_id, const_bstring msg);
int m2_connection_reply(const m2_request_t * req, const_bstring msg);

int m2_connection_deliver(const m2_request_t * req, const bstrList * idents, const_bstring msg);

#endif//_CONNECTION_H_DEF
