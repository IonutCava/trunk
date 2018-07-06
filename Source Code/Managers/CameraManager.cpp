#include "Headers/CameraManager.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"

CameraManager::CameraManager() : _camera(NULL) {
}

CameraManager::~CameraManager() {
	PRINT_FN("Deleting Camera Pool ...");
	PRINT_FN("Removing all cameras and destroying the camera pool ...");
	CameraPool::iterator& it = _cameraPool.begin();
	for_each(CameraPool::value_type& it, _cameraPool){
		it.second->unload();
		SAFE_DELETE(it.second);
	}
	_cameraPool.clear();
	
}
Camera* const CameraManager::getActiveCamera() {
	if(!_camera && !_cameraPool.empty()) 
		_camera = _cameraPool.begin()->second;
	return _camera;
}

void CameraManager::setActiveCamera(const std::string& name) {
	if(!_cameraPool.empty()){
		if(_cameraPool.find(name) != _cameraPool.end()) 
			_camera = _cameraPool[name]; 
		else _camera = _cameraPool.begin()->second;
	}else{
		_camera = New FreeFlyCamera();
		assert(_camera != NULL);
		addNewCamera(name, _camera);
	}
}


void CameraManager::addNewCamera(const std::string& cameraName, Camera* const camera){
	if(camera == NULL) {
		ERROR_FN("CameraManare: Camera [ %s ] creation failed!",cameraName.c_str());
		return;
	}
	_cameraPool.insert(make_pair(cameraName,camera));
}
