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
    
    WriteLock w_lock(_cameraPoolLock);
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
        camera = MemoryManager_NEW FirstPersonCamera(cameraName);
        break;
    case Camera::CameraType::FREE_FLY:
        camera = MemoryManager_NEW FreeFlyCamera(cameraName);
        break;
    case Camera::CameraType::ORBIT:
        camera = MemoryManager_NEW OrbitCamera(cameraName);
        break;
    case Camera::CameraType::SCRIPTED:
        camera = MemoryManager_NEW ScriptedCamera(cameraName);
        break;
    case Camera::CameraType::THIRD_PERSON:
        camera = MemoryManager_NEW ThirdPersonCamera(cameraName);
        break;
    }

    if (camera != nullptr) {
        addNewCamera(camera);
    }

    return camera;
}

bool CameraManager::destroyCamera(Camera*& camera) {
    if (camera != nullptr) {
        if (_camera && _camera->getGUID()  == camera->getGUID()) {
            setActiveCamera(nullptr);
        }

        if (camera->unload() ) {
            WriteLock w_lock(_cameraPoolLock);
            _cameraPool.erase(_ID_RT(camera->getName()));
            _cameraPoolGUID.erase(camera->getGUID());
            MemoryManager::DELETE(camera);
            return true;
        }
    }

    return false;
}

void CameraManager::update(const U64 deltaTime) {
    assert(_camera);
    _camera->update(deltaTime);
}

bool CameraManager::frameStarted(const FrameEvent& evt) {
    assert(_camera);

    if (_addNewListener) {
        Camera* cam = nullptr;
        ReadLock r_lock(_cameraPoolLock);
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
    if (_camera) {
        _camera->onActivate();
        for (const DELEGATE_CBK_PARAM<Camera&>& listener : _changeCameralisteners) {
            listener(*cam);
        }
    }
}

void CameraManager::addNewCamera(Camera* const camera) {
    if (camera == nullptr) {
        Console::errorfn(Locale::get(_ID("ERROR_CAMERA_MANAGER_CREATION")));
        return;
    }

    for (const DELEGATE_CBK_PARAM<Camera&>& listener : _updateCameralisteners) {
        camera->addUpdateListener(listener);
    }

    WriteLock w_lock(_cameraPoolLock);
    hashAlg::emplace(_cameraPool, _ID_RT(camera->getName()), camera);
    hashAlg::emplace(_cameraPoolGUID, camera->getGUID(), camera);
}

Camera* CameraManager::findCamera(ULL nameHash) {
    ReadLock r_lock(_cameraPoolLock);
    const CameraPool::const_iterator& it = _cameraPool.find(nameHash);
    if(it != std::end(_cameraPool)) {
        return it->second;
    }

    return nullptr;
}
};