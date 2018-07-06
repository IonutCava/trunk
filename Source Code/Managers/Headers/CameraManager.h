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

#ifndef _CAMERA_MANAGER_H
#define _CAMERA_MANAGER_H

#include "core.h"
#include <boost/noncopyable.hpp>
#include "Rendering/Camera/Headers/Camera.h"

/// Multiple camera managers can be created if needed in the future
/// No need for singletons here
class CameraManager : private boost::noncopyable {
typedef Unordered_map<std::string, Camera*> CameraPool;

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
	vectorImpl<boost::function0<void> > _listeners;
};

#endif