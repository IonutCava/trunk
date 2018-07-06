#include "Headers/PhysicsComponent.h"

#include "Core/Headers/Console.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Physics/Headers/PXDevice.h"
#include "Physics/Headers/PhysicsAPIWrapper.h"

namespace Divide {

PhysicsComponent::PhysicsComponent(SceneGraphNode& parentSGN)
    : SGNComponent(SGNComponent::ComponentType::SGN_COMP_PHYSICS, parentSGN),
      _physicsCollisionGroup(PhysicsGroup::NODE_COLLIDE_IGNORE),
      _transformUpdated(true),
      _physicsAsset(nullptr),
      _transform(nullptr) {
    _transform = MemoryManager_NEW Transform();
    reset();
}

PhysicsComponent::~PhysicsComponent() { MemoryManager::DELETE(_transform); }

void PhysicsComponent::reset() {
    _worldMatrix.identity();
    _scaleCache.set(1.0f);
    _positionCache.set(0.0f);
    _orientationCache.identity();
    _prevTransformValues._translation.set(0.0f);
    _prevTransformValues._scale.set(1.0f);
    _prevTransformValues._orientation.identity();
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

void PhysicsComponent::physicsAsset(PhysicsAsset* const asset) {
    DIVIDE_ASSERT(_physicsAsset == nullptr,
                  "PhysicsComponent error: Double set physics asset detected! "
                  "remove the previous one first!");
    _physicsAsset = asset;
    if (_physicsAsset) {
        _physicsAsset->setParent(this);
    }
}

void PhysicsComponent::cookCollisionMesh(const stringImpl& sceneName) {
    STUBBED("ToDo: add terrain height field and water cooking support! -Ionut")
    for (SceneGraphNode::NodeChildren::value_type& it :
         _parentSGN.getChildren()) {
        it.second->getComponent<PhysicsComponent>()->cookCollisionMesh(
            sceneName);
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

void PhysicsComponent::setTransformDirty() {
    if (_physicsAsset) {
        _physicsAsset->resetTransforms(true);
    }
    _transformUpdated = true;
}

void PhysicsComponent::setPosition(const vec3<F32>& position) {
    if (_transform) {
        _transform->setPosition(position);
        setTransformDirty();
    }
}

void PhysicsComponent::setScale(const vec3<F32>& scale) {
    if (_transform) {
        _transform->setScale(scale);
        setTransformDirty();
    }
}

void PhysicsComponent::setRotation(const vec3<F32>& axis, F32 degrees,
                                   bool inDegrees) {
    if (_transform) {
        _transform->setRotation(axis, degrees, inDegrees);
        setTransformDirty();
    }
}

void PhysicsComponent::setRotation(const vec3<F32>& euler, bool inDegrees) {
    if (_transform) {
        _transform->setRotation(euler, inDegrees);
        setTransformDirty();
    }
}

void PhysicsComponent::setRotation(const Quaternion<F32>& quat) {
    if (_transform) {
        _transform->setRotation(quat);
        setTransformDirty();
    }
}

void PhysicsComponent::translate(const vec3<F32>& axisFactors) {
    if (_transform) {
        _transform->translate(axisFactors);
        setTransformDirty();
    }
}

void PhysicsComponent::scale(const vec3<F32>& axisFactors) {
    if (_transform) {
        _transform->scale(axisFactors);
        setTransformDirty();
    }
}

void PhysicsComponent::rotate(const vec3<F32>& axis, F32 degrees,
                              bool inDegrees) {
    if (_transform) {
        _transform->rotate(axis, degrees, inDegrees);
        setTransformDirty();
    }
}

void PhysicsComponent::rotate(const vec3<F32>& euler, bool inDegrees) {
    if (_transform) {
        _transform->rotate(euler, inDegrees);
        setTransformDirty();
    }
}

void PhysicsComponent::rotate(const Quaternion<F32>& quat) {
    if (_transform) {
        _transform->rotate(quat);
        setTransformDirty();
    }
}

void PhysicsComponent::rotateSlerp(const Quaternion<F32>& quat,
                                   const D32 deltaTime) {
    if (_transform) {
        _transform->rotateSlerp(quat, deltaTime);
        setTransformDirty();
    }
}

void PhysicsComponent::setScale(const F32 scale) {
    if (_transform) {
        _transform->setScale(scale);
        setTransformDirty();
    }
}

void PhysicsComponent::setScaleX(const F32 scale) {
    if (_transform) {
        _transform->setScaleX(scale);
        setTransformDirty();
    }
}

void PhysicsComponent::setScaleY(const F32 scale) {
    if (_transform) {
        _transform->setScaleY(scale);
        setTransformDirty();
    }
}

void PhysicsComponent::setScaleZ(const F32 scale) {
    if (_transform) {
        _transform->setScaleZ(scale);
        setTransformDirty();
    }
}

void PhysicsComponent::scale(const F32 scale) {
    if (_transform) {
        _transform->scale(scale);
        setTransformDirty();
    }
}

void PhysicsComponent::scaleX(const F32 scale) {
    if (_transform) {
        _transform->scaleX(scale);
        setTransformDirty();
    }
}

void PhysicsComponent::scaleY(const F32 scale) {
    if (_transform) {
        _transform->scaleY(scale);
        setTransformDirty();
    }
}

void PhysicsComponent::scaleZ(const F32 scale) {
    if (_transform) {
        _transform->scaleZ(scale);
        setTransformDirty();
    }
}

void PhysicsComponent::rotateX(const F32 angle, bool inDegrees) {
    if (_transform) {
        _transform->rotateX(angle, inDegrees);
        setTransformDirty();
    }
}

void PhysicsComponent::rotateY(const F32 angle, bool inDegrees) {
    if (_transform) {
        _transform->rotateY(angle, inDegrees);
        setTransformDirty();
    }
}

void PhysicsComponent::rotateZ(const F32 angle, bool inDegrees) {
    if (_transform) {
        _transform->rotateZ(angle, inDegrees);
        setTransformDirty();
    }
}

void PhysicsComponent::setRotationX(const F32 angle, bool inDegrees) {
    if (_transform) {
        _transform->setRotationX(angle, inDegrees);
        setTransformDirty();
    }
}

void PhysicsComponent::setRotationXEuler(const F32 angle, bool inDegrees) {
    if (_transform) {
        _transform->setRotationXEuler(angle, inDegrees);
        setTransformDirty();
    }
}

void PhysicsComponent::setRotationY(const F32 angle, bool inDegrees) {
    if (_transform) {
        _transform->setRotationY(angle, inDegrees);
        setTransformDirty();
    }
}

void PhysicsComponent::setRotationZ(const F32 angle, bool inDegrees) {
    if (_transform) {
        _transform->setRotationZ(angle, inDegrees);
        setTransformDirty();
    }
}

void PhysicsComponent::translateX(const F32 positionX) {
    if (_transform) {
        _transform->translateX(positionX);
        setTransformDirty();
    }
}

void PhysicsComponent::translateY(const F32 positionY) {
    if (_transform) {
        _transform->translateY(positionY);
        setTransformDirty();
    }
}

void PhysicsComponent::translateZ(const F32 positionZ) {
    if (_transform) {
        _transform->translateZ(positionZ);
        setTransformDirty();
    }
}

void PhysicsComponent::setPositionX(const F32 positionX) {
    if (_transform) {
        _transform->setPositionX(positionX);
        setTransformDirty();
    }
}

void PhysicsComponent::setPositionY(const F32 positionY) {
    if (_transform) {
        _transform->setPositionY(positionY);
        setTransformDirty();
    }
}

void PhysicsComponent::setPositionZ(const F32 positionZ) {
    if (_transform) {
        _transform->setPositionZ(positionZ);
        setTransformDirty();
    }
}

const mat4<F32>& PhysicsComponent::getWorldMatrix(D32 interpolationFactor,
                                                  const bool local) {
    if (_transform) {
        if (interpolationFactor < 0.95) {
            _worldMatrix.identity();
            _worldMatrix.setScale(lerp(_prevTransformValues._scale,
                                       _transform->getScale(),
                                       (F32)interpolationFactor));
            _worldMatrix *= Divide::getMatrix(
                slerp(_prevTransformValues._orientation,
                      _transform->getOrientation(), (F32)interpolationFactor));
            _worldMatrix.setTranslation(lerp(_prevTransformValues._translation,
                                             _transform->getPosition(),
                                             (F32)interpolationFactor));
        } else {
            _worldMatrix.set(_transform->getMatrix());
        }
        _prevTransformValues = _transform->getValues();
    } else {
        _worldMatrix.identity();
    }

    const SceneGraphNode* const parent = _parentSGN.getParent();
    if (!local && parent) {
        _worldMatrix *=
            parent->getComponent<PhysicsComponent>()->getWorldMatrix(
                interpolationFactor, local);
    }

    return _worldMatrix;
}

/// Return the scale factor
const vec3<F32>& PhysicsComponent::getScale(const bool local) {
    if (_transform) {
        _scaleCache.set(_transform->getScale());
    } else {
        _scaleCache.set(1.0f);
    }

    const SceneGraphNode* const parent = _parentSGN.getParent();
    if (!local && parent) {
        _scaleCache *=
            parent->getComponent<PhysicsComponent>()->getScale(local);
    }

    return _scaleCache;
}

/// Return the position
const vec3<F32>& PhysicsComponent::getPosition(const bool local) {
    if (_transform) {
        _positionCache.set(_transform->getPosition());
    } else {
        _positionCache.set(0.0f);
    }

    const SceneGraphNode* const parent = _parentSGN.getParent();
    if (!local && parent) {
        _positionCache +=
            parent->getComponent<PhysicsComponent>()->getPosition(local);
    }

    return _positionCache;
}

/// Return the orientation quaternion
const Quaternion<F32>& PhysicsComponent::getOrientation(const bool local) {
    if (_transform) {
        _orientationCache.set(_transform->getOrientation());
    } else {
        _orientationCache.identity();
    }

    const SceneGraphNode* const parent = _parentSGN.getParent();
    if (!local && parent) {
        _orientationCache.set(parent->getComponent<PhysicsComponent>()
                                  ->getOrientation(local)
                                  .inverse() *
                              _orientationCache);
    }

    return _orientationCache;
}
};