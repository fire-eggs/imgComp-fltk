#pragma once

#ifdef _WIN32
#define MAXNAMLEN 512
#define F_OK 0
#else
#include <unistd.h>
#endif


void initlog(const char*);

void log(const char* fmt, ...);
