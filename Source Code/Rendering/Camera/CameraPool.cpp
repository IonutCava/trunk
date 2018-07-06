#include "Headers/Camera.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Rendering/Camera/Headers/FirstPersonCamera.h"
#include "Rendering/Camera/Headers/OrbitCamera.h"
#include "Rendering/Camera/Headers/ScriptedCamera.h"
#include "Rendering/Camera/Headers/ThirdPersonCamera.h"

namespace Divide {

const char* Camera::DefaultCamera = "defaultCamera";
const U64 Camera::DefaultCameraHash = _ID(Camera::DefaultCamera);

bool Camera::_addNewListener = false;
Camera* Camera::_activeCamera = nullptr;
Camera* Camera::_previousCamera = nullptr;

vectorImpl<DELEGATE_CBK_PARAM<Camera&> > Camera::_changeCameralisteners;
vectorImpl<DELEGATE_CBK_PARAM<Camera&> > Camera::_updateCameralisteners;

SharedLock Camera::_cameraPoolLock;
Camera::CameraPool Camera::_cameraPool;
Camera::CameraPoolGUID Camera::_cameraPoolGUID;

void Camera::update(const U64 deltaTime) {
    if (_addNewListener) {
        Camera* cam = nullptr;
        ReadLock r_lock(_cameraPoolLock);
        for (CameraPool::value_type& it : _cameraPool) {
            cam = it.second;
            cam->clearListeners();
            for (const DELEGATE_CBK_PARAM<Camera&>& listener : _updateCameralisteners) {
                cam->addUpdateListenerInternal(listener);
            }
        }
        _addNewListener = false;
    }

    ReadLock r_lock(_cameraPoolLock);
    for (CameraPool::value_type& it : _cameraPool) {
        it.second->updateInternal(deltaTime);
    }
}

void Camera::activeCamera(Camera* cam) {
    Camera* activeCam = activeCamera();
    if (activeCam) {
        if (cam && activeCam->getGUID() == cam->getGUID()) {
            return;
        }
        activeCam->setActiveInternal(false);
    }

    _previousCamera = _activeCamera;
    _activeCamera = cam;
    if (cam) {
        _activeCamera->setActiveInternal(true);
        for (const DELEGATE_CBK_PARAM<Camera&>& listener : _changeCameralisteners) {
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
    return _activeCamera;
}

bool Camera::mouseMoved(const Input::MouseEvent& arg) {
    return activeCamera() ? activeCamera()->mouseMovedInternal(arg) : false;
}

void Camera::destroyPool() {
    Console::printfn(Locale::get(_ID("CAMERA_MANAGER_DELETE")));
    Console::printfn(Locale::get(_ID("CAMERA_MANAGER_REMOVE_CAMERAS")));

    WriteLock w_lock(_cameraPoolLock);
    for (CameraPool::value_type& it : _cameraPool) {
        it.second->unload();
    }
    MemoryManager::DELETE_HASHMAP(_cameraPool);
    _cameraPoolGUID.clear();
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
        for (const DELEGATE_CBK_PARAM<Camera&>& listener : _updateCameralisteners) {
            camera->addUpdateListenerInternal(listener);
        }

        WriteLock w_lock(_cameraPoolLock);
        hashAlg::emplace(_cameraPool, _ID_RT(camera->getName()), camera);
        hashAlg::emplace(_cameraPoolGUID, camera->getGUID(), camera);
    }

    return camera;
}

bool Camera::destroyCamera(Camera*& camera) {
    if (camera != nullptr) {
        Camera* activeCam = activeCamera();
        if (activeCam && activeCam->getGUID() == camera->getGUID()) {
            activeCamera(nullptr);
        }

        if (camera->unload()) {
            WriteLock w_lock(_cameraPoolLock);
            _cameraPool.erase(_ID_RT(camera->getName()));
            _cameraPoolGUID.erase(camera->getGUID());
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

void Camera::addChangeListener(const DELEGATE_CBK_PARAM<Camera&>& f) {
    _changeCameralisteners.push_back(f);
}

void Camera::addUpdateListener(const DELEGATE_CBK_PARAM<Camera&>& f) {
    _updateCameralisteners.push_back(f);
    _addNewListener = true;
}

}; //namespace Divide