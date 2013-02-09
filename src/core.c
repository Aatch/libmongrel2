#include <zmq.h>
#include <stdlib.h>

#include "core.h"
#include "dbg.h"

m2_context_t * global_ctx;

void m2_init() {
    check(!(global_ctx), "m2_init has already been called");

    global_ctx = m2_ctx_new();
error:
    return;
}

void m2_deinit() {
    if (global_ctx) {
        m2_ctx_destroy(global_ctx);
    }
}

m2_context_t * m2_ctx_new() {
    void * zmq_ctx = zmq_ctx_new();
    check(zmq_ctx, "Error creating new ZeroMQ context");

    m2_context_t * ctx = (m2_context_t *)malloc(sizeof(m2_context_t));
    check(ctx, "Error allocating memory for context");

    ctx->zmq_ctx = zmq_ctx;

    return ctx;

error:
    if (zmq_ctx)
        zmq_ctx_destroy(zmq_ctx);

    return NULL;
}

void m2_ctx_destroy(m2_context_t * ctx) {
    if (ctx) {
        zmq_ctx_destroy(ctx->zmq_ctx);
        free(ctx);
    }
}

void * m2_get_zmq_ctx(m2_context_t * ctx) {
    check(ctx, "Invalid context given");

    return ctx->zmq_ctx;

error:
    return NULL;
}
