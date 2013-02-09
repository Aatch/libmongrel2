/**
 * Heavily inspired by dbg.h from Mongrel2.
 *
 * Loses a lot of the general logging features.
 *
 */

#ifndef _DBG_H_DEF
#define _DBG_H_DEF

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

// For handling the format attribute
#ifndef __GNUC__
# define __attribute__(x) // Nothing
#endif

void fprintf_with_timestamp(FILE *log_file, const char *format, ...) __attribute__((format(printf,2,3)));

#ifdef NDEBUG
#define debug(M, ...) //Nothing
#else
#define debug(M, ...) fprintf_with_timestamp(stderr, "[DEBUG] %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define check(A, M, ...) if (!(A)) { debug(M, ##__VA_ARGS__); errno=0; goto error; }

#endif//DBG_H_DEF
