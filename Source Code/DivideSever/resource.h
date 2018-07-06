#ifndef _RESOURCE_H_
#define _RESOURCE_H_

#define GETTIME() start_time - time_t(nullptr)

#if defined(_MSC_VER)
#	pragma warning( push )
#		pragma warning(disable:4244)
#elif defined(__GNUC__)
#	pragma GCC diagnostic push
#		//pragma GCC diagnostic ignored "-Wall"
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

#if defined(_MSC_VER)
#	pragma warning( pop )
#elif defined(__GNUC__)
#	pragma GCC diagnostic pop
#endif

#endif