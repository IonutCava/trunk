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

// As of March 2020
#if !defined(CPP_17_SUPPORT) && !defined(ThirdPartyLibs)
#error "Divide Framework requires C++17 support at a minimum!."
#endif 

#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING

#ifndef BOOST_EXCEPTION_DISABLE
#define BOOST_EXCEPTION_DISABLE
#endif

#ifndef BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE
#define BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE
#endif 

#pragma once
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

#if defined(CPP_17_SUPPORT)
#include <any>
#include <variant>
#include <execution>
#else
#include <boost/any.hpp>
#include <boost/variant.hpp>
#endif

#include <boost/intrusive/slist.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/functional/factory.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/regex.hpp>
#include <simplefilewatcher/includes/FileWatcher.h>

#include <BetterEnums/include/enum.h>

#include <Optick/optick.h>

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

#include <Libs/fmt/include/fmt/format.h>
#include <Libs/fmt/include/fmt/printf.h>

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
#include "Core/Math/Headers/MathMatrices.h"
#include "Core/Math/Headers/Quaternion.h"

#ifndef YES_IMGUIMINIGAMES
#define YES_IMGUIMINIGAMES
#endif
#define NO_IMGUICODEEDITOR 
#include <imgui.h>

//#include <ECS.h>

#endif //_DIVIDE_PCH_
