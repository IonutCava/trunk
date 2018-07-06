#include "Headers/CameraManager.h"

#include "Core/Headers/ParamHandler.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"

CameraManager::CameraManager() : _camera(nullptr)
{
}

CameraManager::~CameraManager() {
    PRINT_FN(Locale::get("CAMERA_MANAGER_DELETE"));
    PRINT_FN(Locale::get("CAMERA_MANAGER_REMOVE_CAMERAS"));
    FOR_EACH(CameraPool::value_type& it, _cameraPool){
        it.second->unload();
        SAFE_DELETE(it.second);
    }
    _cameraPool.clear();
}

void CameraManager::update(const U64 deltaTime){
    _camera->update(deltaTime);
}

bool CameraManager::onMouseMove(const OIS::MouseEvent& arg){
    _camera->onMouseMove(arg);

    return true;
}

Camera* const CameraManager::getActiveCamera() {
    if(!_camera && !_cameraPool.empty())
        _camera = _cameraPool.begin()->second;
    return _camera;
}

void CameraManager::setActiveCamera(const std::string& name) {
    assert(!_cameraPool.empty());

    if(_camera) _camera->onDectivate();

    if(_cameraPool.find(name) != _cameraPool.end()) 	_camera = _cameraPool[name];
    else  		                                        _camera = _cameraPool.begin()->second;

    _camera->onActivate();

    for(const DELEGATE_CBK& listener : _changeCameralisteners){
        listener();
    }
}

void CameraManager::addNewCamera(const std::string& cameraName, Camera* const camera){
    if(camera == nullptr) {
        ERROR_FN(Locale::get("ERROR_CAMERA_MANAGER_CREATION"),cameraName.c_str());
        return;
    }
    camera->setIOD(ParamHandler::getInstance().getParam<F32>("postProcessing.anaglyphOffset"));
    camera->setName(cameraName);

    for(const DELEGATE_CBK& listener : _updateCameralisteners){
        camera->addUpdateListener(listener);
    }
    _cameraPool.insert(make_pair(cameraName,camera));
}