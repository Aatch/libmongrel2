#include "dbg.h"
#include "bstring.h"

void fprintf_with_timestamp(FILE *log_file, const char *format, ...) {
    va_list args;
    time_t now = time(NULL);
    struct tm res;

    bstring time_stamp = bStrfTime("%H:%M:%S", localtime_r(&now, &res));

    va_start(args, format);
    fprintf(log_file, "%s ", bdata(time_stamp));
    vfprintf(log_file, format, args);
    bdestroy(time_stamp);
    va_end(args);
}
