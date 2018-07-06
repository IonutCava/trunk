#include "Headers/CameraManager.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/ParamHandler.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Rendering/Camera/Headers/FirstPersonCamera.h"
#include "Rendering/Camera/Headers/OrbitCamera.h"
#include "Rendering/Camera/Headers/ScriptedCamera.h"
#include "Rendering/Camera/Headers/ThirdPersonCamera.h"

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
    Console::printfn(Locale::get(_ID("CAMERA_MANAGER_DELETE")));
    Console::printfn(Locale::get(_ID("CAMERA_MANAGER_REMOVE_CAMERAS")));
    for (CameraPool::value_type& it : _cameraPool) {
        it.second->unload();
    }
    MemoryManager::DELETE_HASHMAP(_cameraPool);
    _cameraPoolGUID.clear();
}

Camera* CameraManager::createCamera(const stringImpl& cameraName,
                                    Camera::CameraType type) {
    Camera* camera = nullptr;
    switch (type) {
    case Camera::CameraType::FIRST_PERSON:
        camera = MemoryManager_NEW FirstPersonCamera();
        break;
    case Camera::CameraType::FREE_FLY:
        camera = MemoryManager_NEW FreeFlyCamera();
        break;
    case Camera::CameraType::ORBIT:
        camera = MemoryManager_NEW OrbitCamera();
        break;
    case Camera::CameraType::SCRIPTED:
        camera = MemoryManager_NEW ScriptedCamera();
        break;
    case Camera::CameraType::THIRD_PERSON:
        camera = MemoryManager_NEW ThirdPersonCamera();
        break;
    }

    if (camera != nullptr) {
        addNewCamera(cameraName, camera);
    }

    return camera;
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
        Console::errorfn(Locale::get(_ID("ERROR_CAMERA_MANAGER_CREATION")),
                         cameraName.c_str());
        return;
    }

    camera->setIOD(ParamHandler::getInstance().getParam<F32>(_ID("postProcessing.anaglyphOffset")));
    camera->setName(cameraName);

    for (const DELEGATE_CBK_PARAM<Camera&>& listener : _updateCameralisteners) {
        camera->addUpdateListener(listener);
    }

    hashAlg::emplace(_cameraPool, _ID_RT(cameraName), camera);
    hashAlg::emplace(_cameraPoolGUID, camera->getGUID(), camera);
}

Camera* CameraManager::findCamera(ULL nameHash) {
    const CameraPool::const_iterator& it = _cameraPool.find(nameHash);
    if(it != std::end(_cameraPool)) {
        return it->second;
    }

    return nullptr;
}
};