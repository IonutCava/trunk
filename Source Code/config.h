/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
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
#define  MAX_FRAMESKIP 5

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

#ifndef _DEBUG
	#define NDEBUG
#endif

///Current platform
#ifndef _WIN32
#define _WIN32
#define _WIN32_WINNT=0x501
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

///Edit the maximum number of concurrent threads that this application may start excluding events.
///Default 5: Rendering + Update + A.I. + Networking + PhysX
#ifndef THREAD_LIMIT
#define THREAD_LIMIT 5
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
#define USE_MATH_SIMD
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
///ToDo: Define a macro for this using lambda expressions. Not supported in VS2008 - Ionut
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