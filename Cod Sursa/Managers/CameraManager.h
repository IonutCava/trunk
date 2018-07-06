#include "resource.h"
#ifndef _CAMERA_MANAGER_H
#define _CAMERA_MANAGER_H

#include "Manager.h"
#include "Rendering/Camera/Camera.h"
#include "Utility/Headers/Singleton.h"

class FreeFlyCamera;
SINGLETON_BEGIN_EXT1(CameraManager,Manager)

public:
	Camera* const getActiveCamera();
	void setActiveCamera(const std::string& name);

private:
	CameraManager() {_camera = NULL;}
	Camera* _camera;

SINGLETON_END()

#endif