/**
 * @file variant.h
 *
 * Variant types. Uses TNetstrings as a base
 * for available types.
 *
 * Needed to transparently handle both the
 * TNetstrings and JSON protocols.
 *
 * Also contains methods for parsing and dumping
 * both TNetstrings and JSON.
 *
 */
#ifndef _VARIANT_H_DEF
#define _VARIANT_H_DEF

#include <stdlib.h>
#include "bstring.h"

typedef enum {
    m2_type_string  = ',',
    m2_type_integer = '#',
    m2_type_float   = '^',
    m2_type_boolean = '!',
    m2_type_null    = '~',
    m2_type_dict    = '}',
    m2_type_list    = ']',
    m2_type_invalid = 'Z',
} m2_variant_tag;

typedef struct variant_s variant_t;

extern inline m2_variant_tag m2_variant_type(const variant_t * val);

variant_t * m2_variant_string_new();
variant_t * m2_variant_integer_new();
variant_t * m2_variant_float_new();
variant_t * m2_variant_bool_new();
variant_t * m2_variant_null_new();
/**
 * Creates a new, empty dictionary variant type.
 */
variant_t * m2_variant_dict_new();
/**
 * Creates a new, empty list variant type.
 */
variant_t * m2_variant_list_new();

/**
 * Frees the given variant type.
 *
 * If the variant is a compound type (dict or list) then
 * all of the items contained will also be freed.
 */
void m2_variant_destroy(variant_t * value);

/**
 * Gets the string for the variant \a value.
 *
 * @param value     A string variant.
 *
 * @returns NULL if \a value is not a string type.
 */
bstring m2_variant_get_string(variant_t * value);

/**
 * Sets the entry \a key to \a item.
 * 
 * Takes ownership of the key and item.
 *
 * @param dict  A variant dictionary.
 * @param key   The key for the dictionary.
 * @param item  A variant type as the item.
 *
 * @returns 0 on error, non-zero on success.
 */
int m2_variant_dict_set(variant_t * val, const_bstring key, variant_t * item);

/**
 * Gets an item from a dictionary.
 */
variant_t * m2_variant_dict_get(const variant_t * dict, const_bstring key);

/**
 * Appends \a item to \a list
 *
 * @param list  The list to append to.
 * @param item  The item to append.
 *
 * @returns 0 on error, non-zero on success.
 */
int m2_variant_list_append(variant_t * list, variant_t * item);

// Parsing functions

/**
 * Parses a TNetstring and returns the variant value for it.
 * Sets \a rest to the first character after the range parsed,
 * unless passed NULL.
 *
 * @param       data      The data to parse
 * @param       len       The length of the data to parse
 * @param[out]  rest      A pointer to be filled with the location
 *                        of the first character after the parsed
 *                        range. Can be set to null to ignore.
 */
variant_t * m2_parse_tns(const char * data, size_t len, char ** rest);

variant_t * m2_parse_json(const char * data);

// Dumping functions
void m2_variant_dump_json(variant_t * val);

#endif//_VARIANT_H_DEF
