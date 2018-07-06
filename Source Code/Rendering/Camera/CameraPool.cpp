#include "Headers/Camera.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Rendering/Camera/Headers/FirstPersonCamera.h"
#include "Rendering/Camera/Headers/OrbitCamera.h"
#include "Rendering/Camera/Headers/ScriptedCamera.h"
#include "Rendering/Camera/Headers/ThirdPersonCamera.h"

namespace Divide {

Camera* Camera::_activeCamera = nullptr;
Camera* Camera::_previousCamera = nullptr;

vectorImpl<DELEGATE_CBK<void, const Camera&> > Camera::_changeCameralisteners;
vectorImpl<DELEGATE_CBK<void, const Camera&> > Camera::_updateCameralisteners;

SharedLock Camera::_cameraPoolLock;
Camera::CameraPool Camera::_cameraPool;

void Camera::update(const U64 deltaTime) {
    ReadLock r_lock(_cameraPoolLock);
    for (CameraPool::value_type& it : _cameraPool) {
        it.second->updateInternal(deltaTime);
    }
}

void Camera::onUpdate(const Camera& cam) {
    for (const DELEGATE_CBK<void, const Camera&>& listener : _updateCameralisteners) {
        listener(cam);
    }
}

void Camera::activeCamera(Camera* cam) {
    if (_activeCamera) {
        if (cam && _activeCamera->getGUID() == cam->getGUID()) {
            return;
        }
        _activeCamera->setActiveInternal(false);
    }

    _previousCamera = _activeCamera;
    _activeCamera = cam;
    if (cam) {
        _activeCamera->setActiveInternal(true);
        for (const DELEGATE_CBK<void, Camera&>& listener : _changeCameralisteners) {
            listener(*_activeCamera);
        }
    }
}

void Camera::activeCamera(U64 cam) {
    activeCamera(findCamera(cam));
}

Camera* Camera::previousCamera() {
    return _previousCamera;
}

Camera* Camera::activeCamera() {
    assert(_activeCamera != nullptr);

    return _activeCamera;
}

void Camera::destroyPool() {
    Console::printfn(Locale::get(_ID("CAMERA_MANAGER_DELETE")));
    Console::printfn(Locale::get(_ID("CAMERA_MANAGER_REMOVE_CAMERAS")));

    WriteLock w_lock(_cameraPoolLock);
    for (CameraPool::value_type& it : _cameraPool) {
        it.second->unload();
    }
    MemoryManager::DELETE_HASHMAP(_cameraPool);
}

Camera* Camera::createCamera(const stringImpl& cameraName, CameraType type) {
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
        WriteLock w_lock(_cameraPoolLock);
        hashAlg::emplace(_cameraPool, _ID_RT(camera->getName()), camera);
    }

    return camera;
}

bool Camera::destroyCamera(Camera*& camera) {
    if (camera != nullptr) {
        if (_activeCamera && _activeCamera->getGUID() == camera->getGUID()) {
            activeCamera(nullptr);
        }

        if (camera->unload()) {
            WriteLock w_lock(_cameraPoolLock);
            _cameraPool.erase(_ID_RT(camera->getName()));
            MemoryManager::DELETE(camera);
            return true;
        }
    }

    return false;
}

Camera* Camera::findCamera(U64 nameHash) {
    ReadLock r_lock(_cameraPoolLock);
    const CameraPool::const_iterator& it = _cameraPool.find(nameHash);
    if (it != std::end(_cameraPool)) {
        return it->second;
    }

    return nullptr;
}

void Camera::addChangeListener(const DELEGATE_CBK<void, const Camera&>& f) {
    _changeCameralisteners.push_back(f);
}

void Camera::addUpdateListener(const DELEGATE_CBK<void, const Camera&>& f) {
    _updateCameralisteners.push_back(f);
}

}; //namespace Divide