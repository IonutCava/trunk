#include "Headers/PhysicsComponent.h"

#include "Core/Headers/Console.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Physics/Headers/PXDevice.h"
#include "Physics/Headers/PhysicsAPIWrapper.h"

namespace Divide {

PhysicsComponent::PhysicsComponent(SceneGraphNode& parentSGN)
    : SGNComponent(SGNComponent::ComponentType::PHYSICS, parentSGN),
      _physicsCollisionGroup(PhysicsGroup::NODE_COLLIDE_IGNORE),
      _physicsAsset(nullptr),
      _dirty(true),
      _dirtyInterp(true),
      _prevInterpValue(0.0)
{
    _transform = MemoryManager_NEW Transform();
    reset();
}

PhysicsComponent::~PhysicsComponent()
{
    if (_physicsAsset != nullptr) {
        _physicsAsset->setParent(nullptr);
    }
    MemoryManager::DELETE(_transform);
}

void PhysicsComponent::update(const U64 deltaTime) {
    if (_transform) {
        _prevTransformValues = _transform->getValues();
    }
}

void PhysicsComponent::reset() {
    _worldMatrix.identity();
    _prevTransformValues._translation.set(0.0f);
    _prevTransformValues._scale.set(1.0f);
    _prevTransformValues._orientation.identity();
    while (!_transformStack.empty()) {
        _transformStack.pop();
    }
    _transformUpdatedMask.setAllFlags();
    _dirty = true;
    _dirtyInterp = true;
    _prevInterpValue = 0.0;
}

void PhysicsComponent::useDefaultTransform(const bool state) {
    if (state && !_transform) {
        _transform = MemoryManager_NEW Transform();
        reset();
    } else if (!state && _transform) {
        MemoryManager::DELETE(_transform);
        reset();
    }
}

void PhysicsComponent::cookCollisionMesh(const stringImpl& sceneName) {
    STUBBED("ToDo: add terrain height field and water cooking support! -Ionut")
    
    U32 childCount = _parentSGN.getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        _parentSGN.getChild(i, childCount).getComponent<PhysicsComponent>()->cookCollisionMesh(sceneName);
    }

    if (_parentSGN.getNode()->getType() == SceneNodeType::TYPE_OBJECT3D) {
        Object3D::ObjectType crtType = _parentSGN.getNode<Object3D>()->getObjectType();
        if (crtType !=  Object3D::ObjectType::TEXT_3D && 
            crtType !=  Object3D::ObjectType::MESH && 
            crtType !=  Object3D::ObjectType::FLYWEIGHT) {
            PHYSICS_DEVICE.createActor(
                _parentSGN, sceneName,
                _parentSGN.usageContext() == SceneGraphNode::UsageContext::NODE_STATIC
                    ? PhysicsActorMask::MASK_RIGID_STATIC
                    : PhysicsActorMask::MASK_RIGID_DYNAMIC,
                _physicsCollisionGroup == PhysicsGroup::NODE_COLLIDE_IGNORE
                    ? PhysicsCollisionGroup::GROUP_NON_COLLIDABLE
                    : (_physicsCollisionGroup == PhysicsGroup::NODE_COLLIDE_NO_PUSH
                           ? PhysicsCollisionGroup::GROUP_COLLIDABLE_NON_PUSHABLE
                           : PhysicsCollisionGroup::GROUP_COLLIDABLE_PUSHABLE));
        }
    }
}

void PhysicsComponent::setTransformDirty(TransformType type) {
    if (_physicsAsset) {
        _physicsAsset->resetTransforms(true);
    }

    _transformUpdatedMask.setFlag(type);
    _dirty = true;
    _dirtyInterp = true;
}

void PhysicsComponent::setPosition(const vec3<F32>& position) {
    if (_transform) {
        _transform->setPosition(position);
    }
    setTransformDirty(TransformType::TRANSLATION);
}

void PhysicsComponent::setScale(const vec3<F32>& scale) {
    if (_transform) {
        _transform->setScale(scale);
    }
    setTransformDirty(TransformType::SCALE);
}

void PhysicsComponent::setRotation(const Quaternion<F32>& quat) {
    if (_transform) {
        _transform->setRotation(quat);
    }
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::setRotation(const vec3<F32>& axis, F32 degrees,
                                   bool inDegrees) {
    if (_transform) {
        _transform->setRotation(axis, degrees, inDegrees);
    }
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::setRotation(const vec3<F32>& euler, bool inDegrees) {
    if (_transform) {
        _transform->setRotation(euler, inDegrees);
    }
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::translate(const vec3<F32>& axisFactors) {
    if (_transform) {
        _transform->translate(axisFactors);
    }
    setTransformDirty(TransformType::TRANSLATION);
}

void PhysicsComponent::scale(const vec3<F32>& axisFactors) {
    if (_transform) {
        _transform->scale(axisFactors);
    }
     setTransformDirty(TransformType::SCALE);
}

void PhysicsComponent::rotate(const vec3<F32>& axis, F32 degrees,
                              bool inDegrees) {
    if (_transform) {
        _transform->rotate(axis, degrees, inDegrees);
    }
     setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::rotate(const vec3<F32>& euler, bool inDegrees) {
    if (_transform) {
        _transform->rotate(euler, inDegrees);
    }
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::rotate(const Quaternion<F32>& quat) {
    if (_transform) {
        _transform->rotate(quat);
    }
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::rotateSlerp(const Quaternion<F32>& quat,
                                   const D32 deltaTime) {
    if (_transform) {
        _transform->rotateSlerp(quat, deltaTime);
    }
   setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::setScale(const F32 scale) {
    if (_transform) {
        _transform->setScale(scale);
    }
     setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::setScaleX(const F32 scale) {
    if (_transform) {
        _transform->setScaleX(scale);
    }
    setTransformDirty(TransformType::SCALE);
}

void PhysicsComponent::setScaleY(const F32 scale) {
    if (_transform) {
        _transform->setScaleY(scale);
    }
    setTransformDirty(TransformType::SCALE);
}

void PhysicsComponent::setScaleZ(const F32 scale) {
    if (_transform) {
        _transform->setScaleZ(scale);
    }
    setTransformDirty(TransformType::SCALE);
}

void PhysicsComponent::scale(const F32 scale) {
    if (_transform) {
        _transform->scale(scale);
    }
    setTransformDirty(TransformType::SCALE);
}

void PhysicsComponent::scaleX(const F32 scale) {
    if (_transform) {
        _transform->scaleX(scale);
    }
    setTransformDirty(TransformType::SCALE);
}

void PhysicsComponent::scaleY(const F32 scale) {
    if (_transform) {
        _transform->scaleY(scale);
    }
    setTransformDirty(TransformType::SCALE);
}

void PhysicsComponent::scaleZ(const F32 scale) {
    if (_transform) {
        _transform->scaleZ(scale);
    }
    setTransformDirty(TransformType::SCALE);
}

void PhysicsComponent::rotateX(const F32 angle, bool inDegrees) {
    if (_transform) {
        _transform->rotateX(angle, inDegrees);
    }
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::rotateY(const F32 angle, bool inDegrees) {
    if (_transform) {
        _transform->rotateY(angle, inDegrees);
    }
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::rotateZ(const F32 angle, bool inDegrees) {
    if (_transform) {
        _transform->rotateZ(angle, inDegrees);
    }
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::setRotationX(const F32 angle, bool inDegrees) {
    if (_transform) {
        _transform->setRotationX(angle, inDegrees);
    }
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::setRotationY(const F32 angle, bool inDegrees) {
    if (_transform) {
        _transform->setRotationY(angle, inDegrees);
    }
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::setRotationZ(const F32 angle, bool inDegrees) {
    if (_transform) {
        _transform->setRotationZ(angle, inDegrees);
    }
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::translateX(const F32 positionX) {
    if (_transform) {
        _transform->translateX(positionX);
    }
    setTransformDirty(TransformType::TRANSLATION);
}

void PhysicsComponent::translateY(const F32 positionY) {
    if (_transform) {
        _transform->translateY(positionY);
    }
    setTransformDirty(TransformType::TRANSLATION);
}

void PhysicsComponent::translateZ(const F32 positionZ) {
    if (_transform) {
        _transform->translateZ(positionZ);
    }
    setTransformDirty(TransformType::TRANSLATION);
}

void PhysicsComponent::setPositionX(const F32 positionX) {
    if (_transform) {
        _transform->setPositionX(positionX);
    }
    setTransformDirty(TransformType::TRANSLATION);
}

void PhysicsComponent::setPositionY(const F32 positionY) {
    if (_transform) {
        _transform->setPositionY(positionY);
    }
    setTransformDirty(TransformType::TRANSLATION);
}

void PhysicsComponent::setPositionZ(const F32 positionZ) {
    if (_transform) {
        _transform->setPositionZ(positionZ);
    }
    setTransformDirty(TransformType::TRANSLATION);
}

void PhysicsComponent::pushTransforms() {
    if (_transform) {
        _transformStack.push(_transform->getValues());
    }
}

bool PhysicsComponent::popTransforms() {
    if (!_transformStack.empty()) {
        if (_transform) {
            _prevTransformValues = _transformStack.top();
            _transform->setValues(_prevTransformValues);
            _transformStack.pop();
        }
        setTransformDirty(TransformType::TRANSLATION);
        setTransformDirty(TransformType::SCALE);
        setTransformDirty(TransformType::ROTATION);
        return true;
    }
    return false;
}

void PhysicsComponent::getWorldMatrixNonInterp(mat4<F32>& matOut, const bool local, bool dirty) const {
    if (_transform && dirty) {
        bool wasRebuilt = false;
        matOut *= _transform->getMatrix(wasRebuilt);
    }

    if (!local) {
        SceneGraphNode_ptr grandParent = _parentSGN.getParent().lock();
        if (grandParent) {
            grandParent->getComponent<PhysicsComponent>()->getWorldMatrixNonInterp(matOut, local, dirty);
        }
    }
}

void PhysicsComponent::getWorldMatrixInterp(mat4<F32>& matOut, D32 interpolationFactor, bool dirty) const {
    if (dirty) {
        matOut *= mat4<F32>(getPosition(interpolationFactor, true),
                            getScale(interpolationFactor, true),
                            GetMatrix(getOrientation(interpolationFactor, true)));
    }

    SceneGraphNode_ptr grandParent = _parentSGN.getParent().lock();
    if (grandParent) {
        grandParent->getComponent<PhysicsComponent>()->getWorldMatrixInterp(matOut, interpolationFactor, dirty);
    }
}


const mat4<F32>& PhysicsComponent::getWorldMatrix(D32 interpolationFactor, bool dirty) {
    dirty |= (_dirtyInterp || _prevInterpValue != interpolationFactor);
    if (dirty) {
        _worldMatrixInterp.identity();
    }
    getWorldMatrixInterp(_worldMatrixInterp, interpolationFactor, dirty);
    _dirtyInterp = false;

    return _worldMatrixInterp;
}

const mat4<F32>& PhysicsComponent::getWorldMatrix(const bool local, bool dirty) {
    dirty |= _dirty;
    if (dirty) {
        _worldMatrix.identity();
    }
    getWorldMatrixNonInterp(_worldMatrix, local, dirty);
    _dirty = true;

    return _worldMatrix;
}

/// Return the scale factor
vec3<F32> PhysicsComponent::getScale(D32 interpolationFactor,
                                     const bool local) const {
    vec3<F32> scale;
    if (_transform) {
        if (interpolationFactor < 0.975) {
            scale.set(Lerp(_prevTransformValues._scale, _transform->getScale(),
                           to_float(interpolationFactor)));
        } else {
            scale.set(_transform->getScale());
        }
    } else {
        scale.set(1.0f);
    }

    if (!local) {
        SceneGraphNode_ptr grandParent = _parentSGN.getParent().lock();
        if (grandParent) {
            scale *= grandParent->getComponent<PhysicsComponent>()->getScale(
                interpolationFactor, local);
        }
    }

    return scale;
}

/// Return the position
vec3<F32> PhysicsComponent::getPosition(D32 interpolationFactor,
                                        const bool local) const {
    vec3<F32> position;
    if (_transform) {
        if (interpolationFactor < 0.975) {
            position.set(Lerp(_prevTransformValues._translation,
                              _transform->getPosition(),
                              to_float(interpolationFactor)));
        } else {
            position.set(_transform->getPosition());
        }
    } else {
        position.set(0.0f);
    }

    if (!local) {
        SceneGraphNode_ptr grandParent = _parentSGN.getParent().lock();
        if (grandParent) {
            position += grandParent->getComponent<PhysicsComponent>()->getPosition(
                interpolationFactor, local);
        }
    }
    return position;
}

/// Return the orientation quaternion
Quaternion<F32> PhysicsComponent::getOrientation(D32 interpolationFactor,
                                                 const bool local) const {
    Quaternion<F32> orientation;
    
    if (_transform) {
        if (interpolationFactor < 0.975) {
            orientation.set(Slerp(_prevTransformValues._orientation,
                                  _transform->getOrientation(),
                                  to_float(interpolationFactor)));
        } else {
            orientation.set(_transform->getOrientation());
        }
    } else {
        orientation.identity();
    }

    SceneGraphNode_ptr grandParent = _parentSGN.getParent().lock();
    if (!local && grandParent) {
        orientation.set(grandParent->getComponent<PhysicsComponent>()
                            ->getOrientation(interpolationFactor, local) * orientation);
    }

    return orientation;
}
};