#ifdef _WIN32
#include "stdlib.h"
//#define sleep _sleep
#include <direct.h> // _getcwd
#define getcwd _getcwd
#define MAXNAMLEN 512
#include <io.h> // access
#define access _access
#define F_OK 0
#else
#include <unistd.h> // access
#endif

#include <stdarg.h> // va_start for log
#include <chrono>   // timestamp for log
#include <cstring>  // strcat
#include <dirent.h> // MAXNAMLEN
#include <string>   // std::string

char _logpath[MAXNAMLEN * 2];

void initlog(const char *name)
{
    // TODO is there a FLTK function???
    // set up the log file
    auto res = getcwd(_logpath, sizeof(_logpath));
    strcat(_logpath, "/");
    strcat(_logpath, name);
}

std::string getCurrentTimestamp()
{
    auto currentTime = std::chrono::system_clock::now();
    char buffer[40];
    char buffer2[80];

    auto transformed = currentTime.time_since_epoch().count() / 1000000;

    unsigned short millis = transformed % 1000;

    auto tt = std::chrono::system_clock::to_time_t(currentTime);
    auto timeinfo = localtime(&tt);
    strftime(buffer, 40, "%F %H:%M:%S", timeinfo);
    snprintf(buffer2, 80, "%s.%03u", buffer, millis);

    return std::string(buffer2);
}

void log(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char buff[1024];
    vsnprintf(buff, 1024, fmt, ap);

    std::string ts = getCurrentTimestamp();

    FILE* logf = fopen(_logpath, "a");
    fputs(ts.c_str(), logf);
    fputs(" | ", logf);
    fputs(buff, logf);
    fputc('\n', logf);
    fclose(logf);
}

void showLog()
{
    char buff [(int)(MAXNAMLEN*2.5)];
    sprintf(buff, "xdg-open '%s'", _logpath);
    system(buff);
}
