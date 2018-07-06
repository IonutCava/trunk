/*
   Copyright (c) 2015 DIVIDE-Studio
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

/**************************************************************************************************************\
*                             Welcome to DIVIDE Engine's config settings! *
* From here you can decide how you want to build your release of the code.
* - Decide how many threads your application can use (maximum upper limit).
* - Show the debug ms-dos window that prints application information
* - Define what the console output log file should be named
* - Enable/Disable SSE support
* - Set renderer options
***************************************************************************************************************/
#ifndef _DIVIDE_CONFIG_H_
#define _DIVIDE_CONFIG_H_

namespace Divide {
namespace Config {
/// Use OpenGL/OpenGL ES for rendering
const bool USE_OPENGL_RENDERING = true;
/// Select between desktop GL and ES GL
const bool USE_OPENGL_ES = false;
/// if this is false, a variable timestep will be used for the game loop
const bool USE_FIXED_TIMESTEP = true;
/// How many textures to store per material.
/// bump(0) + opacity(1) + spec(2) + tex[3..MAX_TEXTURE_STORAGE - 1]
const unsigned int MAX_TEXTURE_STORAGE = 6;
/// Application desired framerate for physics simulations
const unsigned int TARGET_FRAME_RATE = 60;
/// Minimum required RAM size (in bytes) for the current build
const unsigned int REQUIRED_RAM_SIZE = 2 * 1024 * 1024; //2Gb
/// Application update rate divisor (how many draw calls per render call
/// e.g. 2 = 30Hz update rate at 60Hz rendering)
const unsigned int TICK_DIVISOR = 2;
/// Application update rate
const unsigned int TICKS_PER_SECOND = TARGET_FRAME_RATE / TICK_DIVISOR;
/// Maximum frameskip
const unsigned int MAX_FRAMESKIP = 5;
const unsigned long long SKIP_TICKS = (1000 * 1000) / Config::TICKS_PER_SECOND;
/// AI update frequency
const unsigned int AI_THREAD_UPDATE_FREQUENCY = TICKS_PER_SECOND;
/// Toggle multi-threaded resource loading on or off
const bool USE_GPU_THREADED_LOADING = false;
/// Maximum number of instances of a single mesh with a single draw call
const unsigned int MAX_INSTANCE_COUNT = 512;
/// Maximum number of points that can be sent to the GPU per batch
const unsigned int MAX_POINTS_PER_BATCH = static_cast<unsigned int>(1 << 31);
/// Maximum number of bones available per node
const unsigned int MAX_BONE_COUNT_PER_NODE = 256;
/// Estimated maximum number of visible objects per render pass
//(This includes debug primitives)
const unsigned int MAX_VISIBLE_NODES = 1024;
/// How many clip planes should the shaders us
const unsigned int MAX_CLIP_PLANES = 6;
/// Generic index value used to separate primitives within the same vertex
/// buffer
const unsigned int PRIMITIVE_RESTART_INDEX_L = 0xFFFFFFFF;
const unsigned int PRIMITIVE_RESTART_INDEX_S = 0xFFFF;
/// Terrain LOD configuration
/// Camera distance to the terrain chunk is calculated as follows:
///    vector EyeToChunk = terrainBoundingBoxCenter - EyePos; cameraDistance =
///    EyeToChunk.length();
/// Number of LOD levels for the terrain
const unsigned int TERRAIN_CHUNKS_LOD = 3;
/// How many grass elements (3 quads p.e.) to add to each terrain element
const unsigned int MAX_GRASS_BATCHES = 2000000;
/// SceneNode LOD selection
/// Distance computation is identical to the of the terrain (using SceneNode's
/// bounding box)
const unsigned int SCENE_NODE_LOD = 3;
/// Relative distance for LOD0->LOD1 selection
const unsigned int SCENE_NODE_LOD0 = 100;
/// Relative distance for LOD0->LOD2 selection
const unsigned int SCENE_NODE_LOD1 = 180;
/// Use "precompiled" shaders if possible
const bool USE_SHADER_BINARY = true;
/// Use HW AA'ed lines
const bool USE_HARDWARE_AA_LINES = true;
/// Multi-draw causes some problems with profiling software (e.g.
/// GPUPerfStudio2)
const bool BATCH_DRAW_COMMANDS = false;

/// Compute related options
namespace Compute {

}; //namespace Compute

/// Profiling options
namespace Profile {
/// use only a basic shader
const bool DISABLE_SHADING = false;
/// skip all draw calls
const bool DISABLE_DRAWS = false;
/// every viewport call is overriden with 1x1 (width x height)
const bool USE_1x1_VIEWPORT = false;
/// textures are capped at 2x2 when uploaded to the GPU
const bool USE_2x2_TEXTURES = false;
/// disable persistently mapped buffers
const bool DISABLE_PERSISTENT_BUFFER = false;
};  // namespace Profile

namespace Assert {
#if defined(_DEBUG)
/// Log assert fails messages to the error log file
const bool LOG_ASSERTS = true;
/// Do not call the platform "assert" function in order to continue application
/// execution
const bool CONTINUE_ON_ASSERT = false;
/// Popup a GUI Message Box on asserts;
const bool SHOW_MESSAGE_BOX = true;
#elif defined(_PROFILE)
const bool LOG_ASSERTS = true;
const bool CONTINUE_ON_ASSERT = false;
const bool SHOW_MESSAGE_BOX = false;
#else  //_RELEASE
const bool LOG_ASSERTS = false;
const bool CONTINUE_ON_ASSERT = false;
const bool SHOW_MESSAGE_BOX = false;
#endif
};  // namespace Assert

namespace Lighting {
/// Number of lights in the current view allowed to affect the scene
const unsigned int MAX_LIGHTS_IN_VIEW = 16;
// How many lights (in order as passed to the shader for the node) should cast
/// shadows
const unsigned int MAX_SHADOW_CASTING_LIGHTS_PER_NODE = 2;
/// Used for CSM or PSSM to determine the maximum number of frustum splits
const unsigned int MAX_SPLITS_PER_LIGHT = 4;
/// How many "units" away should a directional light source be from the camera's
/// position
const unsigned int DIRECTIONAL_LIGHT_DISTANCE = 500;
// Note: since each uniform buffer may at most be 64kb, care must be taken that
// the grid resolution doesnt exceed this
//       e.g. 1920x1200 wont do with 16x16.
const unsigned int LIGHT_GRID_TILE_DIM_X = 64;
const unsigned int LIGHT_GRID_TILE_DIM_Y = 64;
// used for clustered forward
const unsigned int LIGHT_GRID_MAX_DIM_Z = 256;

// Max screen size of 4K
const unsigned int LIGHT_GRID_MAX_DIM_X =
    ((3840 + LIGHT_GRID_TILE_DIM_X - 1) / LIGHT_GRID_TILE_DIM_X);
const unsigned int LIGHT_GRID_MAX_DIM_Y =
    ((2160 + LIGHT_GRID_TILE_DIM_Y - 1) / LIGHT_GRID_TILE_DIM_Y);

/// the maximum number of lights supported, this is limited by constant buffer
/// size, commonly this is 64kb, but AMD only seem to allow 2048 lights...
const unsigned int NUM_POSSIBLE_LIGHTS = 1024;
};  // namespace Lighting
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

/// Enable move semantings in EASTL libraries
#ifndef EA_COMPILER_HAS_MOVE_SEMANTICS
#define EA_COMPILER_HAS_MOVE_SEMANTICS
#endif

#ifndef GPU_VALIDATION_IN_RELEASE_BUILD
//#define GPU_VALIDATION_IN_RELEASE_BUILD
#endif

#ifndef GPU_VALIDATION_IN_PROFILE_BUILD
#define GPU_VALIDATION_IN_PROFILE_BUILD
#endif

#ifndef GPU_VALIDATION_IN_DEBUG_BUILD
#define GPU_VALIDATION_IN_DEBUG_BUILD
#endif

#ifndef ENABLE_GPU_VALIDATION
    #if defined(_RELEASE) && defined(GPU_VALIDATION_IN_RELEASE_BUILD)
    #define ENABLE_GPU_VALIDATION
    #endif

    #if defined(_PROFILE) && defined(GPU_VALIDATION_IN_PROFILE_BUILD)
    #define ENABLE_GPU_VALIDATION
    #endif

    #if defined(_DEBUG) && defined(GPU_VALIDATION_IN_DEBUG_BUILD)
    #define ENABLE_GPU_VALIDATION
    #endif
#endif

#endif  //_DIVIDE_CONFIG_H_
