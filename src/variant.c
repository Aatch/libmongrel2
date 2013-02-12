#include "variant.h"
#include "bstring.h"
#include "adt/hash.h"
#include "adt/darray.h"

#define check(A) if (!(A)) { goto error; }

typedef struct {
    m2_variant_tag type;
    union {
        bstring string;
        long integer;
        double fpoint;
        int boolean;
        hash_t * dict;
        darray_t * list;
    } value;
} variant_t;

m2_variant_tag m2_variant_get_type(const void * val) {
    return ((variant_t *)val)->type;
}

void m2_variant_destroy(void * val) {

    variant_t * var = (variant_t *) val;

    switch(var->type) {
        case m2_type_string:
            bdestroy(var->value.string);
            break;
        case m2_type_dict:
            hash_free_nodes(var->value.dict);
            hash_destroy(var->value.dict);
        case m2_type_list:
            darray_clear_destroy(var->value.list);
        default:
            break;
    }

    free(val);
}

static inline variant_t * variant_val_create(m2_variant_tag tag) {
    variant_t * val = (variant_t *)malloc(sizeof(*val));

    val->type = tag;

    return val;
}

static hnode_t * hnode_alloc(void * unused) {
    (void)unused;

    return (hnode_t *)malloc(sizeof(hnode_t));
}

static void hnode_free(hnode_t * node, void * unused) {
    (void)unused;

    bdestroy((bstring)hnode_getkey(node));
    m2_variant_destroy(hnode_get(node));
    free(node);
}

void * m2_variant_dict_new() {
    variant_t * val = variant_val_create(m2_type_dict);
    val->value.dict = hash_create(HASHCOUNT_T_MAX, (hash_comp_t)bstrcmp, (hash_fun_t)bstr_hash_fun);
    hash_set_allocator(val->value.dict, hnode_alloc, hnode_free, NULL);

    return val;
}

int m2_variant_dict_set(void * val, const_bstring key, void * item) {
    check(m2_variant_get_type(val) == m2_type_dict);

    variant_t * dict = (variant_t *)val;

    hnode_t * node = NULL;
    node = hash_lookup(dict->value.dict, key);
    if (node)
        node->hash_data = item;
    else
        hash_alloc_insert(dict->value.dict, key, item);

    return 1;

error:
    return 0;
}

void * m2_variant_dict_get(const void * val, const_bstring key) {
    check(m2_variant_get_type(val) == m2_type_dict);

    variant_t * dict = (variant_t *)val;

    return hash_lookup(dict->value.dict);
}

int m2_variant_list_append(void * val, void * item) {
    check(m2_variant_get_type(val) == m2_type_list);

    variant_t * list = (variant_t *)val;

    darray_push(list->value.list, item);

    return 1;
error:
    return 0;
}

static inline variant_t * parse_string(const char * data, size_t len) {
    variant_t * val = variant_val_create(m2_type_string);
    val->value.string = blk2bstr(data, len);

    return val;
}

static inline variant_t * parse_integer(const char * data, size_t len) {
    variant_t * val = variant_val_create(m2_type_integer);
    char * end = NULL;
    val->value.integer = strtol( data, &end, 10);

    check(end != NULL && (end - data) == len);

    return val;

error:
    m2_variant_destroy(val);
    return NULL;
}

static inline variant_t * parse_float(const char * data, size_t len) {
    variant_t * val = variant_val_create(m2_type_float);
    char * end = NULL;
    val->value.fpoint = strtod(data, &end);

    check(end != NULL && (end - data) == len);

    return val;

error:
    m2_variant_destroy(val);
    return NULL;
}

static inline variant_t * parse_bool(const char * data, size_t len) {
    const char * i = data;
    int d = 0;
    if (len == 4) {
        check(*(i++) != 't' || *(i++) != 'r' || *(i++) != 'u' || *(i++) != 'e');
        d = 1;
    } else if (len == 5) {
        check(*(i++) != 'f' || *(i++) != 'a' || *(i++) != 'l' || *(i++) != 's' || *(i++) != 'e');
    } else {
        check(0);
    }

    variant_t * val = variant_val_create(m2_type_boolean);
    val->value.boolean = d;

    return val;

error:
    return NULL;
}

#define rotate_buffer(data,rest,len,orig_len) {\
    len = len - (rest - data);\
    check(len < orig_len);\
    data = rest;\
}

static inline variant_t * parse_dict(const char * data, size_t len) {

    variant_t * val = (variant_t *)m2_variant_dict_new();

    void * key = NULL;
    void * item = NULL;
    char * rest = NULL;
    size_t orig_len = len;

    while (len > 0) {
        key = m2_parse_tns(data, len, &rest);
        check(key);
        check(m2_variant_get_type(key) == m2_type_string);
        rotate_buffer(data, rest, len, orig_len);

        item = m2_parse_tns(data, len, &rest);
        check(item);
        rotate_buffer(data, rest, len, orig_len);

        m2_variant_dict_insert(val, ((variant_t *)key)->value.string, item);

        key = NULL;
        item = NULL;
    }

    return val;

error:
    if (key) m2_variant_destroy(key);
    if (item) m2_variant_destroy(item);
    m2_variant_destroy(val);
    return NULL;
}

static inline variant_t * parse_list(const char * data, size_t len) {

    variant_t * val = (variant_t *)m2_variant_list_new();

    void * item = NULL;
    char * rest = NULL;
    size_t orig_len = len;

    while (len > 0) {
        item = m2_parse_tns(data, len, &rest);
        check(item);
        rotate_buffer(data, rest, len, orig_len);

        m2_variant_list_append(val, item);

        item = NULL;
    }

    return val;

error:
    if (item) m2_variant_destroy(item);
    m2_variant_destroy(val);

    return NULL;
}
void * m2_parse_tns(const char * data,
        size_t len, char ** rest) {

    void * val = NULL;
    const char * end = data+len;
    m2_variant_tag type = m2_type_null;

    check(data && len);

    char * valstr = NULL;
    long vallen = strtol(data, &valstr, 10);

    //Checks!
    check(vallen >= 0);
    check(end >= (valstr+(vallen+1)))
    check(valstr[0] == ':')
    valstr++;

    type = valstr[vallen];

    if (rest) {
        *rest = valstr + vallen + 1;
    }

    switch (type) {
        case m2_type_string:
            val = parse_string(valstr, vallen);
            break;
        case m2_type_integer:
            val = parse_integer(valstr, vallen);
            break;
        case m2_type_float:
            val = parse_float(valstr, vallen);
            break;
        case m2_type_boolean:
            val = parse_bool(valstr, vallen);
            break;
        case m2_type_dict:
            val = parse_dict(valstr, vallen);
            break;
        case m2_type_list:
            val = parse_list(valstr, vallen);
            break;
        case m2_type_null:
            check(vallen == 0);
            val = variant_val_create(m2_type_null);
            break;
    }


    return val;

error:
    return NULL;
}
