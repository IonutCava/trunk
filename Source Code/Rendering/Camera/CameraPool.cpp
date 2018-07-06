#include "stdafx.h"

#include "Headers/Camera.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Rendering/Camera/Headers/FirstPersonCamera.h"
#include "Rendering/Camera/Headers/OrbitCamera.h"
#include "Rendering/Camera/Headers/ScriptedCamera.h"
#include "Rendering/Camera/Headers/ThirdPersonCamera.h"

namespace Divide {

Camera* Camera::s_activeCamera = nullptr;

std::array<Camera*, to_base(Camera::UtilityCamera::COUNT)> Camera::_utilityCameras;

U32 Camera::s_changeCameraId = 0;
U32 Camera::s_updateCameraId = 0;
Camera::ListenerMap Camera::s_changeCameraListeners;
Camera::ListenerMap Camera::s_updateCameraListeners;

SharedLock Camera::s_cameraPoolLock;
Camera::CameraPool Camera::s_cameraPool;

void Camera::update(const U64 deltaTimeUS) {
    ReadLock r_lock(s_cameraPoolLock);
    for (CameraPool::value_type& it : s_cameraPool) {
        it.second->updateInternal(deltaTimeUS);
    }
}

void Camera::onUpdate(const Camera& cam) {
    for (ListenerMap::value_type it : s_updateCameraListeners) {
        it.second(cam);
    }
}

bool Camera::activeCamera(Camera* camera) {
    if (s_activeCamera) {
        if (camera && s_activeCamera->getGUID() == camera->getGUID()) {
            return false;
        }
        s_activeCamera->setActiveInternal(false);
    }

    s_activeCamera = camera;
    if (camera) {
        s_activeCamera->setActiveInternal(true);
        for (ListenerMap::value_type it : s_changeCameraListeners) {
            it.second(*s_activeCamera);
        }
    }

    return true;
}

bool Camera::activeCamera(U64 camera) {
    return activeCamera(findCamera(camera));
}

Camera* Camera::utilityCamera(UtilityCamera type) {
    if (type != UtilityCamera::COUNT) {
        return _utilityCameras[to_base(type)];
    }

    return nullptr;
}

void Camera::initPool(const vec2<U16>& renderResolution) {
    _utilityCameras[to_base(UtilityCamera::_2D)] = Camera::createCamera("2DRenderCamera", Camera::CameraType::FREE_FLY);
    _utilityCameras[to_base(UtilityCamera::_2D_FLIP_Y)] = Camera::createCamera("2DRenderCameraFlipY", Camera::CameraType::FREE_FLY);
    _utilityCameras[to_base(UtilityCamera::CUBE)] = Camera::createCamera("CubeCamera", Camera::CameraType::FREE_FLY);
    _utilityCameras[to_base(UtilityCamera::DUAL_PARABOLOID)] = Camera::createCamera("DualParaboloidCamera", Camera::CameraType::FREE_FLY);

    // Update the 2D camera so it matches our new rendering viewport
    _utilityCameras[to_base(UtilityCamera::_2D)]->setProjection(vec4<F32>(0, to_F32(renderResolution.w), 0, to_F32(renderResolution.h)), vec2<F32>(-1, 1));
    _utilityCameras[to_base(UtilityCamera::_2D_FLIP_Y)]->setProjection(vec4<F32>(0, to_F32(renderResolution.w), to_F32(renderResolution.h), 0), vec2<F32>(-1, 1));
}

void Camera::destroyPool() {
    Console::printfn(Locale::get(_ID("CAMERA_MANAGER_DELETE")));
    Console::printfn(Locale::get(_ID("CAMERA_MANAGER_REMOVE_CAMERAS")));

    WriteLock w_lock(s_cameraPoolLock);
    for (CameraPool::value_type& it : s_cameraPool) {
        it.second->unload();
    }
    MemoryManager::DELETE_HASHMAP(s_cameraPool);
    _utilityCameras.fill(nullptr);
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
        WriteLock w_lock(s_cameraPoolLock);
        hashAlg::insert(s_cameraPool, _ID_RT(camera->getName()), camera);
    }

    return camera;
}

bool Camera::destroyCamera(Camera*& camera) {
    if (camera != nullptr) {
        if (s_activeCamera && s_activeCamera->getGUID() == camera->getGUID()) {
            activeCamera(nullptr);
        }

        if (camera->unload()) {
            WriteLock w_lock(s_cameraPoolLock);
            s_cameraPool.erase(_ID_RT(camera->getName()));
            MemoryManager::DELETE(camera);
            return true;
        }
    }

    return false;
}

Camera* Camera::findCamera(U64 nameHash) {
    ReadLock r_lock(s_cameraPoolLock);
    const CameraPool::const_iterator& it = s_cameraPool.find(nameHash);
    if (it != std::end(s_cameraPool)) {
        return it->second;
    }

    return nullptr;
}

bool Camera::removeChangeListener(U32 id) {
    ListenerMap::const_iterator it = s_changeCameraListeners.find(id);
    if (it != std::cend(s_changeCameraListeners)) {
        s_changeCameraListeners.erase(it);
        return true;
    }

    return false;
}

U32 Camera::addChangeListener(const DELEGATE_CBK<void, const Camera&>& f) {
    hashAlg::insert(s_changeCameraListeners, ++s_changeCameraId, f);
    return s_changeCameraId;
}

bool Camera::removeUpdateListener(U32 id) {
    ListenerMap::const_iterator it = s_updateCameraListeners.find(id);
    if (it != std::cend(s_updateCameraListeners)) {
        s_updateCameraListeners.erase(it);
        return true;
    }

    return false;
}

U32 Camera::addUpdateListener(const DELEGATE_CBK<void, const Camera&>& f) {
    hashAlg::insert(s_updateCameraListeners, ++s_updateCameraId, f);
    return s_updateCameraId;
}

}; //namespace Divide