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

#ifndef PHYSX_H_
#define PHYSX_H_

#include "resource.h"

//PhysX includes
#include <pxphysicsapi.h>
#include <pxdefaulterrorcallback.h>
#include <pxdefaultallocator.h>
//PhysX includes //


//PhysX libraries
#pragma comment(lib, "PhysX3_x86.lib")
#pragma comment(lib, "Foundation")
#pragma comment(lib, "PhysX3Extensions.lib")
//PhysX libraries //

static physx::PxPhysics* gPhysicsSDK = NULL;
static physx::PxDefaultErrorCallback gDefaultErrorCallback;
static physx::PxDefaultAllocator gDefaultAllocatorCallback;

DEFINE_SINGLETON( PhysX )

private:

	PhysX(); 

public:
	
   bool InitNx();  
   void ExitNx(); 
   void idle();

END_SINGLETON

#endif