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

#ifndef _CAMERA_MANAGER_H
#define _CAMERA_MANAGER_H

#include "core.h"
#include <boost/noncopyable.hpp>
#include "Rendering/Camera/Headers/Camera.h"

/// Multiple camera managers can be created if needed in the future
/// No need for singletons here 
class CameraManager : private boost::noncopyable {
typedef unordered_map<std::string, Camera*> CameraPool;

public:
	CameraManager();
	~CameraManager();

	Camera* const getActiveCamera();
	void setActiveCamera(const std::string& name);
	void addNewCamera(const std::string& cameraName, Camera* const camera);

	void addCameraChangeListener(boost::function0<void> f) {_listeners.push_back(f);}

private:
	Camera* _camera;
	CameraPool _cameraPool;
	std::vector<boost::function0<void> > _listeners;
};

#endif