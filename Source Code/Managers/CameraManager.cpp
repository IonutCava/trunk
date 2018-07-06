#include "CameraManager.h"
#include "Rendering/Camera/FreeFlyCamera.h"

Camera* const CameraManager::getActiveCamera()
{
	if(!_camera) 
		_camera = dynamic_cast<Camera*>(_resDB.begin()->second);
	return _camera;
}

void CameraManager::setActiveCamera(const std::string& name)
{
	if(!_resDB.empty()){
		if(_resDB.find(name) != _resDB.end()) 
			_camera = dynamic_cast<Camera*>(_resDB[name]); 
		else _camera = dynamic_cast<Camera*>(_resDB.begin()->second);
	}else{
		_camera = new FreeFlyCamera();
		assert(_camera != NULL);
		add(name, _camera);
	}
}