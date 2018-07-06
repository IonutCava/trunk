/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

/**************************************************************************************************************\
*                             Welcome to DIVIDE Engine's config settings!                                     *
* From here you can decide how you want to build your release of the code.
* - Decide how many threads your application can use (maximum upper limit).
* - Show the debug ms-dos window that prints application information
* - Define what the console output log file should be named
* - Enable/Disable SSE support
* - Set renderer options
***************************************************************************************************************/
#ifndef _CONFIG_HEADER_
#define _CONFIG_HEADER_

namespace Config
{
    /// How many textures to store per material. bump(0) + opacity(1) + spec(2) + tex[3..MAX_TEXTURE_STORAGE - 1]
    const int MAX_TEXTURE_STORAGE = 6;
    /// Application desired framerate for physics simulations
    const int TARGET_FRAME_RATE = 60;
    /// Application update rate divisor (how many draw calls per render call e.g. 2 = 30Hz update rate at 60Hz rendering)
    const int TICK_DIVISOR = 2;
    ///	Application update rate
    const int TICKS_PER_SECOND = TARGET_FRAME_RATE / TICK_DIVISOR;
    /// Maximum frameskip
    const int MAX_FRAMESKIP = 5;
    /// AI update frequency
    const int AI_THREAD_UPDATE_FREQUENCY = TICKS_PER_SECOND;
    /// Minimum triangle count for a mesh to apply depth rendering optimisations
    const int DEPTH_VBO_MIN_TRIANGLES = 1000;
    /// Minimum vbo size in bytes for a mesh to apply depth rendering optimisations (4MB default)
    const int DEPTH_VBO_MIN_BYTES = 4 * 1024 * 1024;
    /// Maximum number of instances of a single mesh with a single draw call
    const int MAX_INSTANCE_COUNT = 512;
    /// How many clip planes should the shaders us
    const int MAX_CLIP_PLANES = 6;
    /// How many lights should affect a single node
    const int MAX_LIGHTS_PER_SCENE_NODE = 4;
    /// How many lights total to use in the application (32 should be enough)
    const int MAX_LIGHTS_PER_SCENE = 16;
    /// How many lights (in order as passed to the shader for the node) should cast shadows
    const int MAX_SHADOW_CASTING_LIGHTS_PER_NODE = 2;
    /// How many "units" away should a directional light source be from the camera's position
    const int DIRECTIONAL_LIGHT_DISTANCE = 500;
    /// Terrain LOD configuration
    /// Camera distance to the terrain chunk is calculated as follows:
    ///	vector EyeToChunk = terrainBoundingBoxCenter - EyePos; cameraDistance = EyeToChunk.length();
    const int TERRAIN_CHUNKS_LOD = 3; //< Number of LOD levels for the terrain
    const int TERRAIN_CHUNK_LOD0 = 100; //< Relative distance for LOD0->LOD1 selection
    const int TERRAIN_CHUNK_LOD1 = 180; //< Relative distance for LOD0->LOD2 selection

    /// SceneNode LOD selection
    /// Distance computation is identical to the of the terrain (using SceneNode's bounding box)
    const int SCENE_NODE_LOD = 3;
    const int SCENE_NODE_LOD0 = 100; //< Relative distance for LOD0->LOD1 selection
    const int SCENE_NODE_LOD1 = 180; //< Relative distance for LOD0->LOD2 selection

    /// Edit the maximum number of concurrent threads that this application may start excluding tasks.
    /// Default 2 without: Rendering + Update + A.I. + Networking + PhysX
    const int THREAD_LIMIT = 2;

	/// Use HW AA'ed lines
	const bool USE_HARDWARE_AA_LINES = true;
}

/// if this is 0, a variable timestep will be used for the game loop
#define USE_FIXED_TIMESTEP 1

///Direct 3D desired target version
#define TARGET_D3D_VERSION D3D11 /*or D3D10*/

///Comment this out to show the debug console
#ifndef HIDE_DEBUG_CONSOLE
#define HIDE_DEBUG_CONSOLE
#endif //HIDE_DEBUG_CONSOLE

///Current platform
#ifndef _WIN32
#define _WIN32
#endif
#ifndef WIN32
#define WIN32 //for Physx
#endif
#ifndef __APPLE_CC__
//#define __APPLE_CC__
#endif
#ifndef LINUX
//#define LINUX
#endif

///OS specific stuff
#if defined( __WIN32__ ) || defined( _WIN32 )
    #define OS_WINDOWS
#elif defined( __APPLE_CC__ ) // Apple OS X could be supported in the future
    #define OS_APPLE
#else //Linux is the only other OS supported
    #define OS_NIX
#endif

//#define HAVE_POSIX_MEMALIGN
#define HAVE_ALIGNED_MALLOC
#ifdef HAVE_ALIGNED_MALLOC
//#define __GNUC__
#endif

///Please enter the desired log file name
#ifndef OUTPUT_LOG_FILE
#define OUTPUT_LOG_FILE "console.log"
#endif //OUTPUT_LOG_FILE

#ifndef ERROR_LOG_FILE
#define ERROR_LOG_FILE "errors.log"
#endif //ERROR_LOG_FILE

///Reduce Build time on Windows Platform
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif //VC_EXTRALEAN

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif //WIN32_LEAN_AND_MEAN

///Use SSE functions for math calculations: usefull for release
#ifndef USE_MATH_SIMD
//#define USE_MATH_SIMD
#define ALIGNED_BYTES 16
#endif //USE_MATH_SIMD

///Use boost or std::tr1 unordered_map
///0 = BOOST
///1 = TR1
#ifndef UNORDERED_MAP_IMP
#define UNORDERED_MAP_IMP 0
#endif //UNORDERED_MAP_IMP

///Use stlport or stl vector
///0 = Boost
///1 = STL
/// STL note: Due to a bug in STL, fixed in VC++ 11, SIMD and std::vector's do not go along
///           To fix this change line 716 of <vector.h> from
///                       "void resize(size_type _Newsize, _Ty _Val)"
///           to
///                       "void resize(size_type _Newsize,const _Ty& _Val)"
#ifndef VECTOR_IMP
#define VECTOR_IMP 1
#endif //VECTOR_IMP

///Use boost or std for_each
#ifndef FOR_EACH_IMPLEMENTATION
#define FOR_EACH_IMPLEMENTATION BOOST
//#define FOR_EACH_IMPLEMENTATION STD
#endif //FOR_EACH_IMPLEMENTATION

///Disable the use of the PhysXAPI
#ifndef _USE_PHYSX_API_
#define _USE_PHYSX_API_
#endif

///Maximum number of joysticks to use. Remeber to update the "Joystics" enum from InputInterface.h
#ifndef MAX_ALLOWED_JOYSTICKS
#define MAX_ALLOWED_JOYSTICKS 4
#endif

///If the target machine uses the nVidia Optimus layout (IntelHD + nVidia discreet GPU) this will force the engine to use the nVidia GPU always
#ifdef WINDOWS_OS
#define FORCE_NV_OPTIMUS_HIGHPERFORMANCE
#endif
#endif //_CONFIG_HEADER