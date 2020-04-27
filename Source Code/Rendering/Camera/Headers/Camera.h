/*
   Copyright (c) 2018 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#pragma once
#ifndef _CAMERA_H
#define _CAMERA_H

#include "Frustum.h"
#include "CameraSnapshot.h"

#include "Core/Resources/Headers/Resource.h"

namespace Divide {

class GFXDevice;
class Camera : public Resource {
   public:
    enum class CameraType : U8 {
        FREE_FLY = 0,
        STATIC,
        FIRST_PERSON,
        THIRD_PERSON,
        ORBIT,
        SCRIPTED,
        COUNT
    };

    enum class UtilityCamera : U8 {
        _2D = 0,
        _2D_FLIP_Y,
        DEFAULT,
        CUBE,
        DUAL_PARABOLOID,
        COUNT
    };

   public:
     virtual ~Camera() = default;

    virtual void fromCamera(const Camera& camera, bool flag = false);
    virtual void fromSnapshot(const CameraSnapshot& snapshot);
    const CameraSnapshot& snapshot() const noexcept;

    // Return true if the cached camera state wasn't up-to-date
    bool updateLookAt();
    void setReflection(const Plane<F32>& reflectionPlane);
    void clearReflection() noexcept;

    /// Global rotations are applied relative to the world axis, not the camera's
    virtual void setGlobalRotation(F32 yaw, F32 pitch, F32 roll = 0.0f);
    inline void setGlobalRotation(const vec3<Angle::DEGREES<F32>>& euler) { setGlobalRotation(euler.yaw, euler.pitch, euler.roll); }

    const mat4<F32>& lookAt(const mat4<F32>& viewMatrix);
    /// Sets the camera's position, target and up directions
    const mat4<F32>& lookAt(const vec3<F32>& eye, const vec3<F32>& target, const vec3<F32>& up);
    /// Sets the camera to point at the specified target point
    inline const mat4<F32>& lookAt(const vec3<F32>& target) { return lookAt(_data._eye, target); }
    inline const mat4<F32>& lookAt(const vec3<F32>& eye, const vec3<F32>& target) { return lookAt(eye, target, getUpDir()); }

    /// Sets the camera's Yaw angle.
    /// This creates a new orientation quaternion for the camera and extracts the Euler angles
    inline void setYaw(Angle::DEGREES<F32> angle) { setRotation(angle, _euler.pitch, _euler.roll); }
    /// Sets the camera's Pitch angle. Yaw and Roll are previous extracted values
    inline void setPitch(Angle::DEGREES<F32> angle) { setRotation(_euler.yaw, angle, _euler.roll); }
    /// Sets the camera's Roll angle. Yaw and Pitch are previous extracted values
    inline void setRoll(Angle::DEGREES<F32> angle) { setRotation(_euler.yaw, _euler.pitch, angle); }
    /// Sets the camera's Yaw angle.
    /// This creates a new orientation quaternion for the camera and extracts the Euler angles
    inline void setGlobalYaw(Angle::DEGREES<F32> angle) { setGlobalRotation(angle, _euler.pitch, _euler.roll); }
    /// Sets the camera's Pitch angle. Yaw and Roll are previous extracted values
    inline void setGlobalPitch(Angle::DEGREES<F32> angle) { setGlobalRotation(_euler.yaw, angle, _euler.roll); }
    /// Sets the camera's Roll angle. Yaw and Pitch are previous extracted values
    inline void setGlobalRoll(Angle::DEGREES<F32> angle) { setGlobalRotation(_euler.yaw, _euler.pitch, angle); }

    inline void setEye(F32 x, F32 y, F32 z) noexcept { _data._eye.set(x, y, z); _viewMatrixDirty = true; }
    inline void setEye(const vec3<F32>& position) noexcept { setEye(position.x, position.y, position.z); }

    inline void setRotation(const Quaternion<F32>& q) { _data._orientation = q; _viewMatrixDirty = true; }
    inline void setRotation(Angle::DEGREES<F32> yaw, Angle::DEGREES<F32> pitch, Angle::DEGREES<F32> roll = 0.0f) { setRotation(Quaternion<F32>(-pitch, -yaw, -roll)); }

    inline void setEuler(const vec3<Angle::DEGREES<F32>>& euler) { setRotation(euler.yaw, euler.pitch, euler.roll); }

    void setAspectRatio(F32 ratio) noexcept;
    inline const F32 getAspectRatio() const noexcept { return _data._aspectRatio; }

    void setVerticalFoV(Angle::DEGREES<F32> verticalFoV) noexcept;
    inline const Angle::DEGREES<F32> getVerticalFoV() const noexcept { return _data._FoV; }

    void setHorizontalFoV(Angle::DEGREES<F32> horizontalFoV) noexcept;
    inline const Angle::DEGREES<F32> getHorizontalFoV() const noexcept {
        const Angle::RADIANS<F32> halfFoV = Angle::to_RADIANS(_data._FoV) * 0.5f;
        return Angle::to_DEGREES(2.0f * std::atan(tan(halfFoV) * _data._aspectRatio));
    }

    inline const CameraType& type() const noexcept { return _type; }
    inline const vec3<F32>& getEye() const noexcept { return _data._eye; }
    inline const vec3<Angle::DEGREES<F32>>& getEuler() const noexcept { return _euler; }
    inline const Quaternion<F32>& getOrientation() const noexcept { return _data._orientation; }
    inline const vec2<F32>& getZPlanes() const noexcept { return _data._zPlanes; }
    inline const vec4<F32>& orthoRect() const noexcept { return _orthoRect; }
    inline bool isOrthoProjected() const noexcept { return _isOrthoCamera; }

    inline vec3<F32> getUpDir() const noexcept {
        const mat4<F32>& viewMat = getViewMatrix();
        return vec3<F32>(viewMat.m[0][1], viewMat.m[1][1], viewMat.m[2][1]);
    }

    inline vec3<F32> getRightDir() const noexcept {
        const mat4<F32>& viewMat = getViewMatrix();
        return vec3<F32>(viewMat.m[0][0], viewMat.m[1][0], viewMat.m[2][0]);
    }

    inline vec3<F32> getForwardDir() const noexcept {
        const mat4<F32>& viewMat = getViewMatrix();
        return vec3<F32>(-viewMat.m[0][2], -viewMat.m[1][2], -viewMat.m[2][2]);
    }

    inline const mat4<F32>& getViewMatrix() const noexcept { return _data._viewMatrix; }
    inline const mat4<F32>& getViewMatrix() { updateViewMatrix(); return _data._viewMatrix; }

    inline const mat4<F32>& getProjectionMatrix() const noexcept { return _data._projectionMatrix; }
    inline const mat4<F32>& getProjectionMatrix() { updateProjection(); return _data._projectionMatrix; }

    inline mat4<F32> getWorldMatrix() { return getViewMatrix().getInverse(); }
    inline mat4<F32> getWorldMatrix() const { return getViewMatrix().getInverse(); }
    inline void getWorldMatrix(mat4<F32>& worldMatOut) { getViewMatrix().getInverse(worldMatOut); }
    inline void getWorldMatrix(mat4<F32>& worldMatOut) const { getViewMatrix().getInverse(worldMatOut); }

    /// Nothing really to unload
    virtual bool unload() noexcept { return true; }

    const mat4<F32>& setProjection(F32 aspectRatio, F32 verticalFoV, const vec2<F32>& zPlanes);
    const mat4<F32>& setProjection(const vec4<F32>& rect, const vec2<F32>& zPlanes);
    const mat4<F32>& setProjection(const mat4<F32>& projection, const vec2<F32>& zPlanes, bool isOrtho) noexcept;

    /// Extract the frustum associated with our current PoV
    virtual bool updateFrustum();
    /// Get the camera's current frustum
    inline const Frustum& getFrustum() const noexcept { assert(!_frustumDirty); return _frustum; }
    inline Frustum& getFrustum() noexcept { assert(!_frustumDirty); return _frustum; }
    inline void lockFrustum(bool state) noexcept { _frustumLocked = state; }

    /// Get the world space position from the specified screen coordinates
    /// (use winCoords.z for depth from 0 to 1)
    vec3<F32> unProject(F32 winCoordsX, F32 winCoordsY, F32 winCoordsZ, const Rect<I32>& viewport) const;
    inline vec3<F32> unProject(const vec3<F32>& winCoords, const Rect<I32>& viewport) const { return unProject(winCoords.x, winCoords.y, winCoords.z, viewport); }
    vec2<F32> project(const vec3<F32>& worldCoords, const Rect<I32>& viewport) const;

    bool removeUpdateListener(U32 id);
    U32 addUpdateListener(const DELEGATE<void, const Camera& /*updated camera*/>& f);

   protected:
    virtual bool updateViewMatrix();
    virtual bool updateProjection();
    /// Inject mouse events
    virtual void updateInternal(const U64 deltaTimeUS, const F32 deltaTimeS) noexcept;

    const char* getResourceTypeName() const noexcept override { return "Camera"; }

    inline bool dirty() const noexcept { return _projectionDirty || _viewMatrixDirty || _frustumDirty; }
   protected:
    SET_DELETE_FRIEND
    SET_DELETE_HASHMAP_FRIEND
    explicit Camera(const Str128& name, const CameraType& type, const vec3<F32>& eye = VECTOR3_ZERO);

   protected:
     using ListenerMap = hashMap<U32, DELEGATE<void, const Camera&>>;
     ListenerMap _updateCameraListeners;

    CameraSnapshot _data;
    Frustum _frustum;
    Plane<F32> _reflectionPlane;
    vec4<F32> _orthoRect = VECTOR4_UNIT;
    vec3<Angle::DEGREES<F32>> _euler = VECTOR3_ZERO;
    Angle::DEGREES<F32> _accumPitchDegrees = 0.0f;

    U32 _updateCameraId = 0u;
    CameraType _type = CameraType::COUNT;

    // Since quaternion reflection is complicated and not really needed now, 
    // handle reflections a-la Ogre -Ionut
    bool _reflectionActive = false;
    bool _projectionDirty = true;
    bool _viewMatrixDirty = false;
    bool _frustumLocked = false;
    bool _isOrthoCamera = false;
    bool _frustumDirty = true;

   // Camera pool
   public:
    static void update(const U64 deltaTimeUS);
    static void initPool();
    static void destroyPool();

    template <typename T>
    typename std::enable_if<std::is_base_of<Camera, T>::value, bool>::type
    static destroyCamera(T*& camera) {
        if (destroyCameraInternal(camera)) {
            camera = nullptr;
            return true;
        }

        return false;
    }

    template <typename T = Camera>
    typename std::enable_if<std::is_base_of<Camera, T>::value, T*>::type
    static utilityCamera(UtilityCamera type) {
        return static_cast<T*>(utilityCameraInternal(type));
    }

    template <typename T>
    static T* createCamera(const Str128& cameraName) {
        return static_cast<T*>(createCameraInternal(cameraName, T::Type()));
    }

    template <typename T = Camera>
    typename std::enable_if<std::is_base_of<Camera, T>::value, T*>::type
    static findCamera(U64 nameHash) {
        return static_cast<T*>(findCameraInternal(nameHash));
    }

    static bool removeChangeListener(U32 id);
    static U32  addChangeListener(const DELEGATE<void, const Camera& /*new camera*/>& f);

   private:
     static bool    destroyCameraInternal(Camera* camera);
     static Camera* utilityCameraInternal(UtilityCamera type);
     static Camera* createCameraInternal(const Str128& cameraName, CameraType type);
     static Camera* findCameraInternal(U64 nameHash);

   private:
    using CameraPool = vectorEASTL<Camera*>;

    static std::array<Camera*, to_base(UtilityCamera::COUNT)> _utilityCameras;

    static U32 s_changeCameraId;
    static ListenerMap s_changeCameraListeners;

    static CameraPool s_cameraPool;
    static SharedMutex s_cameraPoolLock;
};

TYPEDEF_SMART_POINTERS_FOR_TYPE(Camera);

// Just a simple alias for a regular camera
class StaticCamera final : public Camera {
protected:
    friend class Camera;
    explicit StaticCamera(const Str128& name, const vec3<F32>& eye = VECTOR3_ZERO)
        : Camera(name, Type(), eye)
    {
    }
    bool unload() noexcept final { return Camera::unload(); }

public:
    ~StaticCamera() = default;
    static constexpr CameraType Type() noexcept { return CameraType::STATIC; }
};

};  // namespace Divide
#endif
