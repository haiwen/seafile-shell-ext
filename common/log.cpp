
#include <string>

#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <userenv.h>
#include <stdarg.h>
#else
#include <pwd.h>
#include <unistd.h>
#endif

#include "log.h"

namespace {

static FILE *log_fp;

#ifdef _WIN32
std::string wStringToUtf8(const wchar_t *src)
{
    char dst[4096];
    int len;

    len = WideCharToMultiByte
        (CP_UTF8,               /* multibyte code page */
         0,                     /* flags */
         src,                   /* src */
         -1,                    /* src len, -1 for all includes \0 */
         dst,                   /* dst */
         sizeof(dst),           /* dst buf len */
         NULL,                  /* default char */
         NULL);                 /* BOOL flag indicates default char is used */

    if (len <= 0) {
        return "";
    }

    return dst;
}

void regulatePath(wchar_t *p)
{
    if (!p)
        return;

    wchar_t *s = p;
    /* Use capitalized C/D/E, etc. */
    if (s[0] >= L'a')
        s[0] = toupper(s[0]);

    /* Use / instead of \ */
    while (*s) {
        if (*s == L'\\')
            *s = L'/';
        s++;
    }

    s--;
    /* strip trailing white spaces and path seperator */
    while (isspace(*s) || *s == L'/') {
        *s = L'\0';
        s--;
    }
}


std::string getHomeDir()
{
    static wchar_t *home;

    if (home)
        return wStringToUtf8 (home);

    wchar_t buf[MAX_PATH] = {'\0'};

    if (!home) {
        /* Try env variable first. */
        GetEnvironmentVariableW(L"USERPROFILE", buf, MAX_PATH);
        if (wcslen(buf) != 0)
#if defined(_MSC_VER)
            home = _wcsdup(buf);
#else
            home = wcsdup(buf);
#endif
    }

    if (!home) {
        /* No `HOME' ENV; Try user profile */
        HANDLE hToken = NULL;
        DWORD len = MAX_PATH;
        if (OpenProcessToken (GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            GetUserProfileDirectoryW(hToken, buf, &len);
            CloseHandle(hToken);
            if (wcslen(buf) != 0)
#if defined(_MSC_VER)
                home = _wcsdup(buf);
#else
                home = wcsdup(buf);
#endif
        }
    }

    if (home) {
        regulatePath(home);
    }

    return home ? wStringToUtf8(home):"";
}
#else
std::string getHomeDir()
{
    struct passwd *pw = getpwuid(getuid());
    std::string home {pw->pw_dir}; 
    if (home.empty())
        return "";

    return home + "/.seadrive";
}
#endif

std::string getLogPath()
{
    std::string home = getHomeDir();
    printf("home dir: %s\n", home.c_str());
    if (home.empty())
        return "";

    return home + "/" + log_file_name;
}


}

void
seaf_ext_log_start ()
{
    if (log_fp)
        return;

    std::string log_path = getLogPath();
    if (!log_path.empty())
        log_fp = fopen (log_path.c_str(), "a");

    if (log_fp) {
        seaf_ext_log ("\n----------------------------------\n"
                      "log file initialized: 4.1.0 %s"
                      "\n----------------------------------\n"
                      , log_path.c_str());
    } else {
        fprintf (stderr, "[LOG] Can't init log file %s\n", log_path.c_str());
    }
}

void
seaf_ext_log_stop ()
{
    if (log_fp) {
        fclose (log_fp);
        log_fp = NULL;
    }
}

void
seaf_ext_log_aux (const char *format, ...)
{
    if (!log_fp)
        seaf_ext_log_start();

    if (log_fp) {
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

        fputs (buf, log_fp);
        if (fwrite(buffer, sizeof(char), length, log_fp) < length)
            return;

        fputc('\n', log_fp);
        fflush(log_fp);
    }
}
