#pragma once
#include <stdint.h>
#ifdef __linux
#define LINUX
#endif // __linux

#ifndef WIN32
#ifdef _WIN32
#define WIN32
#endif
#endif

