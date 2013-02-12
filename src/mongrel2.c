#include <zmq.h>

#include "adt/hash.h"
#include "adt/darray.h"
#include "mem/halloc.h"
#include "variant.h"

#include "mongrel2.h"

static void parse_request(m2_request_t * req, void * raw, int len);

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

    parse_request(req, raw, msglen);

    return req;
}

static void parse_request(m2_request_t * req, void * raw, int msglen) {

    unsigned char * data = (unsigned char *)raw;

    bstring uuid, conn_id, path, header, body;
    struct tagbstring * strings =
        (struct tagbstring *)malloc(sizeof(struct tagbstring)*5);

    uuid    = &strings[0];
    conn_id = &strings[1];
    path    = &strings[2];

    // Set up the marker pointers
    unsigned char * p = data;
    unsigned char * pe = data+msglen;

    uuid->data = data;
    uuid->slen = 0;
    uuid->mlen = -1;

    while (*(p++) != ' ' && p != pe) {
        ++uuid->slen;
    }

    if (p == data || p == pe) return;

    uuid->data[uuid->slen] = '\0';

    conn_id->data = p;
    conn_id->slen = 0;
    conn_id->mlen = -1;
    while (*(p++) != ' ' && p != pe) {
        ++conn_id->slen;
    }

    if (p == pe) return;

    conn_id->data[conn_id->slen] = '\0';

    path->data = p;
    path->slen = 0;
    path->mlen = -1;

    while(*(p++) != ' ' && p != pe) {
        ++path->slen;
    }

    if (p == pe) return;

    path->data[path->slen] = '\0';

    size_t len = (pe - p);

    char * rest;

    void * headers = m2_parse_tns((const char *)p,len,&rest);

    if (!headers) return;

    len = (rest - (char *)pe);

    if (len > 0) {
        void * body = m2_parse_tns((const char *)rest, len, NULL);

        if (body) {
            req->body = m2_variant_get_string(body);
        }
    }

    req->conn_id = conn_id;
    req->headers = headers;
    req->path = path;
    req->uuid = uuid;
}

void m2_request_free(m2_request_t * req) {

    m2_variant_destroy(req->headers);
    bdestroy(req->body);
    bdestroy(req->conn_id);
    bdestroy(req->path);
    bdestroy(req->uuid);

    h_free(req);
}

void * m2_request_get_header(const m2_request_t * req, const_bstring name) {
    return m2_variant_dict_get(req->headers, name);
}

int m2_send(void * conn, const_bstring uuid, const_bstring conn_id, const_bstring msg) {
    conn_t * connection = (conn_t *)conn;

    bstring header = bformat("%s %d:%s, ", uuid->data, conn_id->slen, conn_id->data);

    if (msg)
        bconcat(header, msg);

    int n = zmq_send(connection->send_sock, header->data, header->slen, 0);

    bdestroy(header);

    return n;
}

int m2_reply(const m2_request_t * req, const_bstring msg) {
    if (req)
        return m2_send(req->conn, req->uuid, req->conn_id, msg);
    return 0;
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


