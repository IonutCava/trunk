#include "Headers/PhysicsComponent.h"

#include "Core/Headers/Console.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Physics/Headers/PXDevice.h"
#include "Physics/Headers/PhysicsAsset.h"
#include "Physics/Headers/PhysicsAPIWrapper.h"

namespace Divide {

PhysicsComponent::PhysicsComponent(SceneGraphNode& parentSGN, PhysicsGroup physicsGroup)
    : SGNComponent(SGNComponent::ComponentType::PHYSICS, parentSGN),
      TransformInterface(),
      _physicsCollisionGroup(physicsGroup),
      _dirty(true),
      _dirtyInterp(true),
      _parentDirty(true),
      _isUniformScaled(true),
      _prevInterpValue(0.0)
{
    if (physicsDriven()) {
        _transformInterface.reset(PHYSICS_DEVICE.createRigidActor(parentSGN));
    } else {
        _transformInterface = std::make_unique<Transform>();
    }

    reset();
}

PhysicsComponent::~PhysicsComponent()
{
}

void PhysicsComponent::update(const U64 deltaTime) {
    if (!physicsDriven()) {
        _prevTransformValues = getTransform()->getValues();
    }

    _parentDirty = isParentTransformDirty();

    if (_transformUpdatedMask.hasSetFlags()) {
        if (_transformUpdatedMask.getFlag(TransformType::SCALE)) {
            _isUniformScaled = getScale().isUniform();
        }

        for (DELEGATE_CBK<>& cbk : _transformCallbacks) {
            cbk();
        }
        
        _transformUpdatedMask.clearAllFlags();
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

void PhysicsComponent::ignoreView(const bool state, const I64 cameraGUID) {
    _ignoreViewSettings._cameraGUID = cameraGUID;
    setTransformDirty(TransformType::VIEW_OFFSET);
}

void PhysicsComponent::setViewOffset(const vec3<F32>& posOffset, const mat4<F32>& rotationOffset) {
    _ignoreViewSettings._posOffset.set(posOffset);
    _ignoreViewSettings._transformOffset.set(rotationOffset);

    _dirty = _dirtyInterp = true;

    setTransformDirty(TransformType::VIEW_OFFSET);
}

void PhysicsComponent::cookCollisionMesh(const stringImpl& sceneName) {
   
}

void PhysicsComponent::setTransformDirty(TransformType type) {
    _transformUpdatedMask.setFlag(type);
    _dirty = true;
    _dirtyInterp = true;
}

void PhysicsComponent::setPosition(const vec3<F32>& position) {
    _transformInterface->setPosition(position);
    setTransformDirty(TransformType::TRANSLATION);
}

void PhysicsComponent::setScale(const vec3<F32>& scale) {
    _transformInterface->setScale(scale);
    setTransformDirty(TransformType::SCALE);
}

void PhysicsComponent::setRotation(const Quaternion<F32>& quat) {
    _transformInterface->setRotation(quat);
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::setRotation(const vec3<F32>& axis, F32 degrees,  bool inDegrees) {
    _transformInterface->setRotation(axis, degrees, inDegrees);
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::setRotation(F32 pitch, F32 yaw, F32 roll, bool inDegrees) {
    _transformInterface->setRotation(pitch, yaw, roll, inDegrees);
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::translate(const vec3<F32>& axisFactors) {
    _transformInterface->translate(axisFactors);
    setTransformDirty(TransformType::TRANSLATION);
}

void PhysicsComponent::scale(const vec3<F32>& axisFactors) {
    _transformInterface->scale(axisFactors);
    setTransformDirty(TransformType::SCALE);
}

void PhysicsComponent::rotate(const vec3<F32>& axis, F32 degrees, bool inDegrees) {
    _transformInterface->rotate(axis, degrees, inDegrees);
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::rotate(F32 pitch, F32 yaw, F32 roll, bool inDegrees) {
    _transformInterface->rotate(pitch, yaw, roll, inDegrees);
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::rotate(const Quaternion<F32>& quat) {
    _transformInterface->rotate(quat);
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::rotateSlerp(const Quaternion<F32>& quat, const D64 deltaTime) {
   _transformInterface->rotateSlerp(quat, deltaTime);
   setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::setScaleX(const F32 ammount) {
    _transformInterface->setScaleX(ammount);
    setTransformDirty(TransformType::SCALE);
}

void PhysicsComponent::setScaleY(const F32 ammount) {
    _transformInterface->setScaleY(ammount);
    setTransformDirty(TransformType::SCALE);
}

void PhysicsComponent::setScaleZ(const F32 ammount) {
    _transformInterface->setScaleZ(ammount);
    setTransformDirty(TransformType::SCALE);
}

void PhysicsComponent::scaleX(const F32 scale) {
    _transformInterface->scaleX(scale);
    setTransformDirty(TransformType::SCALE);
}

void PhysicsComponent::scaleY(const F32 scale) {
    _transformInterface->scaleY(scale);
    setTransformDirty(TransformType::SCALE);
}

void PhysicsComponent::scaleZ(const F32 scale) {
    _transformInterface->scaleZ(scale);
    setTransformDirty(TransformType::SCALE);
}

void PhysicsComponent::rotateX(const F32 angle, bool inDegrees) {
    _transformInterface->rotateX(angle, inDegrees);
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::rotateY(const F32 angle, bool inDegrees) {
    _transformInterface->rotateY(angle, inDegrees);
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::rotateZ(const F32 angle, bool inDegrees) {
    _transformInterface->rotateZ(angle, inDegrees);
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::setRotationX(const F32 angle, bool inDegrees) {
    _transformInterface->setRotationX(angle, inDegrees);
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::setRotationY(const F32 angle, bool inDegrees) {
    _transformInterface->setRotationY(angle, inDegrees);
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::setRotationZ(const F32 angle, bool inDegrees) {
    _transformInterface->setRotationZ(angle, inDegrees);
    setTransformDirty(TransformType::ROTATION);
}

void PhysicsComponent::setPositionX(const F32 positionX) {
    _transformInterface->setPositionX(positionX);
    setTransformDirty(TransformType::TRANSLATION);
}

void PhysicsComponent::setPositionY(const F32 positionY) {
    _transformInterface->setPositionY(positionY);
    setTransformDirty(TransformType::TRANSLATION);
}

void PhysicsComponent::setPositionZ(const F32 positionZ) {
    _transformInterface->setPositionZ(positionZ);
    setTransformDirty(TransformType::TRANSLATION);
}

void PhysicsComponent::pushTransforms() {
    if (!physicsDriven()) {
        _transformStack.push(getTransform()->getValues());
    }
}

bool PhysicsComponent::popTransforms() {
    if (!physicsDriven() && !_transformStack.empty()) {
        _prevTransformValues = _transformStack.top();
        getTransform()->setValues(_prevTransformValues);
        _transformStack.pop();

        setTransformDirty(TransformType::TRANSLATION);
        setTransformDirty(TransformType::SCALE);
        setTransformDirty(TransformType::ROTATION);
        return true;
    }
    return false;
}

bool PhysicsComponent::isParentTransformDirty() const {
    SceneGraphNode_ptr parent = _parentSGN.getParent().lock();
    while (parent != nullptr) {
        PhysicsComponent* parentComp = parent->get<PhysicsComponent>();
        if(parentComp->_dirtyInterp || parentComp->_dirty)
        {
            return true;
        }

        parent = parent->getParent().lock();
    }

    return false;
}

void PhysicsComponent::clean(bool interp) {
    if (interp) {
        _dirtyInterp = false;
    } else {
        _dirty = false;
    }
}

const mat4<F32>& PhysicsComponent::getMatrix() {
    return _transformInterface->getMatrix();
}

const mat4<F32>& PhysicsComponent::getWorldMatrix(D64 interpolationFactor) {
    bool dirty = (_dirtyInterp ||
                  !COMPARE(_prevInterpValue, interpolationFactor) ||
                  _parentDirty);

    if (dirty) {
        SceneGraphNode_wptr grandParentPtr = _parentSGN.getParent();
        _worldMatrixInterp.set(getLocalPosition(interpolationFactor),
                               getLocalScale(interpolationFactor),
                               GetMatrix(getLocalOrientation(interpolationFactor)));
        if (!grandParentPtr.expired()) {
            _worldMatrixInterp *= grandParentPtr.lock()->get<PhysicsComponent>()->getWorldMatrix(interpolationFactor);
        }
        clean(true);
    }

    if (_ignoreViewSettings._cameraGUID != -1) {
        _worldMatrixInterp = _worldMatrixInterp * _ignoreViewSettings._transformOffset;
    }

    return _worldMatrixInterp;
}

const mat4<F32>& PhysicsComponent::getWorldMatrix() {
    if (_dirty || _parentDirty){
        _worldMatrix.set(getMatrix());

        SceneGraphNode_wptr grandParentPtr = _parentSGN.getParent();
        if (!grandParentPtr.expired()) {
            _worldMatrix *= grandParentPtr.lock()->get<PhysicsComponent>()->getWorldMatrix();
        }
        clean(false);
    }

    if (_ignoreViewSettings._cameraGUID != -1) {
        _worldMatrix = _worldMatrix * _ignoreViewSettings._transformOffset;
    }

    return _worldMatrix;
}

/// Return the position
vec3<F32> PhysicsComponent::getPosition() const {
    vec3<F32> position(getLocalPosition());

    SceneGraphNode_ptr grandParent = _parentSGN.getParent().lock();
    if (grandParent) {
        position += grandParent->get<PhysicsComponent>()->getPosition();
    }

    return position;
}

vec3<F32> PhysicsComponent::getLocalPosition() const {
    return _transformInterface->getPosition();
}

/// Return the position
vec3<F32> PhysicsComponent::getPosition(D64 interpolationFactor) const {
    vec3<F32> position(getLocalPosition(interpolationFactor));

    SceneGraphNode_ptr grandParent = _parentSGN.getParent().lock();
    if (grandParent) {
        position += grandParent->get<PhysicsComponent>()->getPosition(interpolationFactor);
    }
    
    return position;
}

vec3<F32> PhysicsComponent::getLocalPosition(D64 interpolationFactor) const {
    if (physicsDriven()) {
        return getLocalPosition();
    }

    return Lerp(_prevTransformValues._translation,
                getLocalPosition(),
                to_float(interpolationFactor));
}

vec3<F32> PhysicsComponent::getScale() const {
    vec3<F32> scale(getLocalScale());

    SceneGraphNode_ptr grandParent = _parentSGN.getParent().lock();
    if (grandParent) {
        scale *= grandParent->get<PhysicsComponent>()->getScale();
    }

    return scale;
}

vec3<F32> PhysicsComponent::getLocalScale() const {
    return _transformInterface->getScale();
}

/// Return the scale factor
vec3<F32> PhysicsComponent::getScale(D64 interpolationFactor) const {
    vec3<F32> scale(getLocalScale(interpolationFactor));

    SceneGraphNode_ptr grandParent = _parentSGN.getParent().lock();
    if (grandParent) {
        scale *= grandParent->get<PhysicsComponent>()->getScale(interpolationFactor);
    }

    return scale;
}

vec3<F32> PhysicsComponent::getLocalScale(D64 interpolationFactor) const {
    if (physicsDriven()) {
        return getLocalScale();
    }

    return Lerp(_prevTransformValues._scale,
                getLocalScale(),
                to_float(interpolationFactor));
}

/// Return the orientation quaternion
Quaternion<F32> PhysicsComponent::getOrientation() const {
    Quaternion<F32> orientation(getLocalOrientation());

    SceneGraphNode_ptr grandParent = _parentSGN.getParent().lock();
    if (grandParent) {
        orientation.set(grandParent->get<PhysicsComponent>()->getOrientation() * orientation);
    }

    return orientation;

}

Quaternion<F32> PhysicsComponent::getLocalOrientation() const {
    return _transformInterface->getOrientation();
}

Quaternion<F32> PhysicsComponent::getOrientation(D64 interpolationFactor) const {
    Quaternion<F32> orientation(getLocalOrientation(interpolationFactor));

    SceneGraphNode_ptr grandParent = _parentSGN.getParent().lock();
    if (grandParent) {
        orientation.set(grandParent->get<PhysicsComponent>()->getOrientation(interpolationFactor) * orientation);
    }

    return orientation;
}

Quaternion<F32> PhysicsComponent::getLocalOrientation(D64 interpolationFactor) const {
    if (physicsDriven()) {
        return getLocalOrientation();
    }

    return Slerp(_prevTransformValues._orientation,
                 getLocalOrientation(),
                 to_float(interpolationFactor));
}

bool PhysicsComponent::isUniformScaled() const {
    return _transformInterface->getScale().isUniform();
}

Transform* PhysicsComponent::getTransform() const {
    assert(!physicsDriven());
    return static_cast<Transform*>(_transformInterface.get());
}

PhysicsAsset* PhysicsComponent::getPhysicsAsset() const {
    assert(physicsDriven());
    return static_cast<PhysicsAsset*>(_transformInterface.get());
}

bool PhysicsComponent::physicsDriven() const {
    return (_physicsCollisionGroup != PhysicsGroup::GROUP_IGNORE &&
            _physicsCollisionGroup != PhysicsGroup::GROUP_COUNT);
}

};