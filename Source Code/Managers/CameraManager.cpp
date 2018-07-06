#include "Headers/CameraManager.h"

#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/ParamHandler.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"

namespace Divide {

CameraManager::CameraManager(Kernel* const kernelPtr) : FrameListener(), _kernelPtr(kernelPtr), _camera(nullptr), _addNewListener(false)
{
    REGISTER_FRAME_LISTENER(this, 0);
}

CameraManager::~CameraManager() {
    UNREGISTER_FRAME_LISTENER(this);
    PRINT_FN(Locale::get("CAMERA_MANAGER_DELETE"));
    PRINT_FN(Locale::get("CAMERA_MANAGER_REMOVE_CAMERAS"));
	for (CameraPool::value_type& it : _cameraPool){
        it.second->unload();
        SAFE_DELETE(it.second);
    }
    _cameraPool.clear();
    _cameraPoolGUID.clear();
}

bool CameraManager::frameStarted(const FrameEvent& evt){
    assert(_camera);

    if (_addNewListener){
        Camera* cam = nullptr;
		for (CameraPool::value_type& it : _cameraPool){
            cam = it.second;
            cam->clearListeners();
			for (const DELEGATE_CBK<>& listener : _updateCameralisteners) {
				cam->addUpdateListener(listener);
			}
        }
        _addNewListener = false;
    }

    U64 timeDelta = _kernelPtr->getCurrentTimeDelta();
    _camera->update(timeDelta);
    
    return true;
}

void CameraManager::setActiveCamera(Camera* cam, bool callActivate) {
    if (_camera) _camera->onDeactivate();

    _camera = cam;

    if (callActivate) _camera->onActivate();

	for (const DELEGATE_CBK<>& listener : _changeCameralisteners) {
		listener();
	}
}

void CameraManager::addNewCamera(const stringImpl& cameraName, Camera* const camera){
    if(camera == nullptr) {
        ERROR_FN(Locale::get("ERROR_CAMERA_MANAGER_CREATION"),cameraName.c_str());
        return;
    }

    camera->setIOD(ParamHandler::getInstance().getParam<F32>("postProcessing.anaglyphOffset"));
    camera->setName(cameraName);

	for (const DELEGATE_CBK<>& listener : _updateCameralisteners) {
		camera->addUpdateListener(listener);
	}
    
    hashAlg::emplace(_cameraPool, cameraName, camera);
    hashAlg::emplace(_cameraPoolGUID, camera->getGUID(), camera);
}

Camera* CameraManager::findCamera(const stringImpl& name){
    const CameraPool::const_iterator& it = _cameraPool.find(name);
    assert (it != _cameraPool.end());

    return it->second;
}

Camera* CameraManager::findCamera(U64 cameraGUID) {
    const CameraPoolGUID::const_iterator& it = _cameraPoolGUID.find(cameraGUID);
    assert (it != _cameraPoolGUID.end());

    return it->second;
}

};