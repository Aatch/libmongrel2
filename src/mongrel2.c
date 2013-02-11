#include <zmq.h>

#include "adt/hash.h"
#include "adt/darray.h"
#include "mem/halloc.h"
#include "tnetstrings.h"

#include "mongrel2.h"

typedef struct ctx {
    void * zmq_ctx;
} ctx_t;

typedef struct conn {
    void * recv_sock;
    void * send_sock;
    const_bstring uuid;
    const_bstring recv_addr;
    const_bstring send_addr;
} conn_t;

void * m2_ctx_new() {
    ctx_t * ctx = h_malloc(sizeof(*ctx));
    ctx->zmq_ctx = zmq_ctx_new();

    return (void *)ctx;
}

void m2_ctx_destroy(void * ctx) {
    h_free(ctx);
}

void * m2_connection_open(void * ctx, const_bstring uuid,
        const_bstring recv_addr,
        const_bstring send_addr) {

    ctx_t * context = (ctx_t *)ctx;
    conn_t * conn = h_malloc(sizeof(*conn));
    hattach(conn, ctx);

    conn->uuid = uuid;
    conn->recv_addr = recv_addr;
    conn->send_addr = send_addr;

    void * recv_sock = zmq_socket(context->zmq_ctx, ZMQ_PULL);
    if (uuid) {
        zmq_setsockopt(recv_sock, ZMQ_IDENTITY, bdata(uuid), blength(uuid));
    }

    void * send_sock = zmq_socket(context->zmq_ctx, ZMQ_PUB);

    zmq_connect(recv_sock, bdata(recv_addr));
    zmq_connect(send_sock, bdata(send_addr));

    conn->recv_sock = recv_sock;
    conn->send_sock = send_sock;

    return (void *)conn;
}

void m2_connection_close(void * conn) {
    conn_t * connection = (conn_t *)conn;

    zmq_close(connection->send_sock);
    zmq_close(connection->recv_sock);

    h_free(conn);
}

#define BUFFER_SIZE 256 * 1024 //256k per request? eh.
m2_request_t * m2_recv(void * conn) {

    m2_request_t * req = h_malloc(sizeof(*req));
    hattach(req, conn);

    void * raw = h_malloc(BUFFER_SIZE);
    hattach(conn, raw);

    conn_t * connection = (void *)conn;

    req->conn = conn;

    int msglen = zmq_recv(connection->recv_sock, raw, BUFFER_SIZE, 0);

    req->raw.len = msglen;
    req->raw.data = raw;

    // parse data;

    return req;
}

void m2_request_free(m2_request_t * req) {
    tns_value_t * tns = (tns_value_t *)req->headers;
    tns_value_destroy(tns);

    bdestroy(req->body);
    bdestroy(req->conn_id);
    bdestroy(req->path);
    bdestroy(req->uuid);

    h_free(req);
}

void * m2_request_get_header(const m2_request_t * req, const_bstring name) {
    hash_t * headers = (hash_t *)req->headers;

    hnode_t * node = hash_lookup(headers, name);

    if (node) {
        return node->hash_data;
    }
    return NULL;
}

m2_tns_type_tag m2_tns_type(const void * val) {
    tns_value_t * tns = (tns_value_t *)val;
    if (tns) {
        switch((m2_tns_type_tag) tns->type) {
            case M2_TNS_STRING:
                return M2_TNS_STRING;
            case M2_TNS_NUMBER:
                return M2_TNS_NUMBER;
            case M2_TNS_FLOAT:
                return M2_TNS_FLOAT;
            case M2_TNS_BOOL:
                return M2_TNS_BOOL;
            case M2_TNS_NULL:
                return M2_TNS_NULL;
            case M2_TNS_DICT:
                return M2_TNS_DICT;
            case M2_TNS_LIST:
                return M2_TNS_LIST;
            default:
                return M2_TNS_INVALID;
        }
    }

    return M2_TNS_INVALID;
}

#define to_type(val, type, field) (m2_tns_type(val) == type ? ((tns_value_t *)val)->value.field : 0)

bstring m2_tns_get_string(const void * val){
    return to_type(val, M2_TNS_STRING, string);
}
long    m2_tns_get_number(const void * val){
    return to_type(val, M2_TNS_NUMBER, number);
}
double  m2_tns_get_float (const void * val){
    return to_type(val, M2_TNS_FLOAT, fpoint);
}
int     m2_tns_get_bool  (const void * val){
    return to_type(val, M2_TNS_BOOL, boolean);
}
void *  m2_tns_get_dict  (const void * val){
    return to_type(val, M2_TNS_DICT, dict);
}
void *  m2_tns_get_list  (const void * val){
    return to_type(val, M2_TNS_LIST, list);
}


