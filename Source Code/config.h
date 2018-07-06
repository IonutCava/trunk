/*�Copyright 2009-2011 DIVIDE-Studio�*/
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
* - Configure if you want to you a PhysX generated terrain or an openGl generated one
* - Decide how many threads your application can use (maximum upper limit).
* - Show the debug ms-dos window that prints application information
* - Define what the console output log file should be named
* - 
* - 
***************************************************************************************************************/

//Edit the maximum number of concurrent threads that this application may start.
//Default 2: Rendering + PhysX
#ifndef THREAD_LIMIT
#define THREAD_LIMIT 2
#endif

//Comment this out to show the debug console
#ifndef HIDE_DEBUG_CONSOLE
#define HIDE_DEBUG_CONSOLE
#endif

//Please enter the desired log file name
#ifndef OUTPUT_LOG_FILE
#define OUTPUT_LOG_FILE "console.log"
#endif 

//Show log timestamps
#ifndef SHOW_LOG_TIMESTAMPS
#define SHOW_LOG_TIMESTAMPS
#endif 

//Reduce Build time on Windows Platform
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#endif