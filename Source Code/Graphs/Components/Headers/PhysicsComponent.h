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

#include <stack>

namespace Divide {

namespace Attorney {
    class PXCompPXAsset;
};

class Transform;
class PhysicsAsset;
class SceneGraphNode;
class PhysicsComponent : public SGNComponent {
    friend class Attorney::PXCompPXAsset;
  public:
    enum class PhysicsGroup : U32 {
        NODE_COLLIDE_IGNORE = 0,
        NODE_COLLIDE_NO_PUSH,
        NODE_COLLIDE
    };

    enum class TransformType : U32 {
        TRANSLATION = 0,
        SCALE,
        ROTATION,
        COUNT
    };

    class TransformMask {
        public:
           TransformMask()
           {
               _transformFlags.fill(false);
           }

           inline bool hasSetFlags() const {
                return getFlag(TransformType::SCALE) ||
                       getFlag(TransformType::ROTATION) ||
                       getFlag(TransformType::TRANSLATION);
           }

           inline bool hasSetAllFlags() const {
               return getFlag(TransformType::SCALE) &&
                      getFlag(TransformType::ROTATION) &&
                      getFlag(TransformType::TRANSLATION);
           }

           inline bool getFlag(TransformType type) const {
               return _transformFlags[to_uint(type)];
           }

           inline void clearAllFlags() {
               _transformFlags.fill(false);
           }

           inline void setAllFlags() {
               _transformFlags.fill(true);
           }

           inline bool clearFlag(TransformType type) {
               _transformFlags[to_uint(type)] = false;
           }

           inline void setFlag(TransformType type) {
               _transformFlags[to_uint(type)] = true;
           }

           inline void setFlags(const TransformMask& other) {
               _transformFlags[to_uint(TransformType::SCALE)] = 
                       other.getFlag(TransformType::SCALE);
               _transformFlags[to_uint(TransformType::ROTATION)] =
                       other.getFlag(TransformType::ROTATION);
               _transformFlags[to_uint(TransformType::TRANSLATION)] =
                       other.getFlag(TransformType::TRANSLATION);
           }
        private:
           std::array<bool, to_const_uint(TransformType::COUNT)> _transformFlags;
    };

   public:
    void update(const U64 deltaTime);

    inline const PhysicsGroup& physicsGroup() const {
        return _physicsCollisionGroup;
    }

    inline void physicsGroup(const PhysicsGroup& newGroup) {
        _physicsCollisionGroup = newGroup;
    }

    inline PhysicsAsset* const physicsAsset() { return _physicsAsset; }

    void cookCollisionMesh(const stringImpl& sceneName);

    void reset();

    const mat4<F32>& getLocalMatrix() const;

    const mat4<F32>& getWorldMatrix();

    const mat4<F32>& getWorldMatrix(D32 interpolationFactor);
    
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
    vec3<F32> getScale(D32 interpolationFactor = 1.0, const bool local = false) const;
    /// Return the position
    vec3<F32> getPosition(D32 interpolationFactor = 1.0, const bool local = false) const;
    /// Return the orientation quaternion
    Quaternion<F32> getOrientation(D32 interpolationFactor = 1.0, const bool local = false) const;

    void pushTransforms();
    bool popTransforms();

   protected:
    friend class SceneGraphNode;
    PhysicsComponent(SceneGraphNode& parentSGN);
    ~PhysicsComponent();

    void useDefaultTransform(const bool state);

    inline const TransformMask& transformUpdateMask() const {
        return _transformUpdatedMask;
    }

    inline TransformMask& transformUpdateMask() {
        return _transformUpdatedMask;
    }

   private:
    void setTransformDirty(TransformType type);
    bool isParentTransformDirty(bool interp) const;

   protected:
    PhysicsAsset* _physicsAsset;
    PhysicsGroup _physicsCollisionGroup;
    Transform* _transform;
    TransformValues _prevTransformValues;
    typedef std::stack<TransformValues> TransformStack;
    TransformStack _transformStack;
    TransformMask _transformUpdatedMask;
    /// Transform cache values
    std::atomic_bool _dirty;
    mat4<F32> _worldMatrix;
    bool _dirtyInterp;
    D32  _prevInterpValue;
    mat4<F32> _worldMatrixInterp;
};

namespace Attorney {
class PXCompPXAsset {
   private:
    static void setPhysicsAsset(PhysicsComponent& component,
                                PhysicsAsset* asset)
    {
        DIVIDE_ASSERT(!asset || component._physicsAsset == nullptr,
            "PhysicsComponent error: Double set physics asset detected! "
            "remove the previous one first!");
        component._physicsAsset = asset;
    }

    friend class Divide::PhysicsAsset;
};
};  // namespace Attorney
};  // namespace Divide
#endif