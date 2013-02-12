#ifndef _ERR_H_DEF
#define _ERR_H_DEF


/**
 * Gets the error number from a function that failed.
 *
 * A value of 0 means no error, otherwise its an error.
 */
int m2_errno();

void m2_set_errno(int n);

/**
 * Gets a string with a nice message about the error.
 *
 * The pointer returned is a pointer to the actual string,
 * so it is not suggested that you edit it or rely on it since
 * it can change underneath you.
 *
 * If m2_errno() is 0, then this function returns NULL
 */
char * m2_strerror();

void m2_set_errstr(char * message, ...) __attribute__((format(printf, 1, 2)));

/**
 * Prints the message from m2_strerror to stderr
 */
void m2_perror();

#define check(A,M,...) if (!(A)) { m2_set_errno(-1); m2_set_errstr((M), ##__VA_ARGS__); goto error; } else { m2_set_errno(0); }

#endif//_ERR_H_DEF
