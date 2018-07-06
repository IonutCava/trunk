#ifndef _RESOURCE_H_
#define _RESOURCE_H_

#define ElapsedSeconds() _startTime - time_t(nullptr)

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

#endif