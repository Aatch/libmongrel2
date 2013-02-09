#ifndef _TNETSTRING_H_DEF
#define _TNETSTRING_H_DEF

#include "bstring.h"
#include "adt/hash.h"
#include "adt/darray.h"

enum m2_tns_type {
    M2_TNS_STR    = ',',
    M2_TNS_INT    = '#',
    M2_TNS_DICT   = '}',
    M2_TNS_LIST   = ']',
    M2_TNS_BOOL   = '!',
    M2_TNS_FLOAT  = '^',
    M2_TNS_NULL   = '~',
    M2_TNS_INVALID= 'Z',
};

typedef struct m2_tns_val {
    m2_tns_type type;
    union {
        bstring string;
        long number;
        hash_t * dict;
        darray_t *list;
        int boolean;
        double fpoint;
    } value;
} m2_tns_val_t;


#define m2_tns_get_type(T) (((m2_tns_val_t *)(T)) == NULL ? M2_TNS_INVALID : ((m2_tns_val_t *)(T))->type)

#endif//_TNETSTRING_H_DEF
