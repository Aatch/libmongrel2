#ifndef _REQUEST_H_DEF
#define _REQUEST_H_DEF

#include "bstring.h"

typedef struct m2_request {
    bstring sender;
    bstring path;
    bstring conn_id;
} m2_request_t;

#endif//_REQUEST_H_DEF
