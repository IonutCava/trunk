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

#include "core.h"
#ifndef _CAMERA_MANAGER_H
#define _CAMERA_MANAGER_H

#include "BaseCache.h"
#include "Rendering/Camera/Headers/Camera.h"

class FreeFlyCamera;
DEFINE_SINGLETON_EXT1(CameraManager,BaseCache)

public:
	Camera* const getActiveCamera();
	void setActiveCamera(const std::string& name);

private:
	CameraManager() {_camera = NULL;}
	Camera* _camera;

END_SINGLETON

#endif