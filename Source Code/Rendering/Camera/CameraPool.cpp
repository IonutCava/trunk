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

std::array<Camera*, to_base(Camera::UtilityCamera::COUNT)> Camera::_utilityCameras;

U32 Camera::s_changeCameraId = 0;
Camera::ListenerMap Camera::s_changeCameraListeners;

SharedMutex Camera::s_cameraPoolLock;
Camera::CameraPool Camera::s_cameraPool;

void Camera::update(const U64 deltaTimeUS) {
    SharedLock r_lock(s_cameraPoolLock);
    for (CameraPool::value_type& it : s_cameraPool) {
        it.second->updateInternal(deltaTimeUS);
    }
}

vector<U64> Camera::cameraList() {
    vector<U64> ret;
    ret.reserve(s_cameraPool.size());

    SharedLock r_lock(s_cameraPoolLock);
    for (CameraPool::value_type& it : s_cameraPool) {
        ret.push_back(it.first);
    }

    return ret;
}

Camera* Camera::utilityCamera(UtilityCamera type) {
    if (type != UtilityCamera::COUNT) {
        return _utilityCameras[to_base(type)];
    }

    return nullptr;
}

void Camera::initPool() {
    _utilityCameras[to_base(UtilityCamera::_2D)] = Camera::createCamera("2DRenderCamera", Camera::CameraType::FREE_FLY);
    _utilityCameras[to_base(UtilityCamera::_2D_FLIP_Y)] = Camera::createCamera("2DRenderCameraFlipY", Camera::CameraType::FREE_FLY);
    _utilityCameras[to_base(UtilityCamera::CUBE)] = Camera::createCamera("CubeCamera", Camera::CameraType::FREE_FLY);
    _utilityCameras[to_base(UtilityCamera::DUAL_PARABOLOID)] = Camera::createCamera("DualParaboloidCamera", Camera::CameraType::FREE_FLY);
    _utilityCameras[to_base(UtilityCamera::DEFAULT)] = Camera::createCamera("DefaultCamera", Camera::CameraType::FREE_FLY);
}

void Camera::destroyPool() {
    Console::printfn(Locale::get(_ID("CAMERA_MANAGER_DELETE")));
    Console::printfn(Locale::get(_ID("CAMERA_MANAGER_REMOVE_CAMERAS")));

    UniqueLockShared w_lock(s_cameraPoolLock);
    for (CameraPool::value_type& it : s_cameraPool) {
        it.second->unload();
    }
    MemoryManager::DELETE_HASHMAP(s_cameraPool);
    _utilityCameras.fill(nullptr);
}

Camera* Camera::createCamera(const Str64& cameraName, CameraType type) {
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
        UniqueLockShared w_lock(s_cameraPoolLock);
        hashAlg::insert(s_cameraPool, _ID(camera->resourceName().c_str()), camera);
    }

    return camera;
}

bool Camera::destroyCamera(Camera*& camera) {
    if (camera != nullptr && camera->unload()) {
        UniqueLockShared w_lock(s_cameraPoolLock);
        s_cameraPool.erase(_ID(camera->resourceName().c_str()));
        MemoryManager::DELETE(camera);
        return true;
    }

    return false;
}

Camera* Camera::findCamera(U64 nameHash) {
    SharedLock r_lock(s_cameraPoolLock);
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

}; //namespace Divide