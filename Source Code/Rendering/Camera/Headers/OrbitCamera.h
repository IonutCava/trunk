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

#ifndef _ORBIT_CAMERA_H_
#define _ORBIT_CAMERA_H_

#include "Camera.h"

namespace Divide {

FWD_DECLARE_MANAGED_CLASS(SceneGraphNode);

/// A camera that always looks at a given target and orbits around it.
/// It's position / direction can't be changed by user input
class OrbitCamera : public Camera {
  protected:
    friend class Camera;
    explicit OrbitCamera(const stringImpl& name, 
                         const CameraType& type = CameraType::ORBIT,
                         const vec3<F32>& eye = VECTOR3_ZERO);
  public:
    void setTarget(SceneGraphNode_wptr sgn,
                   const vec3<F32>& offsetDirection = vec3<F32>(0, 0.75, 1.0));

    void fromCamera(Camera& camera) override;

    inline void maxRadius(F32 radius) { _maxRadius = radius; }

    inline void minRadius(F32 radius) { _minRadius = radius; }

    inline void curRadius(F32 radius) {
        _curRadius = radius;
        CLAMP<F32>(_curRadius, _minRadius, _maxRadius);
    }

    inline F32 maxRadius() const { return _maxRadius; }
    inline F32 minRadius() const { return _minRadius; }
    inline F32 curRadius() const { return _curRadius; }

    virtual void update(const U64 deltaTime);
    virtual void move(F32 dx, F32 dy, F32 dz);
    virtual void rotate(F32 yaw, F32 pitch, F32 roll);
    virtual bool moveRelative(const vec3<I32>& relMovement) override;
    virtual void setActiveInternal(bool state) override;

   protected:
    virtual bool updateViewMatrix();

   protected:
    F32 _maxRadius;
    F32 _minRadius;
    F32 _curRadius;
    F32 _currentRotationX;
    F32 _currentRotationY;
    bool _rotationDirty;
    vec3<F32> _offsetDir;
    vec3<F32> _cameraRotation;
    vec3<F32> _newEye;
    SceneGraphNode_wptr _targetNode;
};

};  // namespace Divide

#endif