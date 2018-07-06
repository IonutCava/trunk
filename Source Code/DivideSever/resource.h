#ifndef _RESOURCE_H_
#define _RESOURCE_H_

#define GETTIME() start_time - time_t(NULL)

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
#include <assert.h>
#include <list>
#include <map>
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <set>

using namespace std;
#include "Utility/Headers/DataTypes.h"

#if defined(_MSC_VER)
#	pragma warning( pop )
#elif defined(__GNUC__)
#	pragma GCC diagnostic pop
#endif

#endif