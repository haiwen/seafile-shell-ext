#include <time.h>
#include <stdarg.h>

#include <string>

#include "log.h"

extern std::string shellfix_log_path;
namespace {

static FILE *log_fp;

std::string getLogPath()
{
    if (shellfix_log_path.empty())
        return "";
    return shellfix_log_path;
}

} // anonymouse namespace

int
shellfix_log_start ()
{
    if (log_fp)
        return 0;

    std::string log_path = getLogPath();
    if (!log_path.empty()) {
        log_fp = fopen (log_path.c_str(), "a");
    } else {
        return -1;
    }
    if (log_fp) {
        shellfix_log ("\n----------------------------------\n"
                      "log file initialized  %s"
                      "\n----------------------------------\n"
                      , log_path.c_str());
    } else {
        fprintf (stderr, "[LOG] Can't init log file %s\n", log_path.c_str());
        return -1;
    }
    return 0;
}

void
shellfix_log_stop ()
{
    if (log_fp) {
        fclose (log_fp);
        log_fp = NULL;
    }
}

void
shellfix_log_aux (const char *format, ...)
{
    int result = 0;
    if (!log_fp)
        result = shellfix_log_start();

    va_list params;
    char buffer[1024];
    size_t length = 0;

    va_start(params, format);
    length = vsnprintf(buffer, sizeof(buffer), format, params);
    va_end(params);

    /* Write the timestamp. */
    time_t t;
    struct tm *tm;
    char buf[256];

    t = time(NULL);
    tm = localtime(&t);
    strftime (buf, 256, "[%y/%m/%d %H:%M:%S] ", tm);

    if (result == 0) {
        if (log_fp) {
            fputs (buf, log_fp);
            if (fwrite(buffer, sizeof(char), length, log_fp) < length)
                return;
            fputc('\n', log_fp);
            fflush(log_fp);
         }
    } else {
        fprintf(stdout, buf);
        fprintf(stdout, buffer);
        fprintf(stdout, "\n");
        fflush(stdout);
    }
}

