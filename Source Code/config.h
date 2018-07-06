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
    constexpr bool IS_SHIPPING_BUILD = false;
    constexpr bool IS_DEBUG_BUILD = false;
    constexpr bool IS_PROFILE_BUILD = false;
    constexpr bool IS_RELEASE_BUILD = true;
#endif
};

/// Use OpenGL/OpenGL ES for rendering
constexpr bool USE_OPENGL_RENDERING = true;
/// Select between desktop GL and ES GL
constexpr bool USE_OPENGL_ES = false;
/// if this is false, a variable timestep will be used for the game loop
constexpr bool USE_FIXED_TIMESTEP = true;
/// Application desired framerate for physics and input simulations
constexpr unsigned int TARGET_FRAME_RATE = 60;
/// Minimum required RAM size (in bytes) for the current build
constexpr unsigned int REQUIRED_RAM_SIZE = 2 * 1024 * 1024; //2Gb
/// Application update rate divisor (how many draw calls per render call
/// e.g. 2 = 30Hz update rate at 60Hz rendering)
constexpr unsigned int TICK_DIVISOR = 2;
/// Application update rate
constexpr unsigned int TICKS_PER_SECOND = TARGET_FRAME_RATE / TICK_DIVISOR;
/// Maximum frameskip
constexpr unsigned int MAX_FRAMESKIP = 3;
constexpr unsigned long long SKIP_TICKS = (1000 * 1000) / Config::TICKS_PER_SECOND;
/// The minimum threshold needed for a threaded loop to use sleep
/// Update intervals bellow this threshold will not use sleep!
constexpr unsigned int MIN_SLEEP_THRESHOLD_MS = 5;
/// How many tasks should we keep in a pool to avoid using new/delete (must be power of two)
constexpr unsigned int MAX_POOLED_TASKS = 4096;
/// AI update frequency
constexpr unsigned int AI_THREAD_UPDATE_FREQUENCY = TICKS_PER_SECOND;
/// Toggle multi-threaded resource loading on or off
constexpr bool USE_GPU_THREADED_LOADING = true;
/// Run all threaded tasks in a serial fashion. (used to debug multi-threaded related bugs)
constexpr bool USE_SINGLE_THREADED_TASK_POOLS = false;
/// Maximum number of instances of a single mesh with a single draw call
constexpr unsigned int MAX_INSTANCE_COUNT = 512;
/// Maximum number of points that can be sent to the GPU per batch
constexpr unsigned int MAX_POINTS_PER_BATCH = static_cast<unsigned int>(1 << 31);
/// Maximum number of bones available per node
constexpr unsigned int MAX_BONE_COUNT_PER_NODE = 256;
/// Estimated maximum number of visible objects per render pass
//(This includes debug primitives)
constexpr unsigned int MAX_VISIBLE_NODES = 1024;
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

/// Reflection render target resolution
constexpr unsigned int REFLECTION_TARGET_RESOLUTION = 512;
constexpr unsigned int REFRACTION_TARGET_RESOLUTION = 512;
/// Generic index value used to separate primitives within the same vertex
/// buffer
constexpr unsigned int PRIMITIVE_RESTART_INDEX_L = 0xFFFFFFFF;
constexpr unsigned int PRIMITIVE_RESTART_INDEX_S = 0xFFFF;
/// Terrain LOD configuration
/// Camera distance to the terrain chunk is calculated as follows:
///    vector EyeToChunk = terrainBoundingBoxCenter - EyePos; cameraDistance =
///    EyeToChunk.length();
/// Number of LOD levels for the terrain
constexpr unsigned int TERRAIN_CHUNKS_LOD = 3;
/// How many grass elements (3 quads p.e.) to add to each terrain element
constexpr unsigned int MAX_GRASS_BATCHES = 2000000;
/// SceneNode LOD selection
/// Distance computation is identical to the of the terrain (using SceneNode's
/// bounding box)
constexpr unsigned int SCENE_NODE_LOD = 3;
/// Relative distance for LOD0->LOD1 selection
constexpr unsigned int SCENE_NODE_LOD0 = 100;
/// Relative distance for LOD0->LOD2 selection
constexpr unsigned int SCENE_NODE_LOD1 = 180;
/// Use "precompiled" shaders if possible
constexpr bool USE_SHADER_BINARY = true;
/// Use HW AA'ed lines
constexpr bool USE_HARDWARE_AA_LINES = true;
/// Multi-draw causes some problems with profiling software (e.g.
/// GPUPerfStudio2)
constexpr bool BATCH_DRAW_COMMANDS = false;
/// Maximum number of draw commands allowed in flight at any time
constexpr unsigned int MAX_DRAW_COMMANDS_IN_FLIGHT = 4096;
/// If true, load shader source coude from cache files
/// If false, materials recompute shader source code from shader atoms
/// If true, clear shader cache to apply changes to shader atom source code
#if defined(_DEBUG)
constexpr bool USE_SHADER_TEXT_CACHE = false;
#else
constexpr bool USE_SHADER_TEXT_CACHE = true;
#endif
/// If true, Hi-Z based occlusion culling is used
constexpr bool USE_HIZ_CULLING = true;
/// If true, Hi-Z culling is disabled and potentially culled nodes are drawn in bright red and double in size
constexpr bool DEBUG_HIZ_CULLING = false;
/// If true, the depth pass acts as a zPrePass for the main draw pass as well
/// If false, the main draw pass will clear the depth buffer and populate a new one instead
constexpr bool USE_Z_PRE_PASS = true;

/// Compute related options
namespace Compute {

}; //namespace Compute

/// Profiling options
namespace Profile {
/// enable function level profiling
#if defined(_DEBUG) || defined(_PROFILE)
    constexpr bool ENABLE_FUNCTION_PROFILING = true;
#else
    constexpr bool ENABLE_FUNCTION_PROFILING = true;
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
/// skip all draw calls
constexpr bool DISABLE_DRAWS = false;
/// every viewport call is overridden with 1x1 (width x height)
constexpr bool USE_1x1_VIEWPORT = false;
/// how many profiling timers are we allowed to use in our applications
constexpr unsigned int MAX_PROFILE_TIMERS = 2048;
/// textures are capped at 2x2 when uploaded to the GPU
constexpr bool USE_2x2_TEXTURES = false;
/// disable persistently mapped buffers
constexpr bool DISABLE_PERSISTENT_BUFFER = false;
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
constexpr unsigned int MAX_SHADOW_CASTING_LIGHTS = 5;
/// Used for CSM or PSSM to determine the maximum number of frustum splits
/// And cube map shadows as well
constexpr unsigned int MAX_SPLITS_PER_LIGHT = 6;
/// How many "units" away should a directional light source be from the camera's
/// position
constexpr unsigned int DIRECTIONAL_LIGHT_DISTANCE = 500;

/// the maximum number of lights supported, this is limited by constant buffer
/// size, commonly this is 64kb, but AMD only seem to allow 2048 lights...
constexpr unsigned int MAX_POSSIBLE_LIGHTS = 1024;

/// The following parameters control the behaviour of the Forward+ renderer
constexpr unsigned int FORWARD_PLUS_TILE_RES = 16;
constexpr unsigned int FORWARD_PLUS_MAX_LIGHTS_PER_TILE = 544;
constexpr unsigned int FORWARD_PLUS_LIGHT_INDEX_BUFFER_SENTINEL = 0x7fffffff;
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
    //#define GPU_VALIDATION_IN_PROFILE_BUILD
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

#ifndef SERVER_LOG_FILE
#define SERVER_LOG_FILE "server.log"
#endif  // SERVER_LOG_FILE

/// Disable or enable custom, general purpose allocators in the code
/// (used by containers, frequently created objects, etc)
/// This does not disable all custom allocators as some, like special object pools are very specialised 
//#define USE_CUSTOM_MEMORY_ALLOCATORS

#define BOOST_IMP 0
#define STL_IMP 1
#define EASTL_IMP 2

/// Specify the string implementation to use
#ifndef STRING_IMP
#define STRING_IMP STL_IMP
#endif  // STRING_IMP

/// Specify the vector implementation to use
#ifndef VECTOR_IMP
#define VECTOR_IMP STL_IMP
#endif  // VECTOR_IMP

/// Specify the hash maps / unordered maps implementation to use
#ifndef HASH_MAP_IMP
#define HASH_MAP_IMP STL_IMP
#endif  // HASH_MAP_IMP

#endif  //_DIVIDE_CONFIG_H_
