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
#ifndef _DIVIDE_CONFIG_H_
#define _DIVIDE_CONFIG_H_

namespace Divide {
namespace Config {
namespace Build {

#if defined(_DEBUG)
    constexpr bool IS_SHIPPING_BUILD = false;
    constexpr bool IS_DEBUG_BUILD = true;
    constexpr bool IS_PROFILE_BUILD = false;
    constexpr bool IS_RELEASE_BUILD = false;
#elif defined(_PROFILE)
    constexpr bool IS_SHIPPING_BUILD = false;
    constexpr bool IS_DEBUG_BUILD = false;
    constexpr bool IS_PROFILE_BUILD = true;
    constexpr bool IS_RELEASE_BUILD = false;
#else
    // Set IS_SHIPPING_BUILD to true to disable non-required functionality for shipped games: editors, debug code, etc
    constexpr bool IS_SHIPPING_BUILD = true;
    constexpr bool IS_DEBUG_BUILD = false;
    constexpr bool IS_PROFILE_BUILD = false;
    constexpr bool IS_RELEASE_BUILD = true;
#endif

    constexpr bool ENABLE_EDITOR = !IS_SHIPPING_BUILD;
};

/// Application desired framerate for physics and input simulations
constexpr unsigned int TARGET_FRAME_RATE = 60;
/// Application update rate divisor (how many many times should we update our state per second)
/// e.g. For TARGET_FRAME_RATE = 60, TICK_DIVISOR = 2 => update at 30Hz, render at 60Hz.
constexpr unsigned int TICK_DIVISOR = 2;
/// Maximum frameskip
constexpr unsigned int MAX_FRAMESKIP = 3;
/// The minimum threshold needed for a threaded loop to use sleep. Update intervals bellow this threshold will not use sleep!
constexpr unsigned int MIN_SLEEP_THRESHOLD_MS = 5;
/// Minimum required RAM size (in bytes) for the current build
constexpr unsigned int REQUIRED_RAM_SIZE = 2 * 1024 * 1024; //2Gb
/// How many tasks should we keep in a pool to avoid using new/delete (must be power of two)
constexpr unsigned int MAX_POOLED_TASKS = 16384;
/// Maximum number of bones available per node
constexpr unsigned int MAX_BONE_COUNT_PER_NODE = 128;
/// Estimated maximum number of visible objects per render pass (this includes debug primitives)
constexpr unsigned int MAX_VISIBLE_NODES = 4096 * 2;
/// How many clip planes should the shaders us
/// How many reflective objects are we allowed to display on screen at the same time
#   if defined(_DEBUG)
constexpr unsigned int MAX_REFLECTIVE_NODES_IN_VIEW = 3;
#   else
constexpr unsigned int  MAX_REFLECTIVE_NODES_IN_VIEW = 5;
#   endif
#   if defined(_DEBUG)
constexpr unsigned int MAX_REFRACTIVE_NODES_IN_VIEW = 2;
#   else
constexpr unsigned int  MAX_REFRACTIVE_NODES_IN_VIEW = 4;
#   endif
constexpr unsigned int MAX_REFLECTIVE_PROBES_PER_PASS = 6;

/// Resolution used for environment probe rendering
constexpr unsigned int REFLECTION_TARGET_RESOLUTION_ENVIRONMENT_PROBE = 256;

/// Generic index value used to separate primitives within the same vertex
/// buffer
constexpr unsigned int PRIMITIVE_RESTART_INDEX_L = 0xFFFFFFFF;
constexpr unsigned int PRIMITIVE_RESTART_INDEX_S = 0xFFFF;

/// Maximum number of players we support locally
constexpr unsigned int MAX_LOCAL_PLAYER_COUNT = 4;

/// Use the coloured version of WOIT as detailed here: http://casual-effects.blogspot.com/2015/03/colored-blended-order-independent.html (Note: Not yet working!)
constexpr bool USE_COLOURED_WOIT = false;

/// Compute related options
namespace Compute {

}; //namespace Compute

/// Profiling options
namespace Profile {
/// how many profiling timers are we allowed to use in our applications
constexpr unsigned int MAX_PROFILE_TIMERS = 1024;
/// enable function level profiling
#if defined(_DEBUG) || defined(_PROFILE)
    constexpr bool ENABLE_FUNCTION_PROFILING = true;
#else
    constexpr bool ENABLE_FUNCTION_PROFILING = false;
#endif
/// run performance profiling code
#if defined(_DEBUG) || defined(_PROFILE)
    constexpr bool BENCHMARK_PERFORMANCE = true;
#else
    constexpr bool BENCHMARK_PERFORMANCE = false;
#endif
/// Benchmark reset frequency in milliseconds
constexpr unsigned int BENCHMARK_FREQUENCY = 500;
/// use only a basic shader
constexpr bool DISABLE_SHADING = false;
/// textures are capped at 2x2 when uploaded to the GPU
constexpr bool USE_2x2_TEXTURES = false;
};  // namespace Profile

namespace Assert {
#if defined(_DEBUG)
/// Log assert fails messages to the error log file
    constexpr bool LOG_ASSERTS = true;
/// Do not call the platform "assert" function in order to continue application
/// execution
    constexpr bool CONTINUE_ON_ASSERT = false;
/// Popup a GUI Message Box on asserts;
    constexpr bool SHOW_MESSAGE_BOX = true;
#elif defined(_PROFILE)
    constexpr bool LOG_ASSERTS = true;
    constexpr bool CONTINUE_ON_ASSERT = false;
    constexpr bool SHOW_MESSAGE_BOX = false;
#else  //_RELEASE
    constexpr bool LOG_ASSERTS = false;
    constexpr bool CONTINUE_ON_ASSERT = false;
    constexpr bool SHOW_MESSAGE_BOX = false;
#endif
};  // namespace Assert

namespace Lighting {
// How many lights (in order as passed to the shader for the node) should cast shadows
constexpr unsigned short MAX_SHADOW_CASTING_LIGHTS = 4;
constexpr unsigned short MAX_SHADOW_CASTING_DIRECTIONAL_LIGHTS = 2;
static_assert(MAX_SHADOW_CASTING_DIRECTIONAL_LIGHTS <= MAX_SHADOW_CASTING_LIGHTS, "Too many directional shadow casting lights configured!");
/// Used for cube map shadows and for CSM or PSSM to determine the maximum number of frustum splits
constexpr unsigned short MAX_CSM_SPLITS_PER_LIGHT = 4;
static_assert(MAX_CSM_SPLITS_PER_LIGHT <= 6, "Too many CSM splits. Max = 6 (number of cube faces))");

/// Used mainly for caching/memory efficiency reasons
constexpr unsigned short MAX_POSSIBLE_LIGHTS = 4096u;

namespace ForwardPlus {
    constexpr unsigned short MAX_LIGHTS_PER_TILE = 512u;
};

};  // namespace Lighting

namespace Networking {
/// How often should the client send messages to the server
constexpr unsigned int NETWORK_SEND_FREQUENCY_HZ = 20;
/// How many times should we try to send an update to the server before giving up?
constexpr unsigned int NETWORK_SEND_RETRY_COUNT = 3;
};
#ifndef GPU_VALIDATION_IN_RELEASE_BUILD
    //#define GPU_VALIDATION_IN_RELEASE_BUILD
#endif

#ifndef GPU_VALIDATION_IN_PROFILE_BUILD
    #define GPU_VALIDATION_IN_PROFILE_BUILD
#endif

#ifndef GPU_VALIDATION_IN_DEBUG_BUILD
#define GPU_VALIDATION_IN_DEBUG_BUILD
#endif


#if (defined(_RELEASE) && defined(GPU_VALIDATION_IN_RELEASE_BUILD)) || \
    (defined(_PROFILE) && defined(GPU_VALIDATION_IN_PROFILE_BUILD)) || \
    (defined(_DEBUG) && defined(GPU_VALIDATION_IN_DEBUG_BUILD))
constexpr bool ENABLE_GPU_VALIDATION = true;
#else
constexpr bool ENABLE_GPU_VALIDATION = false;
#endif

constexpr bool ENABLE_LOCALE_FILE_WATCHER = false;

};  // namespace Config
};  // namespace Divide

/// If the target machine uses the nVidia Optimus layout (IntelHD + nVidiadiscreet GPU)
/// or the AMD PowerXPress system, this forces the client to use the high performance GPU
#ifndef FORCE_HIGHPERFORMANCE_GPU
#define FORCE_HIGHPERFORMANCE_GPU
#endif

/// Please enter the desired log file name
#ifndef OUTPUT_LOG_FILE
#define OUTPUT_LOG_FILE "console.log"
#endif  // OUTPUT_LOG_FILE

#ifndef ERROR_LOG_FILE
#define ERROR_LOG_FILE "errors.log"
#endif  // ERROR_LOG_FILE

#endif  //_DIVIDE_CONFIG_H_
