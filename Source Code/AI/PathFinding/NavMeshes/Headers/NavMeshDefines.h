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

#ifndef _NAVIGATION_MESH_DEFINES_H_
#define _NAVIGATION_MESH_DEFINES_H_

#include "Hardware/Platform/Headers/PlatformDefines.h"

#ifndef RECAST_UTIL_PROPERTIES
#define RECAST_UTIL_PROPERTIES
#endif
#include <ReCast/Include/Recast.h>
#include <Detour/Include/DetourNavMesh.h>
#include <Detour/Include/DetourNavMeshQuery.h>
#include <Detour/Include/DetourNavMeshBuilder.h>

#define MAX_PATHSLOT      128 // how many paths we can store
#define MAX_PATHPOLY      256 // max number of polygons in a path
#define MAX_PATHVERT      512 // most verts in a path 

// Extra padding added to the border size of tiles (together with agent radius)
const F32 BORDER_PADDING = 3;

namespace Navigation {
		/// These are just sample areas to use consistent values across the samples.
	/// The use should specify these base on his needs.
	enum SamplePolyAreas
	{
		SAMPLE_POLYAREA_GROUND,
		SAMPLE_POLYAREA_WATER,
		SAMPLE_POLYAREA_ROAD,
		SAMPLE_POLYAREA_DOOR,
		SAMPLE_POLYAREA_GRASS,
		SAMPLE_POLYAREA_JUMP,
        SAMPLE_AREA_OBSTACLE
	};

	enum SamplePolyFlags
	{
		SAMPLE_POLYFLAGS_WALK		= 0x01,		// Ability to walk (ground, grass, road)
		SAMPLE_POLYFLAGS_SWIM		= 0x02,		// Ability to swim (water).
		SAMPLE_POLYFLAGS_DOOR		= 0x04,		// Ability to move through doors.
		SAMPLE_POLYFLAGS_JUMP		= 0x08,		// Ability to jump.
		SAMPLE_POLYFLAGS_DISABLED	= 0x10,		// Disabled polygon
		SAMPLE_POLYFLAGS_ALL		= 0xffff	// All abilities.
	};

	// structure for storing output straight line paths
	typedef struct
	{
		F32 PosX[MAX_PATHVERT] ;
		F32 PosY[MAX_PATHVERT] ;
		F32 PosZ[MAX_PATHVERT] ;
		I32 MaxVertex ;
		I32 Target ;
	} PATHDATA ;

};

#endif