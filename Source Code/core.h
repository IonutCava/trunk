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

#ifndef CORE_H_
#define CORE_H_

#ifdef HIDE_DEBUG_CONSOLE
	#pragma comment( linker,"/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )
#endif

#define NEW_PARAM (__FILE__, __LINE__)
#define PLACEMENTNEW_PARAM ,__FILE__, __LINE__
#define NEW_DECL , char* zFile, int nLine

void* operator new(size_t t ,char* zFile, int nLine);
void operator delete(void * pxData ,char* zFile, int nLine);
void * malloc_simd(const size_t bytes);
void free_simd(void * pxData);

#define New new NEW_PARAM

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <assert.h>
#include <memory.h>
#include <malloc.h>
#include <typeinfo.h>
#include <time.h>

#include <boost/function.hpp>                          //For callbacks and delegates
#include "Hardware/Platform/Headers/PlatformDefines.h" //For data types
#include "Hardware/Platform/Headers/SharedMutex.h"     //For multi-threading
#include "Core/Math/Headers/MathClasses.h"     //For math classes (mat3,mat4,vec2,vec3,vec4 etc)
#include "Rendering/Headers/Framerate.h"       //For time management
#include "Core/Headers/Console.h"              //For printing to the standard output
#include "Utility/Headers/Localization.h"      //For language parsing
#include "Utility/Headers/UnorderedMap.h"
#include "Utility/Headers/Vector.h"

#define PNew(macroparam) new (macroparam PLACEMENTNEW_PARAM)

#define GETTIME()   getMsToSec(Framerate::getInstance().getElapsedTime())
#define GETMSTIME() Framerate::getInstance().getElapsedTime()

template <class T>
inline T squared(T n){
	return n*n;
}
/// Clamps value n between min and max
template <class T>
inline void CLAMP(T& n, T min, T max){
	n = ((n)<(min))?(min):(((n)>(max))?(max):(n));
}
//Helper method to emulate GLSL
inline F32 fract(F32 floatValue){  return (F32)fmod(floatValue, 1.0f); }
///Packs a floating point value into the [0...255] range (thx sqrt[-1] of opengl.org forums)
inline U8 PACK_FLOAT(F32 floatValue){
	//Scale and bias
  floatValue = (floatValue + 1.0f) * 0.5f;
  return (U8)(floatValue*255.0f);
}
//Pack 3 values into 1 float
inline F32 PACK_FLOAT(U8 x, U8 y, U8 z) {
  U32 packedColor = (x << 16) | (y << 8) | z;
  F32 packedFloat = (F32) ( ((D32)packedColor) / ((D32) (1 << 24)) );
   return packedFloat;
}

//UnPack 3 values from 1 float
inline void UNPACK_FLOAT(F32 src, F32& r, F32& g, F32& b){
  r = fract(src);
  g = fract(src * 256.0f);
  b = fract(src * 65536.0f);

  //Unpack to the -1..1 range
  r = (r * 2.0f) - 1.0f;
  g = (g * 2.0f) - 1.0f;
  b = (b * 2.0f) - 1.0f;
}

/// Converts an arbitrary positive integer value to a bitwise value used for masks
#define toBit(X) (1 << (X))

enum ErrorCodes {
	NO_ERR = 0,
	MISSING_SCENE_DATA = -1,
	MISSING_SCENE_LOAD_CALL = -2,
	GLFW_INIT_ERROR = -3,
	GLFW_WINDOW_INIT_ERROR = -4,
	GLEW_INIT_ERROR = -5,
	GLEW_OLD_HARDWARE = -6,
	DX_INIT_ERROR = -7,
	DX_OLD_HARDWARE = -8,
	SDL_AUDIO_INIT_ERROR = -9,
	FMOD_AUDIO_INIT_ERROR = -10,
	OAL_INIT_ERROR = -11,
	PHYSX_INIT_ERROR = -12,
	PHYSX_EXTENSION_ERROR = -13,
	NO_LANGUAGE_INI = -14
};

///Random stuff added for convenience
#define WHITE() vec4<F32>(1.0f,1.0f,1.0f,1.0f)
#define BLACK() vec4<F32>(0.0f,0.0f,0.0f,1.0f)
#define RED()   vec4<F32>(1.0f,0.0f,0.0f,1.0f)
#define GREEN() vec4<F32>(0.0f,1.0f,0.0f,1.0f)
#define BLUE()  vec4<F32>(0.0f,0.0f,1.0f,1.0f)
#define DIVIDE_BLUE() vec4<F32>(0.1f,0.1f,0.8f,1.0f)

#endif
