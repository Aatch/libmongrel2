#include <stdarg.h>
#include <stdio.h>

#include "err.h"
#include "bstring.h"

static __thread int errno = 0;
static __thread char errstr[1024];

int m2_errno() {
    return errno;
}

void m2_set_errno(int n) {
    errno = n;
}

char * m2_strerror() {
    return errno ? errstr : NULL;
}

void m2_set_errstr(char * message, ...) {
    if (!message) {
        errstr[0] = '\0';
    } else {
        va_list args;
        va_start(args, message);
        vsnprintf(errstr, 1024, message, args);
        va_end(args);
    }
}
