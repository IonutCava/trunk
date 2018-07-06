/*
Copyright (c) 2017 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef _DIVIDE_PCH_
#define _DIVIDE_PCH_

#if defined(_WIN32)
#include "Platform/Headers/PlatformDefinesWindows.h"
#elif defined(__APPLE_CC__) 
#include "Platform/Headers/PlatformDefinesApple.h"
#else //defined(__linux) || defined (__unix)
#include "Platform/Headers/PlatformDefinesUnix.h"
#endif

#if !defined(CPP_VERSION)
#   define CPP_VERSION __cplusplus
#endif

#if CPP_VERSION > 1
#   define CPP_98_SUPPORT
#   define CPP_03_SUPPORT
#   if CPP_VERSION >= 201103L
#       define CPP_11_SUPPORT
#           if CPP_VERSION >= 201402L
#               define CPP_14_SUPPORT
#               if CPP_VERSION > 201402L
#                   if HAS_CPP17
#                       define CPP_17_SUPPORT
#                   endif
#               endif
#           endif
#   endif
#endif 

#if !defined(CPP_11_SUPPORT)
#error "Divide Framework requires C++11 support at a minimum!."
#endif 

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif //_USE_MATH_DEFINES
#include <cmath>
#include <math.h>

#include <condition_variable>
#include <type_traits>
#include <functional>
#include <stdexcept>
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <cstdarg>
#include <numeric>
#include <cstring>
#include <sstream>
#include <float.h>
#include <memory>
#include <bitset>
#include <limits>
#include <atomic>
#include <cstdio>
#include <cctype>
#include <random>
#include <thread>
#include <chrono>
#include <array>
#include <deque>
#include <regex>
#include <array>
#include <set>
#include <new>

#if !defined(CPP_17_SUPPORT)
#include "Core/Headers/cdigginsAny.h"
#else
#include <any>
#endif

#include <simplefilewatcher/includes/FileWatcher.h>

#ifndef BOOST_EXCEPTION_DISABLE
#define BOOST_EXCEPTION_DISABLE
#endif
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/thread/tss.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/functional/factory.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/serialization/vector.hpp>

#define HAVE_M_PI
#define SDL_MAIN_HANDLED
#include <sdl/include/SDL_mixer.h>
#include <sdl/include/SDL.h>

#include <simpleCL.h>

#include <chaiscript/chaiscript_stdlib.hpp>
#include <chaiscript/utility/utility.hpp>

#include <ConcurrentQueue/concurrentqueue.h>

#include "Platform/Threading/Headers/SharedMutex.h"
#include "Platform/Headers/PlatformDataTypes.h"
#include "Platform/Headers/ConditionalWait.h"
#include "Core/Headers/Singleton.h"
#include "Core/Headers/NonCopyable.h"
#include "Core/Headers/GUIDWrapper.h"

#include "Core/TemplateLibraries/Headers/HashMap.h"
#include "Core/TemplateLibraries/Headers/Vector.h"
#include "Core/TemplateLibraries/Headers/String.h"

#endif //_DIVIDE_PCH_
