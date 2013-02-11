#ifndef _darray_h
#define _darray_h
#include <stdlib.h>
#include <assert.h>
#include "mem/halloc.h"

typedef struct darray_t {
    int end;
    int max;
    size_t element_size;
    size_t expand_rate;
    void **contents;
} darray_t;

darray_t *darray_create(size_t element_size, size_t initial_max);

void darray_destroy(darray_t *array);

void darray_clear(darray_t *array);

int darray_expand(darray_t *array);

int darray_contract(darray_t *array);

int darray_push(darray_t *array, void *el);

void *darray_pop(darray_t *array);

void darray_clear_destroy(darray_t *array);

#define darray_last(A) ((A)->contents[(A)->end - 1])
#define darray_first(A) ((A)->contents[0])
#define darray_end(A) ((A)->end)
#define darray_max(A) ((A)->max)

#define DEFAULT_EXPAND_RATE 300


static inline void darray_set(darray_t *array, int i, void *el)
{
    array->contents[i] = el;
}

static inline void *darray_get(darray_t *array, int i)
{
    return array->contents[i];
}

static inline void *darray_remove(darray_t *array, int i)
{
    void *el = array->contents[i];

    array->contents[i] = NULL;

    return el;
}

static inline void *darray_new(darray_t *array)
{
    return h_calloc(1, array->element_size);
}

#define darray_free(E) h_free((E))
#define darray_attach(A, E) hattach((E), (A))

#endif
