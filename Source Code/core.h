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

#ifndef CORE_H_
#define CORE_H_

#ifdef HIDE_DEBUG_CONSOLE
	#pragma comment( linker,"/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )
#endif

<<<<<<< .mine

=======
>>>>>>> .r140
#define NEW_PARAM (__FILE__, __LINE__)
#define PLACEMENTNEW_PARAM ,__FILE__, __LINE__
#define NEW_DECL , char* zFile, int nLine

void* operator new(size_t t ,char* zFile, int nLine);
void operator delete(void * pxData ,char* zFile, int nLine);

#define New new NEW_PARAM

#ifndef NOMINMAX
#define NOMINMAX
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

#include <boost/function.hpp>                  //For callbacks and delegates
#include "Hardware/Platform/Headers/PlatformDefines.h" //For data types
#include "Hardware/Platform/Headers/Mutex.h"           //For multi-threading
#include "Core/Math/Headers/MathClasses.h"     //For math classes (mat3,mat4,vec2,vec3,vec4 etc)
#include "Rendering/Headers/Framerate.h"       //For time management
#include "Core/Headers/Console.h"              //For printing to the standard output
#include "Utility/Headers/Localization.h"      //For language parsing
#include "Utility/Headers/UnorderedMap.h"      //For language parsing

#define tuple_get_impl  std::tr1::get
#define make_tuple_impl std::tr1::make_tuple
#define tuple_impl      std::tr1::tuple
//#define tuple_get_impl  std::get
//#define make_tuple_impl std::make_tuple
//#define tuple_impl std::tuple

#define PNew(macroparam) new (macroparam PLACEMENTNEW_PARAM)

#define GETTIME()   Framerate::getInstance().getElapsedTime()/1000
#define GETMSTIME() Framerate::getInstance().getElapsedTime()

/// Clamps value n between min and max
template <class T>
inline void CLAMP(T& n, T min, T max){
	n = ((n)<(min))?(min):(((n)>(max))?(max):(n));
}

/// Converts an arbitrary positive integer value to a bitwise value used for masks
#define toBit(X) (1 << (X))

enum ERROR_CODES {
	NO_ERR = 0,
	MISSING_SCENE_DATA = -1,
	GLEW_INIT_ERROR = -2,
	GLEW_OLD_HARDWARE = -3,
	DX_INIT_ERROR = -4,
	DX_OLD_HARDWARE = -5,
	SDL_AUDIO_INIT_ERROR = -6,
	FMOD_AUDIO_INIT_ERROR = -7,
	OAL_INIT_ERROR = -8,
	PHYSX_INIT_ERROR = -9,
	PHYSX_EXTENSION_ERROR = -10,
	NO_LANGUAGE_INI = -11
};

#endif
