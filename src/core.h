#ifndef _CORE_H_DEF
#define _CORE_H_DEF

typedef struct m2_context {
    void * zmq_ctx;
} m2_context_t;

/**
 * Initializes the library.
 */
void m2_init();

/**
 * Deitilialises the library.
 *
 * Does nothing if it has already been deinitialised
 *
 * Doesn't close any connections made however.
 */
void m2_deinit();

/**
 * Creates a new context, passed to thread-safe functions
 * functions
 */
m2_context_t * m2_ctx_new();

/**
 * Destroys a context. Doesn't close any connections.
 */
void m2_ctx_destroy(m2_context_t * ctx);



/**
 * Gets the ZeroMQ context for this m2 context
 */
void * m2_get_zmq_ctx(m2_context_t * ctx);

#endif
