/*
   Copyright (c) 2013 DIVIDE-Studio
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

///Application desired framerate for physics simulations
#define TARGET_FRAME_RATE 60
///Application update rate
#define TICKS_PER_SECOND 25
///Maximum frameskip
#define MAX_FRAMESKIP 5
///Minimum triangle count for a mesh to apply depth rendering optimisations
#define DEPTH_VBO_MIN_TRIANGLES 1000
///Minimum vbo size in bytes for a mesh to apply depth rendering optimisations (4MB default)
#define DEPTH_VBO_MIN_BYTES 4 * 1024 * 1024

#define TARGET_D3D_VERSION D3D11 /*or D3D10*/

///How many lights should affect a single node
#define MAX_LIGHTS_PER_SCENE_NODE 4
#define MAX_SHADOW_CASTING_LIGHTS_PER_NODE 2

///Terrain LOD configuration
///Camera distance to the terrain chunk is calculated as follows:
///	vector EyeToChunk = terrainBoundingBoxCenter - EyePos; cameraDistance = EyeToChunk.length();
#define TERRAIN_CHUNKS_LOD 3 //< Number of LOD levels for the terrain
#define TERRAIN_CHUNK_LOD0 100.0f //< Relative distance for LOD0->LOD1 selection
#define TERRAIN_CHUNK_LOD1 180.0f //< Relative distance for LOD0->LOD2 selection

///SceneNode LOD selection
///Distance computation is identical to the of the terrain (using SceneNode's bounding box)
#define SCENE_NODE_LOD 3
#define SCENE_NODE_LOD0 100.0f //< Relative distance for LOD0->LOD1 selection
#define SCENE_NODE_LOD1 180.0f //< Relative distance for LOD0->LOD2 selection

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

//#define HAVE_POSIX_MEMALIGN
#define HAVE_ALIGNED_MALLOC
#ifdef HAVE_ALIGNED_MALLOC
//#define __GNUC__
#endif

///Edit the maximum number of concurrent threads that this application may start excluding tasks.
///Default 2 without: Rendering + Update + A.I. + Networking + PhysX
#ifndef THREAD_LIMIT
#define THREAD_LIMIT 2
#endif //THREAD_LIMIT

///Comment this out to show the debug console
#ifndef HIDE_DEBUG_CONSOLE
#define HIDE_DEBUG_CONSOLE
#endif //HIDE_DEBUG_CONSOLE

///Please enter the desired log file name
#ifndef OUTPUT_LOG_FILE
#define OUTPUT_LOG_FILE "console.log"
#endif //OUTPUT_LOG_FILE

#ifndef ERROR_LOG_FILE
#define ERROR_LOG_FILE "errors.log"
#endif //ERROR_LOG_FILE

///Show log timestamps
#ifndef SHOW_LOG_TIMESTAMPS
#define SHOW_LOG_TIMESTAMPS
#endif //SHOW_LOG_TIMESTAMPS

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

#endif //_CONFIG_HEADER