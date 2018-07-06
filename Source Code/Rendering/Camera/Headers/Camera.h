/*
   Copyright (c) 2016 DIVIDE-Studio
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

#ifndef _CAMERA_H
#define _CAMERA_H

#include "Frustum.h"
#include "Core/Math/Headers/Quaternion.h"
#include "Core/Resources/Headers/Resource.h"
#include "Platform/Input/Headers/EventHandler.h"

namespace Divide {

class Camera : public Resource {
   public:
    enum class CameraType : U32 { 
        FREE_FLY = 0,
        FIRST_PERSON,
        THIRD_PERSON,
        ORBIT, SCRIPTED
    };

   public:

    void fromCamera(const Camera& camera);

    virtual void update(const U64 deltaTime);

    void updateLookAt();

    void reflect(const Plane<F32>& reflectionPlane);
    /// Moves the camera by the specified offsets in each direction
    virtual void move(F32 dx, F32 dy, F32 dz);
    /// Global rotations are applied relative to the world axis, not the camera's
    void setGlobalRotation(F32 yaw, F32 pitch, F32 roll = 0.0f);
    inline void setGlobalRotation(const vec3<F32>& euler) {
        setGlobalRotation(euler.yaw, euler.pitch, euler.roll);
    }
    /// Sets the camera's position, target and up directions
    const mat4<F32>& lookAt(const vec3<F32>& eye, const vec3<F32>& target,
                            const vec3<F32>& up = WORLD_Y_AXIS);
    /// Rotates the camera (changes its orientation) by the specified quaternion
    /// (_orientation *= q)
    void rotate(const Quaternion<F32>& q);
    /// Sets the camera to point at the specified target point
    inline const mat4<F32>& lookAt(const vec3<F32>& target) {
        return lookAt(_eye, target, getUpDir());
    }
    /// Sets the camera's orientation to match the specified yaw, pitch and roll
    /// values;
    /// Creates a quaternion based on the specified euler angles and calls
    /// "rotate" to change
    /// the orientation
    virtual void rotate(F32 yaw, F32 pitch, F32 roll);
    /// Creates a quaternion based on the specified axis-angle and calls
    /// "rotate"
    /// to change
    /// the orientation
    inline void rotate(const vec3<F32>& axis, F32 angle) {
        rotate(Quaternion<F32>(axis, angle * _cameraTurnSpeed));
    }
    /// Yaw, Pitch and Roll call "rotate" with a apropriate quaternion for  each
    /// rotation.
    /// Because the camera is facing the -Z axis, a positive angle will create a
    /// positive Yaw
    /// behind the camera and a negative one in front of the camera
    /// (so we invert the angle - left will turn left when facing -Z)
    inline void rotateYaw(F32 angle) {
        rotate(Quaternion<F32>(
            _yawFixed ? _fixedYawAxis : _orientation * WORLD_Y_AXIS,
            -angle * _cameraTurnSpeed));
    }
    /// Change camera's roll.
    inline void rotateRoll(F32 angle) {
        rotate(Quaternion<F32>(_orientation * WORLD_Z_AXIS,
                               -angle * _cameraTurnSpeed));
    }
    /// Change camera's pitch
    inline void rotatePitch(F32 angle) {
        rotate(Quaternion<F32>(_orientation * WORLD_X_AXIS,
                               -angle * _cameraTurnSpeed));
    }
    /// Sets the camera's Yaw angle.
    /// This creates a new orientation quaternion for the camera and extracts
    /// the
    /// euler angles
    inline void setYaw(F32 angle) {
        setRotation(angle, _euler.pitch, _euler.roll);
    }
    /// Sets the camera's Pitch angle. Yaw and Roll are previous extracted
    /// values
    inline void setPitch(F32 angle) {
        setRotation(_euler.yaw, angle, _euler.roll);
    }
    /// Sets the camera's Roll angle. Yaw and Pitch are previous extracted
    /// values
    inline void setRoll(F32 angle) {
        setRotation(_euler.yaw, _euler.pitch, angle);
    }
    /// Sets the camera's Yaw angle.
    /// This creates a new orientation quaternion for the camera and extracts
    /// the
    /// euler angles
    inline void setGlobalYaw(F32 angle) {
        setGlobalRotation(angle, _euler.pitch, _euler.roll);
    }
    /// Sets the camera's Pitch angle. Yaw and Roll are previous extracted
    /// values
    inline void setGlobalPitch(F32 angle) {
        setGlobalRotation(_euler.yaw, angle, _euler.roll);
    }
    /// Sets the camera's Roll angle. Yaw and Pitch are previous extracted
    /// values
    inline void setGlobalRoll(F32 angle) {
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

    inline void setEye(F32 x, F32 y, F32 z) {
        _eye.set(x, y, z);
        _viewMatrixDirty = true;
    }

    inline void setEye(const vec3<F32>& position) {
        _eye = position;
        _viewMatrixDirty = true;
    }

    inline void setRotation(const Quaternion<F32>& q) {
        _orientation = q;
        _viewMatrixDirty = true;
    }

    inline void setRotation(F32 yaw, F32 pitch, F32 roll = 0.0f) {
        setRotation(Quaternion<F32>(-pitch, -yaw, -roll));
    }

    inline void setAspectRatio(F32 ratio) {
        _aspectRatio = ratio;
        _projectionDirty = true;
    }

    inline void setVerticalFoV(F32 verticalFoV) {
        _verticalFoV = verticalFoV;
        _projectionDirty = true;
    }

    inline void setHorizontalFoV(F32 horizontalFoV) {
        _verticalFoV = Angle::RadiansToDegrees(
            2.0f *
            std::atan(tan(Angle::DegreesToRadians(horizontalFoV) * 0.5f) /
                      _aspectRatio));
        _projectionDirty = true;
    }

    /// Mouse sensitivity: amount of pixels per radian (this should be moved out
    /// of the camera class)
    inline void setMouseSensitivity(F32 sensitivity) {
        _mouseSensitivity = sensitivity;
    }

    inline void setMoveSpeedFactor(F32 moveSpeedFactor) {
        _moveSpeedFactor = moveSpeedFactor;
    }

    inline void setTurnSpeedFactor(F32 turnSpeedFactor) {
        _turnSpeedFactor = turnSpeedFactor;
    }

    inline void setZoomSpeedFactor(F32 zoomSpeedFactor) {
        _zoomSpeedFactor = zoomSpeedFactor;
    }

    /// Exactly as in Ogre3D: locks the yaw movement to the specified axis
    inline void setFixedYawAxis(bool useFixed,
                                const vec3<F32>& fixedAxis = WORLD_Y_AXIS) {
        _yawFixed = useFixed;
        _fixedYawAxis = fixedAxis;
    }

    /// Getter methods.

    inline F32 getMoveSpeedFactor() const { return _moveSpeedFactor; }
    inline F32 getTurnSpeedFactor() const { return _turnSpeedFactor; }
    inline F32 getZoomSpeedFactor() const { return _zoomSpeedFactor; }

    inline const CameraType& getType() const { return _type; }

    inline const vec3<F32>& getEye() const {
        return _eye;
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

    inline const vec3<F32>& getEuler() const { return _euler; }

    inline const vec3<F32>& getTarget() const { return _target; }

    inline const mat4<F32>& getProjectionMatrix() const {
        return _projectionMatrix;
    }

    inline const vec2<F32>& getZPlanes() const { return _zPlanes; }

    inline const vec4<F32>& orthoRect() const { return _orthoRect; }

    inline bool isOrthoProjected() const { return _isOrthoCamera; }

    inline const F32 getVerticalFoV() const { return _verticalFoV; }

    inline const F32 getHorizontalFoV() const {
        F32 halfFoV = Angle::DegreesToRadians(_verticalFoV) * 0.5f;
        return Angle::RadiansToDegrees(2.0f *
                                       std::atan(tan(halfFoV) * _aspectRatio));
    }

    inline const F32 getAspectRatio() const { return _aspectRatio; }

    inline const mat4<F32>& getViewMatrix() const {
        return _viewMatrix;
    }

    inline const mat4<F32>& getViewMatrix(bool update = false) {
        if (update) {
            updateViewMatrix();
        }

        return _viewMatrix;
    }

    inline void getWorldMatrix(mat4<F32>& worldMatOut) {
        getViewMatrix().getInverse(worldMatOut);
    }

    /// Nothing really to unload
    virtual bool unload() { return true; }
    /// Add an event listener called after every RenderLookAt or
    /// RenderLookAtCube
    /// call
    virtual void addUpdateListener(const DELEGATE_CBK_PARAM<Camera&>& f) {
        _listeners.push_back(f);
    }
    /// Informs all listeners of a new event
    virtual void updateListeners();
    /// Clear all listeners from the current camera
    virtual void clearListeners() { _listeners.clear(); }
    /// Inject mouse events
    virtual bool mouseMoved(const Input::MouseEvent& arg) { return true; }
    /// Called when the camera becomes active
    virtual void onActivate();
    /// Called when the camera becomes inactive
    virtual void onDeactivate();

    void setProjection(F32 aspectRatio, F32 verticalFoV,
                       const vec2<F32>& zPlanes, bool updateOnSet = false);

    void setProjection(const vec4<F32>& rect, const vec2<F32>& zPlanes,
                       bool updateOnSet = false);
    /// Extract the frustum associated with our current PoV
    virtual bool updateFrustum();
    /// Get the camera's current frustum
    inline const Frustum& getFrustumConst() const { return *_frustum; }

    inline Frustum& getFrustum() { return *_frustum; }

    inline void lockMovement(bool state) { _movementLocked = state; }

    inline void lockRotation(bool state) { _rotationLocked = state; }

    inline void lockView(bool state) { _viewMatrixLocked = state; }

    inline void lockFrustum(bool state) { _frustumLocked = state; }

    /// Get the world space pozition from the specified screen coordinates
    /// (use winCoords.z for depth from 0 to 1)
    inline vec3<F32> unProject(const vec3<F32>& winCoords) const {
        return unProject(winCoords.x, winCoords.y, winCoords.z);
    }

    /// Get the world space pozition from the specified screen coordinates
    /// (use winCoords.z for depth from 0 to 1)
    inline vec3<F32> unProject(const vec3<F32>& winCoords,
                               const vec4<I32>& viewport) const {
        return unProject(winCoords.x, winCoords.y, winCoords.z, viewport);
    }

    vec3<F32> unProject(F32 winCoordsX, F32 winCoordsY, F32 winCoordsZ) const;
    vec3<F32> unProject(F32 winCoordsX, F32 winCoordsY, F32 winCoordsZ, const vec4<I32>& viewport) const;

   protected:
    virtual bool updateViewMatrix();
    virtual void updateProjection();

   protected:
    SET_DELETE_FRIEND
    SET_DELETE_HASHMAP_FRIEND
    friend class CameraManager;
    explicit Camera(const stringImpl& name, const CameraType& type, const vec3<F32>& eye = VECTOR3_ZERO);
    virtual ~Camera();

   protected:
    mat4<F32> _viewMatrix;
    mat4<F32> _projectionMatrix;
    Quaternion<F32> _orientation;
    vec3<F32> _eye, _target;
    vec3<F32> _euler, _fixedYawAxis;
    vec2<F32> _zPlanes;
    vec4<F32> _orthoRect;

    F32 _verticalFoV;
    F32 _aspectRatio;
    F32 _accumPitchDegrees;
    F32 _turnSpeedFactor;
    F32 _moveSpeedFactor;
    F32 _mouseSensitivity;
    F32 _zoomSpeedFactor;
    F32 _cameraMoveSpeed;
    F32 _cameraTurnSpeed;
    F32 _cameraZoomSpeed;
    CameraType _type;

    vectorImpl<DELEGATE_CBK_PARAM<Camera&> > _listeners;
    bool _isActive;
    bool _projectionDirty;
    bool _viewMatrixDirty;
    bool _viewMatrixLocked;
    bool _frustumLocked;
    bool _rotationLocked;
    bool _movementLocked;
    bool _yawFixed;
    bool _isOrthoCamera;
    bool _frustumDirty;
    Frustum* _frustum;
};

TYPEDEF_SMART_POINTERS_FOR_CLASS(Camera);

};  // namespace Divide
#endif
