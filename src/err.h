#ifndef _ERR_H_DEF
#define _ERR_H_DEF

// For handling the format attribute
#ifndef __GNUC__
# define __attribute__(x) // Nothing
#endif

/**
 * Gets the error number from a function that failed.
 *
 * A value of 0 means no error, otherwise its an error.
 *
 * \note There are currently no specific codes, just zero and
 *       non-zero.
 */
extern int m2_errno();


/**
 * Gets a string with a nice message about the error.
 *
 * The pointer returned is a pointer to the actual string,
 * so it is not suggested that you edit it or rely on it since
 * it can change underneath you.
 *
 * If m2_errno() is 0, then this function returns NULL
 */
extern char * m2_strerror();

extern char * m2_strerror_cpy(char * dest);

/**
 * Prints the message from m2_strerror to stderr
 */
extern void m2_perror();

extern void m2_set_errstr(char * message, ...) __attribute__((format(printf, 1, 2)));
extern void m2_set_errno(int n);

#define check(A,M,...) if (!(A)) { m2_set_errno(-1); m2_set_errstr((M), ##__VA_ARGS__); assert(!(A)); goto error; } else { m2_set_errno(0); }
#define check_mem(A) check(A, "Out of memory")

#endif//_ERR_H_DEF
