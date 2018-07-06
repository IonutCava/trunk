/*
Copyright (c) 2015 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef _PHYSICS_COMPONENT_H_
#define _PHYSICS_COMPONENT_H_

#include "SGNComponent.h"
#include "Core/Math/Headers/Transform.h"

namespace Divide {

class Transform;
class PhysicsAsset;
class SceneGraphNode;
class PhysicsComponent : public SGNComponent {
   public:
    PhysicsComponent(SceneGraphNode& parentSGN);
    ~PhysicsComponent();

    enum class PhysicsGroup : U32 {
        NODE_COLLIDE_IGNORE = 0,
        NODE_COLLIDE_NO_PUSH,
        NODE_COLLIDE
    };

    inline const PhysicsGroup& physicsGroup() const {
        return _physicsCollisionGroup;
    }

    inline void physicsGroup(const PhysicsGroup& newGroup) {
        _physicsCollisionGroup = newGroup;
    }

    void physicsAsset(PhysicsAsset* const asset);

    inline PhysicsAsset* const physicsAsset() { return _physicsAsset; }

    void cookCollisionMesh(const stringImpl& sceneName);

    void reset();

    const mat4<F32>& getWorldMatrix(D32 interpolationFactor,
                                    const bool local = false);
    inline const mat4<F32>& getWorldMatrix(const bool local = false) {
        return getWorldMatrix(1.0, local);
    }

    /// Component <-> Transform interface
    void setPosition(const vec3<F32>& position);
    void setScale(const vec3<F32>& scale);
    void setRotation(const vec3<F32>& axis, F32 degrees, bool inDegrees = true);
    void setRotation(const vec3<F32>& euler, bool inDegrees = true);
    void setRotation(const Quaternion<F32>& quat);
    void translate(const vec3<F32>& axisFactors);
    void scale(const vec3<F32>& axisFactors);
    void rotate(const vec3<F32>& axis, F32 degrees, bool inDegrees = true);
    void rotate(const vec3<F32>& euler, bool inDegrees = true);
    void rotate(const Quaternion<F32>& quat);
    void rotateSlerp(const Quaternion<F32>& quat, const D32 deltaTime);
    void setScale(const F32 scale);
    void setScaleX(const F32 scale);
    void setScaleY(const F32 scale);
    void setScaleZ(const F32 scale);
    void scale(const F32 scale);
    void scaleX(const F32 scale);
    void scaleY(const F32 scale);
    void scaleZ(const F32 scale);
    void rotateX(const F32 angle, bool inDegrees = true);
    void rotateY(const F32 angle, bool inDegrees = true);
    void rotateZ(const F32 angle, bool inDegrees = true);
    void setRotationX(const F32 angle, bool inDegrees = true);
    void setRotationXEuler(const F32 angle, bool inDegrees = true);
    void setRotationY(const F32 angle, bool inDegrees = true);
    void setRotationZ(const F32 angle, bool inDegrees = true);
    void translateX(const F32 positionX);
    void translateY(const F32 positionY);
    void translateZ(const F32 positionZ);
    void setPositionX(const F32 positionX);
    void setPositionY(const F32 positionY);
    void setPositionZ(const F32 positionZ);

    inline bool isUniformScaled() const {
        return _transform != nullptr ? _transform->isUniformScaled() : true;
    }
    /// Return the scale factor
    const vec3<F32>& getScale(const bool local = false);
    /// Return the position
    const vec3<F32>& getPosition(const bool local = false);
    /// Return the orientation quaternion
    const Quaternion<F32>& getOrientation(const bool local = false);

   protected:
    friend class SceneGraphNode;
    void useDefaultTransform(const bool state);

    inline bool transformUpdated() const { return _transformUpdated; }
    inline void transformUpdated(const bool state) {
        _transformUpdated = state;
    }

   private:
    void setTransformDirty();

   protected:
    PhysicsAsset* _physicsAsset;
    PhysicsGroup _physicsCollisionGroup;
    Transform* _transform;
    TransformValues _prevTransformValues;

    bool _transformUpdated;
    /// Transform cache values
    mat4<F32> _worldMatrix;
    vec3<F32> _scaleCache;
    vec3<F32> _positionCache;
    Quaternion<F32> _orientationCache;
};

};  // namespace Divide
#endif