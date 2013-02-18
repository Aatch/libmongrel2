#include <zmq.h>

#include "adt/hash.h"
#include "adt/darray.h"
#include "mem/halloc.h"
#include "variant.h"
#include "err.h"

#include "mongrel2.h"

static int parse_request(m2_request_t * req, void * raw, int len);

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
    check_mem(ctx);
    memset(ctx, 0, sizeof(*ctx));
    ctx->zmq_ctx = zmq_ctx_new();
    check(ctx->zmq_ctx, "Error creating 0MQ context");

    return (void *)ctx;

error:
    m2_ctx_destroy(ctx);
    return NULL;
}

void m2_ctx_destroy(void * ctx) {
    if (ctx) {
        zmq_ctx_destroy(((ctx_t *)ctx)->zmq_ctx);
        h_free(ctx);
    }
}

void * m2_connection_open(void * ctx, const_bstring uuid,
        const_bstring recv_addr,
        const_bstring send_addr) {

    conn_t * conn = NULL;
    void * recv_sock = NULL;
    void * send_sock = NULL;

    check(ctx, "Invalid context");
    check(uuid, "uuid not valid");
    check(recv_addr, "recv_addr not valid");
    check(send_addr, "send_addr not valid");

    ctx_t * context = (ctx_t *)ctx;

    conn = h_malloc(sizeof(*conn));
    check_mem(conn);
    hattach(conn, ctx);

    conn->uuid = uuid;
    conn->recv_addr = recv_addr;
    conn->send_addr = send_addr;

    recv_sock = zmq_socket(context->zmq_ctx, ZMQ_PULL);
    check(recv_sock, "Error creating receiving socket")
    if (uuid) {
        zmq_setsockopt(recv_sock, ZMQ_IDENTITY, bdata(uuid), blength(uuid));
    }

    send_sock = zmq_socket(context->zmq_ctx, ZMQ_PUB);
    check(recv_sock, "Error creating sending socket")

    check(zmq_connect(recv_sock, bdata(recv_addr)) == 0, "Error connecting receive socket to `%s'", bdata(recv_addr));
    check(zmq_connect(send_sock, bdata(send_addr)) == 0, "Error connecting send socket to `%s'", bdata(send_addr));

    conn->recv_sock = recv_sock;
    conn->send_sock = send_sock;

    return (void *)conn;

error:
    if (conn) h_free(conn);
    if (recv_sock) zmq_close(recv_sock);
    if (send_sock) zmq_close(send_sock);

    return NULL;
}

void m2_connection_close(void * conn) {
    if (conn) {
        conn_t * connection = (conn_t *)conn;

        zmq_close(connection->send_sock);
        zmq_close(connection->recv_sock);

        h_free(conn);
    }
}

#define BUFFER_SIZE 4 * 1024 //4k per request
m2_request_t * m2_recv(void * conn) {

    m2_request_t * req = NULL;
    void * raw = NULL;

    check(conn, "Not valid connection");

    req = h_malloc(sizeof(*req));
    check_mem(req);
    hattach(req, conn);

    raw = h_malloc(BUFFER_SIZE);
    check_mem(raw);
    hattach(conn, raw);

    conn_t * connection = (void *)conn;

    req->conn = conn;

    int msglen = zmq_recv(connection->recv_sock, raw, BUFFER_SIZE, 0);
    check(msglen >= 0, "Error recieving request");

    req->raw.len = msglen;
    req->raw.data = raw;

    parse_request(req, raw, msglen);
    if (!req) goto error;
    return req;

error:
    if (req) h_free(req);
    if (raw) h_free(raw);

    return NULL;
}

static int parse_request(m2_request_t * req, void * raw, int msglen) {

    unsigned char * data = (unsigned char *)raw;

    struct tagbstring * strings = NULL;
    void * headers = NULL;
    void * body = NULL;

    bstring uuid, conn_id, path;
    strings =
        (struct tagbstring *)malloc(sizeof(struct tagbstring)*3);
    check_mem(strings);

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

    check(!(p == data || p == pe), "Pointer madness!");

    uuid->data[uuid->slen] = '\0';

    conn_id->data = p;
    conn_id->slen = 0;
    conn_id->mlen = -1;
    while (*(p++) != ' ' && p != pe) {
        ++conn_id->slen;
    }

    check(p < pe, "Pointer madness");

    conn_id->data[conn_id->slen] = '\0';

    path->data = p;
    path->slen = 0;
    path->mlen = -1;

    while(*(p++) != ' ' && p != pe) {
        ++path->slen;
    }

    check(p != pe, "Pointer madness");

    path->data[path->slen] = '\0';

    size_t len = (pe - p);

    char * rest;
    char err[1024];

    headers = m2_parse_tns((const char *)p,len,&rest);
    check(headers, "Error parsing request headers: (%s)", m2_strerror_cpy(err));

    len = ((char *)pe - rest);

    if (len > 0) {
        body = m2_parse_tns((const char *)rest, len, NULL);
        check(body, "Error parsing request body: (%s)", m2_strerror_cpy(err));

        if (body) {
            req->body = m2_variant_get_string(body);
        }
    }

    req->conn_id = conn_id;
    req->headers = headers;
    req->path = path;
    req->uuid = uuid;

    return 1;

error:
    if (strings) free(strings);
    if (headers) m2_variant_destroy(headers);
    if (body) m2_variant_destroy(body);

    return 0;
}

void m2_request_free(m2_request_t * req) {

    if (req) {
        m2_variant_destroy(req->headers);
        bdestroy(req->body);
        bdestroy(req->conn_id);
        bdestroy(req->path);
        bdestroy(req->uuid);

        h_free(req);
    }
}

void * m2_request_get_header(const m2_request_t * req, const_bstring name) {
    return m2_variant_dict_get(req->headers, name);
}

int m2_send(void * conn, const_bstring uuid, const_bstring conn_id, const_bstring msg) {

    bstring header = NULL;

    check(conn, "Invalid connection");
    check(uuid, "Invalid uuid");
    check(conn_id, "Invalid connection id");
    check(msg, "Invalid message");

    conn_t * connection = (conn_t *)conn;

    header = bformat("%s %d:%s, ", uuid->data, conn_id->slen, conn_id->data);
    check(header, "Error formatting response header");

    if (msg)
        bconcat(header, msg);

    int n = zmq_send(connection->send_sock, header->data, header->slen, 0);
    bdestroy(header);

    check(n >= 0, "Error sending message");

    return n;

error:
    if (header) bdestroy(header);

    return -1;
}

int m2_reply(const m2_request_t * req, const_bstring msg) {
    if (req)
        return m2_send(req->conn, req->uuid, req->conn_id, msg);
    return 0;
}

