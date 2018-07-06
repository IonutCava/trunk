/*“Copyright 2009-2011 DIVIDE-Studio”*/
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

#pragma warning(disable:4244)
#pragma warning(disable:4996) //strcpy
 
#define GETTIME()   Framerate::getInstance().getElapsedTime()/1000
#define GETMSTIME() Framerate::getInstance().getElapsedTime()

#define CLAMP(n, min, max) (((n)<(min))?(min):(((n)>(max))?(max):(n)))
#define BIT(x) (1 << (x))

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifdef WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <assert.h>
#include <memory.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>
#include <map>
#include <math.h>
#include <vector>
#include <deque>
#include <list>
#include <time.h>
#if defined UNORDERED_MAP == BOOST
#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>
#define unordered_map boost::unordered_map
#define foreach         BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH
#else
#include <unordered_map>
#include <algorithm>
#define unordered_map std::tr1::unordered_map
//ToDo: fix these 3 to use std::for_each and lambda expressions
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH 
#define reverse_foreach BOOST_REVERSE_FOREACH
#endif

#include "Utility/Headers/MathClasses.h"
#include "Utility/Headers/Console.h"
#include "Rendering/Framerate.h" //For time management

#define NEW_PARAM (__FILE__, __LINE__)
#define PLACEMENTNEW_PARAM ,__FILE__, __LINE__
#define NEW_DECL , char* zFile, int nLine

void* operator new(size_t t ,char* zFile, int nLine);
void operator delete(void * pxData ,char* zFile, int nLine);

#define New new NEW_PARAM
#define PNew(macroparam) new (macroparam PLACEMENTNEW_PARAM)
#define RemoveResource(res) ResourceManager::getInstance().removeResource(res);
#endif
