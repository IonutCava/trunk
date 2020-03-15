/*
Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _DIVIDE_PCH_
#define _DIVIDE_PCH_

#include "Platform/Headers/PlatformDefinesOS.h"

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

#if !defined(CPP_14_SUPPORT)
#error "Divide Framework requires C++14 support at a minimum!."
#endif 

#if defined(CPP_17_SUPPORT)
#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#endif

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif //_USE_MATH_DEFINES
#include <cmath>
#include <math.h>

#include <condition_variable>
#include <unordered_set>
#include <type_traits>
#include <functional>
#include <stdexcept>
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <fstream>
#include <iomanip>
#include <cstdarg>
#include <numeric>
#include <cstring>
#include <sstream>
#include <float.h>
#include <cstdint>
#include <string>
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
#include <stack>
#include <deque>
#include <regex>
#include <array>
#include <queue>
#include <list>
#include <set>
#include <new>

//#include <gsl/gsl>

#if !defined(CPP_17_SUPPORT)
#include "Core/Headers/cdigginsAny.h"
namespace cd = ::cdiggins;
using AnyParam = cd::any;
template <typename T>
FORCE_INLINE T any_cast(const AnyParam& param) {
    return param.constant_cast<T>();
}
#else
#include <any>
using AnyParam = std::any;
template <typename T>
FORCE_INLINE T any_cast(const AnyParam& param) {
    return std::any_cast<T>(param);
}
#endif

namespace boost {
    namespace asio {
        class thread_pool;
    };
};

#include <boost/intrusive/slist.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/functional/factory.hpp>
//ToDo: Replace this with either the C++17 filesystem lib or with custom stuff in PlatformDefines
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/serialization/vector.hpp>
#pragma warning(push)
#pragma warning(disable:4458)
#pragma warning(disable:4706)
#include <boost/wave.hpp>
#include <boost/wave/cpplexer/cpp_lex_token.hpp>    // token class
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp> // lexer class
#pragma warning(pop)

#include <simplefilewatcher/includes/FileWatcher.h>

#include <BetterEnums/include/enum.h>

#include <Optick/optick.h>

#ifndef BOOST_EXCEPTION_DISABLE
#define BOOST_EXCEPTION_DISABLE
#endif

#ifndef BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE
#define BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE
#endif 


#define HAVE_M_PI
#define SDL_MAIN_HANDLED
#include <sdl/include/SDL_mixer.h>
#include <sdl/include/SDL.h>

#include <EASTL/set.h>
#include <EASTL/list.h>
#include <EASTL/array.h>
#include <eastl/stack.h>
#include <EASTL/vector_map.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/weak_ptr.h>
#include <EASTL/unique_ptr.h>

#include <MemoryPool/StackAlloc.h>
#include <MemoryPool/C-11/MemoryPool.h>

#include <ArenaAllocator/arena_allocator.h>

#include <chaiscript/include/chaiscript/chaiscript.hpp>
#include <chaiscript/include/chaiscript/chaiscript_stdlib.hpp>
#include <chaiscript/include/chaiscript/utility/utility.hpp>


#include <ConcurrentQueue/concurrentqueue.h>
#include <ConcurrentQueue/blockingconcurrentqueue.h>

#include "Platform/Headers/PlatformDataTypes.h"
#include "Platform/File/Headers/FileWithPath.h"
#include "Platform/Threading/Headers/SharedMutex.h"
#include "Platform/Headers/ConditionalWait.h"
#include "Core/Headers/Singleton.h"
#include "Core/Headers/NonCopyable.h"
#include "Core/Headers/GUIDWrapper.h"

#include "Core/TemplateLibraries/Headers/HashMap.h"
#include "Core/TemplateLibraries/Headers/Vector.h"
#include "Core/TemplateLibraries/Headers/String.h"


#ifndef YES_IMGUIMINIGAMES
#define YES_IMGUIMINIGAMES
#endif
#define NO_IMGUICODEEDITOR 
#include <imgui.h>

//#include <ECS.h>

#endif //_DIVIDE_PCH_
