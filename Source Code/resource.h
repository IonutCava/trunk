/*“Copyright 2009-2012 DIVIDE-Studio”*/
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

#ifndef RESOURCE_H_
#define RESOURCE_H_

#ifdef HIDE_DEBUG_CONSOLE
	#pragma comment( linker,"/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )
#endif

#ifndef _DEBUG
	#define NDEBUG
#endif

#pragma warning(disable:4244)
#pragma warning(disable:4996) ///< strcpy
 
#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifdef WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <assert.h>
#include <memory.h>
#include <memory.h>
#include <malloc.h>
#include <map>
#include <math.h>
#include <vector>
#include <deque>
#include <list>
#include <typeinfo.h>
#include <time.h>

#if defined(UNORDERED_MAP_IMP) && UNORDERED_MAP_IMP == 0

#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>
#define unordered_map boost::unordered_map
#define for_each         BOOST_FOREACH
#define reverse_for_each BOOST_REVERSE_FOREACH

#else

#include <unordered_map>
#include <algorithm>
#define unordered_map std::tr1::unordered_map
///ToDo: fix these 3 to use std::for_each and lambda expressions
#include <boost/foreach.hpp>
#define for_each BOOST_FOREACH 
#define reverse_for_each BOOST_REVERSE_FOREACH


#endif

#include "Hardware/Platform/PlatformDefines.h" //For data types
#include "Core/Math/Headers/MathClasses.h"     //For math classes (mat3,mat4,vec2,vec3,vec4 etc)
#include "Rendering/Headers/Framerate.h"       //For time management
#include "Core/Headers/Console.h"              //For printing to the standard output

#define NEW_PARAM (__FILE__, __LINE__)
#define PLACEMENTNEW_PARAM ,__FILE__, __LINE__
#define NEW_DECL , char* zFile, int nLine

void* operator new(size_t t ,char* zFile, int nLine);
void operator delete(void * pxData ,char* zFile, int nLine);

#define New new NEW_PARAM
#define PNew(macroparam) new (macroparam PLACEMENTNEW_PARAM)

#define GETTIME()   Framerate::getInstance().getElapsedTime()/1000
#define GETMSTIME() Framerate::getInstance().getElapsedTime()

/// Clamps value n between min and max
template <class T>
inline void CLAMP(T& n, T& min, T& max){
	((n)<(min))?(min):(((n)>(max))?(max):(n))
}

/// Converts an arbitrary positive integer value to a bitwise value used for masks
#define toBit(X) (1 << (X))

/// Converts a point from world coordinates to projection coordinates
///(from Y = depth, Z = up to Y = up, Z = depth)
inline void projectPoint(const vec3& position,vec3& output){
	output.x = position.x;
	output.y = position.z;
	output.z = position.y;
}
/// Converts a point from world coordinates to projection coordinates
///(from Y = depth, Z = up to Y = up, Z = depth)
inline void projectPoint(const ivec3& position, ivec3& output){
	output.x = position.x;
	output.y = position.z;
	output.z = position.y;
}


#endif
