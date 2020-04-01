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

#include "Core/Math/Headers/Quaternion.h"
#include "Core/Resources/Headers/Resource.h"

namespace Divide {

class GFXDevice;
class Camera : public Resource {
   public:
    enum class CameraType : U8 {
        FREE_FLY = 0,
        FIRST_PERSON,
        THIRD_PERSON,
        ORBIT,
        SCRIPTED
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

    virtual void fromCamera(Camera& camera);
    virtual void fromCamera(const Camera& camera);
    virtual void fromSnapshot(const CameraSnapshot& snapshot);

    const CameraSnapshot& snapshot();
    const CameraSnapshot& snapshot() const;

    // Return true if the cached camera state wasn't up-to-date
    bool updateLookAt();

    void setReflection(const Plane<F32>& reflectionPlane);
    void clearReflection();

    /// Moves the camera by the specified offsets in each direction
    virtual void move(F32 dx, F32 dy, F32 dz);
    /// Global rotations are applied relative to the world axis, not the camera's
    void setGlobalRotation(F32 yaw, F32 pitch, F32 roll = 0.0f);
    inline void setGlobalRotation(const vec3<Angle::DEGREES<F32>>& euler) {
        setGlobalRotation(euler.yaw, euler.pitch, euler.roll);
    }
    const mat4<F32>& lookAt(const mat4<F32>& viewMatrix);
    /// Sets the camera's position, target and up directions
    const mat4<F32>& lookAt(const vec3<F32>& eye,
                            const vec3<F32>& target,
                            const vec3<F32>& up);
    /// Rotates the camera (changes its orientation) by the specified quaternion
    /// (_orientation *= q)
    void rotate(const Quaternion<F32>& q);
    /// Sets the camera to point at the specified target point
    inline const mat4<F32>& lookAt(const vec3<F32>& target) {
        return lookAt(_data._eye, target);
    }

    inline const mat4<F32>& lookAt(const vec3<F32>& eye,
                                   const vec3<F32>& target) {
        return lookAt(eye, target, getUpDir());
    }
    /// Sets the camera's orientation to match the specified yaw, pitch and roll
    /// values;
    /// Creates a quaternion based on the specified Euler angles and calls
    /// "rotate" to change
    /// the orientation
    virtual void rotate(Angle::DEGREES<F32> yaw, Angle::DEGREES<F32> pitch, Angle::DEGREES<F32> roll);
    /// Creates a quaternion based on the specified axis-angle and calls
    /// "rotate"
    /// to change
    /// the orientation
    inline void rotate(const vec3<F32>& axis, Angle::DEGREES<F32> angle) {
        rotate(Quaternion<F32>(axis, angle * _speed.turn));
    }
    /// Yaw, Pitch and Roll call "rotate" with a appropriate quaternion for  each
    /// rotation.
    /// Because the camera is facing the -Z axis, a positive angle will create a
    /// positive Yaw
    /// behind the camera and a negative one in front of the camera
    /// (so we invert the angle - left will turn left when facing -Z)
    void rotateYaw(Angle::DEGREES<F32> angle);
    /// Change camera's roll.
    void rotateRoll(Angle::DEGREES<F32> angle);
    /// Change camera's pitch
    void rotatePitch(Angle::DEGREES<F32> angle);
    /// Sets the camera's Yaw angle.
    /// This creates a new orientation quaternion for the camera and extracts the Euler angles
    inline void setYaw(Angle::DEGREES<F32> angle) {
        setRotation(angle, _euler.pitch, _euler.roll);
    }
    /// Sets the camera's Pitch angle. Yaw and Roll are previous extracted values
    inline void setPitch(Angle::DEGREES<F32> angle) {
        setRotation(_euler.yaw, angle, _euler.roll);
    }
    /// Sets the camera's Roll angle. Yaw and Pitch are previous extracted values
    inline void setRoll(Angle::DEGREES<F32> angle) {
        setRotation(_euler.yaw, _euler.pitch, angle);
    }
    /// Sets the camera's Yaw angle.
    /// This creates a new orientation quaternion for the camera and extracts the Euler angles
    inline void setGlobalYaw(Angle::DEGREES<F32> angle) {
        setGlobalRotation(angle, _euler.pitch, _euler.roll);
    }
    /// Sets the camera's Pitch angle. Yaw and Roll are previous extracted
    /// values
    inline void setGlobalPitch(Angle::DEGREES<F32> angle) {
        setGlobalRotation(_euler.yaw, angle, _euler.roll);
    }
    /// Sets the camera's Roll angle. Yaw and Pitch are previous extracted
    /// values
    inline void setGlobalRoll(Angle::DEGREES<F32> angle) {
        setGlobalRotation(_euler.yaw, _euler.pitch, angle);
    }
    /// Moves the camera forward or backwards
    inline void moveForward(F32 factor) {
        move(0.0f, 0.0f, factor);
    }
    /// Moves the camera left or right
    inline void moveStrafe(F32 factor) {
        move(factor, 0.0f, 0.0f);
    }
    /// Moves the camera up or down
    inline void moveUp(F32 factor) {
        move(0.0f, factor, 0.0f);
    }

    inline void setEye(F32 x, F32 y, F32 z) noexcept {
        _data._eye.set(x, y, z);
        _viewMatrixDirty = true;
    }

    inline void setEye(const vec3<F32>& position) noexcept {
        _data._eye = position;
        _viewMatrixDirty = true;
    }

    inline void setRotation(const Quaternion<F32>& q) {
        _data._orientation = q;
        _viewMatrixDirty = true;
    }

    inline void setRotation(Angle::DEGREES<F32> yaw, Angle::DEGREES<F32> pitch, Angle::DEGREES<F32> roll = 0.0f) {
        setRotation(Quaternion<F32>(-pitch, -yaw, -roll));
    }

    void setAspectRatio(F32 ratio);
    void setVerticalFoV(Angle::DEGREES<F32> verticalFoV);
    void setHorizontalFoV(Angle::DEGREES<F32> horizontalFoV);

    /// Mouse sensitivity: amount of pixels per radian (this should be moved out
    /// of the camera class)
    inline void setMouseSensitivity(F32 sensitivity) noexcept {
        _mouseSensitivity = sensitivity;
    }

    inline void setMoveSpeedFactor(F32 moveSpeedFactor) noexcept {
        _speedFactor.move = moveSpeedFactor;
    }

    inline void setTurnSpeedFactor(F32 turnSpeedFactor) noexcept {
        _speedFactor.turn = turnSpeedFactor;
    }

    inline void setZoomSpeedFactor(F32 zoomSpeedFactor) noexcept {
        _speedFactor.zoom = zoomSpeedFactor;
    }

    /// Exactly as in Ogre3D: locks the yaw movement to the specified axis
    inline void setFixedYawAxis(bool useFixed, const vec3<F32>& fixedAxis = WORLD_Y_AXIS) noexcept {
        _yawFixed = useFixed;
        _fixedYawAxis = fixedAxis;
    }

    /// Getter methods.

    inline F32 getTurnSpeedFactor() const noexcept { return _speedFactor.turn; }
    inline F32 getMoveSpeedFactor() const noexcept { return _speedFactor.move; }
    inline F32 getZoomSpeedFactor() const noexcept { return _speedFactor.zoom; }

    inline const CameraType& type() const noexcept { return _type; }

    inline const vec3<F32>& getEye() const noexcept {
        return _data._eye;
    }

    inline const Quaternion<F32>& getOrientation() const noexcept {
        return _data._orientation;
    }

    inline vec3<F32> getUpDir() const {
        const mat4<F32>& viewMat = getViewMatrix();
        return vec3<F32>(viewMat.m[0][1], viewMat.m[1][1], viewMat.m[2][1]);
    }

    inline vec3<F32> getRightDir() const {
        const mat4<F32>& viewMat = getViewMatrix();
        return vec3<F32>(viewMat.m[0][0], viewMat.m[1][0], viewMat.m[2][0]);

    }

    inline vec3<F32> getForwardDir() const {
        const mat4<F32>& viewMat = getViewMatrix();
        return vec3<F32>(-viewMat.m[0][2], -viewMat.m[1][2], -viewMat.m[2][2]);
    }

    inline const vec3<Angle::DEGREES<F32>>& getEuler() const noexcept { return _euler; }

    inline void setEuler(const vec3<Angle::DEGREES<F32>>& euler) {
        setRotation(euler.yaw, euler.pitch, euler.roll);
    }

    inline const vec2<F32>& getZPlanes() const noexcept { return _data._zPlanes; }

    inline const vec4<F32>& orthoRect() const noexcept { return _orthoRect; }

    inline bool isOrthoProjected() const noexcept { return _isOrthoCamera; }

    inline const Angle::DEGREES<F32> getVerticalFoV() const noexcept { return _data._FoV; }

    inline const Angle::DEGREES<F32> getHorizontalFoV() const noexcept {
        const Angle::RADIANS<F32> halfFoV = Angle::to_RADIANS(_data._FoV) * 0.5f;
        return Angle::to_DEGREES(2.0f * std::atan(tan(halfFoV) * _data._aspectRatio));
    }

    inline const F32 getAspectRatio() const noexcept {
        return _data._aspectRatio;
    }

    inline const mat4<F32>& getViewMatrix() const noexcept {
        return _data._viewMatrix;
    }

    inline const mat4<F32>& getViewMatrix() {
        updateViewMatrix();
        return _data._viewMatrix;
    }

    inline const mat4<F32>& getProjectionMatrix() {
        updateProjection();
        return _data._projectionMatrix;
    }

    inline const mat4<F32>& getProjectionMatrix() const noexcept {
        return _data._projectionMatrix;
    }

    inline mat4<F32> getWorldMatrix() {
        mat4<F32> ret;
        getWorldMatrix(ret);
        return ret;
    }

    inline mat4<F32> getWorldMatrix() const {
        mat4<F32> ret;
        getWorldMatrix(ret);
        return ret;
    }

    inline void getWorldMatrix(mat4<F32>& worldMatOut) {
        getViewMatrix().getInverse(worldMatOut);
    }

    inline void getWorldMatrix(mat4<F32>& worldMatOut) const {
        getViewMatrix().getInverse(worldMatOut);
    }

    /// Nothing really to unload
    virtual bool unload() { return true; }

    const mat4<F32>& setProjection(F32 aspectRatio, F32 verticalFoV, const vec2<F32>& zPlanes);

    const mat4<F32>& setProjection(const vec4<F32>& rect, const vec2<F32>& zPlanes);

    const mat4<F32>& setProjection(const mat4<F32>& projection, const vec2<F32>& zPlanes, bool isOrtho);

    /// Extract the frustum associated with our current PoV
    virtual bool updateFrustum();
    /// Get the camera's current frustum
    inline const Frustum& getFrustum() const noexcept { return _frustum; }

    inline Frustum& getFrustum() noexcept { return _frustum; }

    inline void lockMovement(bool state) noexcept { _movementLocked = state; }

    inline void lockRotation(bool state) noexcept { _rotationLocked = state; }

    inline void lockFrustum(bool state) noexcept { _frustumLocked = state; }

    /// Get the world space position from the specified screen coordinates
    /// (use winCoords.z for depth from 0 to 1)
    inline vec3<F32> unProject(const vec3<F32>& winCoords, const Rect<I32>& viewport) const {
        return unProject(winCoords.x, winCoords.y, winCoords.z, viewport);
    }

    vec3<F32> unProject(F32 winCoordsX, F32 winCoordsY, F32 winCoordsZ, const Rect<I32>& viewport) const;

    vec2<F32> project(const vec3<F32>& worldCoords, const Rect<I32>& viewport) const;

    virtual bool moveRelative(const vec3<I32>& relMovement);
    virtual bool rotateRelative(const vec3<I32>& relRotation);
    virtual bool zoom(I32 zoomFactor);

    bool removeUpdateListener(U32 id);
    U32 addUpdateListener(const DELEGATE<void, const Camera& /*updated camera*/>& f);

    virtual ~Camera();

   protected:
    virtual bool updateViewMatrix();
    virtual bool updateProjection();
    /// Inject mouse events
    virtual void updateInternal(const U64 deltaTimeUS, const F32 deltaTimeS);

    const char* getResourceTypeName() const noexcept override { return "Camera"; }

   protected:
    SET_DELETE_FRIEND
    SET_DELETE_HASHMAP_FRIEND
    explicit Camera(const Str128& name, const CameraType& type, const vec3<F32>& eye = VECTOR3_ZERO);

   protected:
    CameraSnapshot _data;
    vec3<F32> _fixedYawAxis;
    vec4<F32> _orthoRect;

    vec3<Angle::DEGREES<F32>> _euler;
    Angle::DEGREES<F32> _accumPitchDegrees;

    vec4<F32> _speedFactor;
    vec4<F32> _speed;

    F32 _mouseSensitivity;
    CameraType _type;

    bool _projectionDirty;
    bool _viewMatrixDirty;
    bool _frustumLocked;
    bool _rotationLocked;
    bool _movementLocked;
    bool _yawFixed;
    bool _isOrthoCamera;
    bool _frustumDirty;
    Frustum _frustum;

    // Since quaternion reflection is complicated and not really needed now, 
    // handle reflections a-la Ogre -Ionut
    bool _reflectionActive;
    Plane<F32> _reflectionPlane;
    U32 _updateCameraId;
    using ListenerMap = hashMap<U32, DELEGATE<void, const Camera&>>;
    ListenerMap _updateCameraListeners;

    // Camera pool
    public:
       static void update(const U64 deltaTimeUS);
       static void initPool();
       static void destroyPool();

       static vectorSTD<U64> cameraList();

       static Camera* utilityCamera(UtilityCamera type);

       static Camera* createCamera(const Str128& cameraName, CameraType type);
       static bool    destroyCamera(Camera*& camera);
       static Camera* findCamera(U64 nameHash);

       static bool removeChangeListener(U32 id);
       static U32 addChangeListener(const DELEGATE<void, const Camera& /*new camera*/>& f);

    private:
      using CameraPool = hashMap<U64, Camera*>;

      static std::array<Camera*, to_base(UtilityCamera::COUNT)> _utilityCameras;

      static U32 s_changeCameraId;
      static ListenerMap s_changeCameraListeners;

      static CameraPool s_cameraPool;
      static SharedMutex s_cameraPoolLock;
};

TYPEDEF_SMART_POINTERS_FOR_TYPE(Camera);

};  // namespace Divide
#endif
