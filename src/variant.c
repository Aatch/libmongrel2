#include "variant.h"
#include "bstring.h"
#include "adt/hash.h"
#include "adt/darray.h"
#include "err.h"
#include "json.h"

#include <stdio.h>

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

m2_variant_tag m2_variant_type(const void * val) {
    if (val) {
        return ((variant_t *)val)->type;
    } else {
        return m2_type_invalid;
    }
}

void m2_variant_destroy(void * val) {

    if (val) {
        variant_t * var = (variant_t *) val;

        switch(var->type) {
            case m2_type_string:
                bdestroy(var->value.string);
                break;
            case m2_type_dict:
                hash_free_nodes(var->value.dict);
                hash_destroy(var->value.dict);
                break;
            case m2_type_list:
                darray_clear_destroy(var->value.list);
                break;
            default:
                break;
        }

        h_free(val);
    }
}

static inline variant_t * variant_val_create(m2_variant_tag tag) {
    variant_t * val = NULL;
    val = (variant_t *)h_malloc(sizeof(*val));
    check_mem(val);

    val->type = tag;

    return val;
error:
    if (val) h_free(val);
    return NULL;
}

static hnode_t * hnode_alloc(void * unused) {
    (void)unused;

    return (hnode_t *)h_malloc(sizeof(hnode_t));
}

static void hnode_free(hnode_t * node, void * unused) {
    (void)unused;

    if (node) {
        bstring key = (bstring)hnode_getkey(node);
        if (key) {
            bdestroy(key);
        }
        void * data = hnode_get(node);
        if (data) {
            m2_variant_destroy(data);
        }
        h_free(node);
    }
}

void * m2_variant_string_new() {
    variant_t * val = variant_val_create(m2_type_string);
    return val;
}
void * m2_variant_integer_new() {
    return variant_val_create(m2_type_integer);
}
void * m2_variant_float_new() {
    return variant_val_create(m2_type_float);
}
void * m2_variant_bool_new() {
    return variant_val_create(m2_type_boolean);
}
void * m2_variant_null_new() {
    return variant_val_create(m2_type_null);
}

void * m2_variant_dict_new() {
    variant_t * val = NULL;
    val = variant_val_create(m2_type_dict);
    if (val) {
        val->value.dict = hash_create(HASHCOUNT_T_MAX, (hash_comp_t)bstrcmp, (hash_fun_t)bstr_hash_fun);
        hash_set_allocator(val->value.dict, hnode_alloc, hnode_free, NULL);
    }

    return val;
}

void * m2_variant_list_new() {
    variant_t * val = NULL;
    val = variant_val_create(m2_type_list);
    if (val) {
        val->value.list = darray_create(sizeof(variant_t), 32);
    }

    return val;
}

int m2_variant_dict_set(void * val, const_bstring key, void * item) {
    check(m2_variant_type(val) == m2_type_dict, "val is not a dictionary");

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
    check(m2_variant_type(val) == m2_type_dict, "val is not a dictionary");

    variant_t * dict = (variant_t *)val;

    return hash_lookup(dict->value.dict, key)->hash_data;

error:
    return NULL;
}

int m2_variant_list_append(void * val, void * item) {
    check(m2_variant_type(val) == m2_type_list, "val is not a list");

    variant_t * list = (variant_t *)val;

    darray_push(list->value.list, item);

    return 1;
error:
    return 0;
}

bstring m2_variant_get_string(void * value) {
    check(m2_variant_type(value) == m2_type_string, "Type is not a string");
    return ((variant_t *)value)->value.string;
error:
    return NULL;
}

/* TNetstrings implementation */
static inline variant_t * tns_parse_string(const char * data, size_t len) {
    variant_t * val = variant_val_create(m2_type_string);
    if (val)
        val->value.string = blk2bstr(data, len);

    return val;
}

static inline variant_t * tns_parse_integer(const char * data, size_t len, int base) {
    variant_t * val = variant_val_create(m2_type_integer);
    if (val) {
        char * end = NULL;
        val->value.integer = strtol( data, &end, base);

        check(end != NULL && (size_t)(end - data) == len, "Error parsing integer");
    }

    return val;

error:
    m2_variant_destroy(val);
    return NULL;
}

static inline variant_t * tns_parse_float(const char * data, size_t len) {
    variant_t * val = variant_val_create(m2_type_float);
    if (val) {
        char * end = NULL;
        val->value.fpoint = strtod(data, &end);

        check(end != NULL && (size_t)(end - data) == len, "Error parsing float");
    }

    return val;

error:
    m2_variant_destroy(val);
    return NULL;
}

static inline variant_t * tns_parse_bool(const char * data, size_t len) {
    const char * i = data;
    int d = 0;
    if (len == 4) {
        check(*(i++) != 't' || *(i++) != 'r' || *(i++) != 'u' || *(i++) != 'e', "Invalid bool value");
        d = 1;
    } else if (len == 5) {
        check(*(i++) != 'f' || *(i++) != 'a' || *(i++) != 'l' || *(i++) != 's' || *(i++) != 'e', "Invalid bool value");
    } else {
        check(0, "Invalid bool value");
    }

    variant_t * val = variant_val_create(m2_type_boolean);
    val->value.boolean = d;

    return val;

error:
    return NULL;
}

#define rotate_buffer(data,rest,len,orig_len) {\
    len = len - (rest - data);\
    check(len < orig_len, "Buffer math went wonky");\
    data = rest;\
}

static inline variant_t * tns_parse_dict(const char * data, size_t len) {

    variant_t * val = (variant_t *)m2_variant_dict_new();

    void * key = NULL;
    void * item = NULL;
    char * rest = NULL;
    size_t orig_len = len;

    while (len > 0) {
        key = m2_parse_tns(data, len, &rest);
        check(key, "Error parsing key");
        check(m2_variant_type(key) == m2_type_string, "key must be a string");
        rotate_buffer(data, rest, len, orig_len);

        item = m2_parse_tns(data, len, &rest);
        check(item, "Error parsing item");
        rotate_buffer(data, rest, len, orig_len);

        m2_variant_dict_set(val, ((variant_t *)key)->value.string, item);
        free(key);

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

static inline variant_t * tns_parse_list(const char * data, size_t len) {

    variant_t * val = (variant_t *)m2_variant_list_new();

    void * item = NULL;
    char * rest = NULL;
    size_t orig_len = len;

    while (len > 0) {
        item = m2_parse_tns(data, len, &rest);
        check(item, "Error parsing item");
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

    check(data, "Data cannot be NULL");
    check(len, "Len must be greater than 0");

    char * valstr = NULL;
    long vallen = strtol(data, &valstr, 10);

    //Checks!
    check(vallen >= 0, "Invalid size");
    check(end >= (valstr+(vallen)), "Parsed value (%ld) is greater than buffer size", vallen)
    check(valstr[0] == ':', "Invalid TNetstring, expected ':' got %c", valstr[0])
    valstr++;

    type = valstr[vallen];

    if (rest) {
        *rest = valstr + vallen + 1;
    }

    switch (type) {
        case m2_type_string:
            val = tns_parse_string(valstr, vallen);
            break;
        case m2_type_integer:
            val = tns_parse_integer(valstr, vallen, 10);
            break;
        case m2_type_float:
            val = tns_parse_float(valstr, vallen);
            break;
        case m2_type_boolean:
            val = tns_parse_bool(valstr, vallen);
            break;
        case m2_type_dict:
            val = tns_parse_dict(valstr, vallen);
            break;
        case m2_type_list:
            val = tns_parse_list(valstr, vallen);
            break;
        case m2_type_null:
            check(vallen == 0, "Null must be represented as '0:~'");
            val = variant_val_create(m2_type_null);
            break;
        case m2_type_invalid:
        default:
            check(0, "Invalid type");
    }


    return val;

error:
    return NULL;
}

static variant_t * json_val_to_variant(const json_value * jval) {
    variant_t * val = NULL;
    json_type t = jval->type;
    int i = 0;
    int len = 0;
    switch(t) {
        case json_array:
            val = m2_variant_list_new();
            len = jval->u.array.length;
            for (i = 0; i < len; i++) {
                m2_variant_list_append(val, json_val_to_variant(jval->u.array.values[i]));
            }
            break;
        case json_boolean:
            val = m2_variant_bool_new();
            val->value.boolean = jval->u.boolean;
            break;
        case json_double:
            val = m2_variant_float_new();
            val->value.fpoint = jval->u.dbl;
            break;
        case json_integer:
            val = m2_variant_integer_new();
            val->value.integer = jval->u.integer;
            break;
        case json_none:
        case json_null:
            val = m2_variant_null_new();
            break;
        case json_object:
            val = m2_variant_dict_new();
            len = jval->u.object.length;
            for (i = 0; i < len; i++) {
                m2_variant_dict_set(val, bfromcstr(jval->u.object.values[i].name),
                        json_val_to_variant(jval->u.object.values[i].value));
            }
            break;
        case json_string:
            val = m2_variant_string_new();
            val->value.string = bfromcstr(jval->u.string.ptr);
            break;
    }

    return val;
}

void * m2_parse_json(const char * str) {
    json_value * val = json_parse(str);

    void * v = (void *)json_val_to_variant(val);

    json_value_free(val);

    return v;
}

void m2_variant_dump_json(void * val) {
    variant_t * var = (variant_t *)val;
    int first = 0;
    switch(var->type) {
        case m2_type_dict:
            printf("{ ");
            hscan_t scan;
            hnode_t * n;
            hash_scan_begin(&scan, var->value.dict);
            while ((n = hash_scan_next(&scan))) {
                if (first) {
                    printf(",\n");
                }
                first = 1;
                printf("\"%s\":", ((bstring)n->hash_key)->data);
                m2_variant_dump_json(n->hash_data);
            }

            printf(" }");
            break;
        case m2_type_boolean:
            if (var->value.boolean)
                printf("true");
            else
                printf("false");
            break;
        case m2_type_float:
            printf("%e", var->value.fpoint);
            break;
        case m2_type_integer:
            printf("%ld", var->value.integer);
            break;
        case m2_type_list:
            printf("[ ");

            int i = 0;
            for (i = 0; i < var->value.list->end; i++) {
                if (first)
                    printf(",\n");
                first = 1;
                m2_variant_dump_json(darray_get(var->value.list, i));
            }
            break;
        case m2_type_null:
            printf("null");
            break;
        case m2_type_string:
            printf("\"%s\"", var->value.string->data);
            break;
        default:
            break;
    }
}
