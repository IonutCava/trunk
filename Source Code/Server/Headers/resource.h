#ifndef _RESOURCE_H_
#define _RESOURCE_H_

#define ElapsedSeconds() start_time - time_t(nullptr)

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#//pragma GCC diagnostic ignored "-Wall"
#endif

//#define _UNIX_
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cassert>
#include <list>
#include <map>
#include <algorithm>
#include <deque>
#include <set>

#include "Utility/Headers/DataTypes.h"

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset,
                     const char* pName, int flags, unsigned debugFlags,
                     const char* file, int line);

// EASTL also wants us to define this (see string.h line 197)
int Vsnprintf8(char* pDestination, size_t n, const char* pFormat,
               va_list arguments);

#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif