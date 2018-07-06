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

/**************************************************************************************************************\
*                             Welcome to DIVIDE Engine's config settings!                                     *
* From here you can decide how you want to build your release of the code.
* - Decide how many threads your application can use (maximum upper limit).
* - Show the debug ms-dos window that prints application information
* - Define what the console output log file should be named
* - 
* - 
***************************************************************************************************************/
#ifndef _CONFIG_HEADER_
#define _CONFIG_HEADER_

//Edit the maximum number of concurrent threads that this application may start excluding events.
//Default 5: Rendering + Update + A.I. + Networking + PhysX
#ifndef THREAD_LIMIT
#define THREAD_LIMIT 5
#endif //THREAD_LIMIT

//Comment this out to show the debug console
#ifndef HIDE_DEBUG_CONSOLE
#define HIDE_DEBUG_CONSOLE
#endif //HIDE_DEBUG_CONSOLE

//Please enter the desired log file name
#ifndef OUTPUT_LOG_FILE
#define OUTPUT_LOG_FILE "console.log"
#endif //OUTPUT_LOG_FILE

//Show log timestamps
#ifndef SHOW_LOG_TIMESTAMPS
#define SHOW_LOG_TIMESTAMPS
#endif //SHOW_LOG_TIMESTAMPS

//Reduce Build time on Windows Platform
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif //VC_EXTRALEAN

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif //WIN32_LEAN_AND_MEAN

//Use SSE functions for math calculations: usefull for release
#ifndef USE_MATH_SSE
//#define USE_MATH_SSE
#endif //USE_MATH_SSE

//Use boost or std::tr1 unordered_map
#ifndef UNORDERED_MAP
#define UNORDERED_MAP BOOST
//#define UNORDERED_MAP TR1
#endif //UNORDERED_MAP

//Use boost or std for_each
#ifndef FOR_EACH_IMPLEMENTATION
#define FOR_EACH_IMPLEMENTATION BOOST
//ToDo: Define a macro for this using lambda expressions. Not supported in VS2008 - Ionut
//#define FOR_EACH_IMPLEMENTATION STD 
#endif //FOR_EACH_IMPLEMENTATION

#endif //_CONFIG_HEADER