#include "Headers/CameraManager.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/ParamHandler.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"

namespace Divide {

CameraManager::CameraManager(Kernel* const kernelPtr)
    : FrameListener(),
      _kernelPtr(kernelPtr),
      _camera(nullptr),
      _addNewListener(false) {
    REGISTER_FRAME_LISTENER(this, 0);
}

CameraManager::~CameraManager() {
    UNREGISTER_FRAME_LISTENER(this);
    Console::printfn(Locale::get("CAMERA_MANAGER_DELETE"));
    Console::printfn(Locale::get("CAMERA_MANAGER_REMOVE_CAMERAS"));
    for (CameraPool::value_type& it : _cameraPool) {
        it.second->unload();
    }
    MemoryManager::DELETE_HASHMAP(_cameraPool);
    _cameraPoolGUID.clear();
}

void CameraManager::update(const U64 deltaTime) {
    assert(_camera);
    _camera->update(deltaTime);
}

bool CameraManager::frameStarted(const FrameEvent& evt) {
    assert(_camera);

    if (_addNewListener) {
        Camera* cam = nullptr;
        for (CameraPool::value_type& it : _cameraPool) {
            cam = it.second;
            cam->clearListeners();
            for (const DELEGATE_CBK_PARAM<Camera&>& listener : _updateCameralisteners) {
                cam->addUpdateListener(listener);
            }
        }
        _addNewListener = false;
    }

    return true;
}

void CameraManager::setActiveCamera(Camera* cam) {
    if (_camera) {
        _camera->onDeactivate();
    }

    _camera = cam;
    _camera->onActivate();
    for (const DELEGATE_CBK_PARAM<Camera&>& listener : _changeCameralisteners) {
        listener(*cam);
    }
}

void CameraManager::addNewCamera(const stringImpl& cameraName,
                                 Camera* const camera) {
    if (camera == nullptr) {
        Console::errorfn(Locale::get("ERROR_CAMERA_MANAGER_CREATION"),
                         cameraName.c_str());
        return;
    }

    camera->setIOD(ParamHandler::getInstance().getParam<F32>(
        "postProcessing.anaglyphOffset"));
    camera->setName(cameraName);

    for (const DELEGATE_CBK_PARAM<Camera&>& listener : _updateCameralisteners) {
        camera->addUpdateListener(listener);
    }

    hashAlg::emplace(_cameraPool, cameraName, camera);
    hashAlg::emplace(_cameraPoolGUID, camera->getGUID(), camera);
}

Camera* CameraManager::findCamera(const stringImpl& name) {
    const CameraPool::const_iterator& it = _cameraPool.find(name);
    assert(it != std::end(_cameraPool));

    return it->second;
}

Camera* CameraManager::findCamera(U64 cameraGUID) {
    const CameraPoolGUID::const_iterator& it = _cameraPoolGUID.find(cameraGUID);
    assert(it != std::end(_cameraPoolGUID));

    return it->second;
}
};