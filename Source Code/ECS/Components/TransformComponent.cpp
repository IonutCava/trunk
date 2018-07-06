#include "stdafx.h"

#include "Headers/TransformComponent.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "ECS/Events/Headers/TransformEvents.h"

namespace Divide {
    TransformComponent::TransformComponent(SceneGraphNode& parentSGN)
      : SGNComponent(SGNComponent::ComponentType::PHYSICS, parentSGN),
        _dirty(true),
        _dirtyInterp(true),
        _parentDirty(true),
        _prevInterpValue(0.0)
    {
        _transformInterface = std::make_unique<Transform>();
        RegisterEventCallbacks();
    }

    TransformComponent::~TransformComponent()
    {
        UnregisterAllEventCallbacks();
    }

    void TransformComponent::RegisterEventCallbacks() {
        RegisterEventCallback(&TransformComponent::OnParentTransformDirty);
        RegisterEventCallback(&TransformComponent::OnParentTransformClean);
    }

    void TransformComponent::OnParentTransformDirty(const ParentTransformDirty* event) {
        if (GetOwner() == event->ownerID) {
            _parentDirty = true;
        }
    }

    void TransformComponent::OnParentTransformClean(const ParentTransformClean* event) {
        if (GetOwner() == event->ownerID) {
            _parentDirty = false;
        }
    }

    void TransformComponent::reset() {
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

    void TransformComponent::snapshot() {
        ReadLock r_lock(_lock);
        _transformInterface->getValues(_prevTransformValues);
    }


    void TransformComponent::notifyListeners() {
        if (_transformUpdatedMask.hasSetFlags()) {
            for (DELEGATE_CBK<void>& cbk : _transformCallbacks) {
                cbk();
            }

            _transformUpdatedMask.clearAllFlags();
        }
    }

    void TransformComponent::ignoreView(const bool state, const I64 cameraGUID) {
        _ignoreViewSettings._cameraGUID = cameraGUID;
        setTransformDirty(TransformType::VIEW_OFFSET);
    }

    void TransformComponent::setViewOffset(const vec3<F32>& posOffset, const mat4<F32>& rotationOffset) {
        _ignoreViewSettings._posOffset.set(posOffset);
        _ignoreViewSettings._transformOffset.set(rotationOffset);

        setTransformDirty(TransformType::VIEW_OFFSET);
    }

    void TransformComponent::setTransformDirty(TransformType type) {
        _transformUpdatedMask.setFlag(type);
        _dirty = true;
        _dirtyInterp = true;
        ECS::ECS_Engine->SendEvent<TransformDirty>(GetOwner(), type);
    }

    void TransformComponent::setPosition(const vec3<F32>& position) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->setPosition(position);
        }

        setTransformDirty(TransformType::TRANSLATION);
    }

    void TransformComponent::setScale(const vec3<F32>& scale) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->setScale(scale);
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::setRotation(const Quaternion<F32>& quat) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->setRotation(quat);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setRotation(const vec3<F32>& axis, Angle::DEGREES<F32> degrees) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->setRotation(axis, degrees);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setRotation(Angle::DEGREES<F32> pitch, Angle::DEGREES<F32> yaw, Angle::DEGREES<F32> roll) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->setRotation(pitch, yaw, roll);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::translate(const vec3<F32>& axisFactors) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->translate(axisFactors);
        }

        setTransformDirty(TransformType::TRANSLATION);
    }

    void TransformComponent::scale(const vec3<F32>& axisFactors) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->scale(axisFactors);
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::rotate(const vec3<F32>& axis, Angle::DEGREES<F32> degrees) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->rotate(axis, degrees);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::rotate(Angle::DEGREES<F32> pitch, Angle::DEGREES<F32> yaw, Angle::DEGREES<F32> roll) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->rotate(pitch, yaw, roll);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::rotate(const Quaternion<F32>& quat) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->rotate(quat);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::rotateSlerp(const Quaternion<F32>& quat, const D64 deltaTime) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->rotateSlerp(quat, deltaTime);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setScaleX(const F32 ammount) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->setScaleX(ammount);
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::setScaleY(const F32 ammount) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->setScaleY(ammount);
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::setScaleZ(const F32 ammount) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->setScaleZ(ammount);
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::scaleX(const F32 scale) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->scaleX(scale);
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::scaleY(const F32 scale) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->scaleY(scale);
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::scaleZ(const F32 scale) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->scaleZ(scale);
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::rotateX(const Angle::DEGREES<F32> angle) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->rotateX(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::rotateY(const Angle::DEGREES<F32> angle) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->rotateY(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::rotateZ(const Angle::DEGREES<F32> angle) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->rotateZ(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setRotationX(const Angle::DEGREES<F32> angle) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->setRotationX(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setRotationY(const Angle::DEGREES<F32> angle) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->setRotationY(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setRotationZ(const Angle::DEGREES<F32> angle) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->setRotationZ(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setPositionX(const F32 positionX) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->setPositionX(positionX);
        }

        setTransformDirty(TransformType::TRANSLATION);
    }

    void TransformComponent::setPositionY(const F32 positionY) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->setPositionY(positionY);
        }

        setTransformDirty(TransformType::TRANSLATION);
    }

    void TransformComponent::setPositionZ(const F32 positionZ) {
        {
            WriteLock w_lock(_lock);
            _transformInterface->setPositionZ(positionZ);
        }

        setTransformDirty(TransformType::TRANSLATION);
    }

    void TransformComponent::pushTransforms() {
        TransformValues temp;
        {
            ReadLock r_lock(_lock);
            _transformInterface->getValues(temp);
        }

        _transformStack.push(temp);
    }

    bool TransformComponent::popTransforms() {
        if (!_transformStack.empty()) {
            _prevTransformValues = _transformStack.top();
            {
                WriteLock w_lock(_lock);
                static_cast<Transform*>(_transformInterface.get())->setValues(_prevTransformValues);
            }

            _transformStack.pop();

            setTransformDirty(TransformType::TRANSLATION);
            setTransformDirty(TransformType::SCALE);
            setTransformDirty(TransformType::ROTATION);
            return true;
        }
        return false;
    }


    void TransformComponent::getValues(TransformValues& valuesOut) const {
        ReadLock r_lock(_lock);
        _transformInterface->getValues(valuesOut);
    }

    const mat4<F32>& TransformComponent::getMatrix() {
        ReadLock r_lock(_lock);
        return _transformInterface->getMatrix();
    }

    const mat4<F32>& TransformComponent::getWorldMatrix(D64 interpolationFactor) {
        bool dirty = (_dirtyInterp ||
                      !COMPARE(_prevInterpValue, interpolationFactor) ||
                      _parentDirty);

        if (dirty) {
            SceneGraphNode_wptr grandParentPtr = _parentSGN.getParent();
            _worldMatrixInterp.set(getLocalPosition(interpolationFactor),
                                   getLocalScale(interpolationFactor),
                                   mat4<F32>(GetMatrix(getLocalOrientation(interpolationFactor)), false));
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

    const mat4<F32>& TransformComponent::getWorldMatrix() {
        if (_dirty || _parentDirty) {
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
    vec3<F32> TransformComponent::getPosition() const {
        vec3<F32> position(getLocalPosition());

        SceneGraphNode_ptr grandParent = _parentSGN.getParent().lock();
        if (grandParent) {
            position += grandParent->get<PhysicsComponent>()->getPosition();
        }

        return position;
    }

    vec3<F32> TransformComponent::getLocalPosition() const {
        vec3<F32> pos;
        getPosition(pos);
        return pos;
    }

    /// Return the position
    vec3<F32> TransformComponent::getPosition(D64 interpolationFactor) const {
        vec3<F32> position(getLocalPosition(interpolationFactor));

        SceneGraphNode_ptr grandParent = _parentSGN.getParent().lock();
        if (grandParent) {
            position += grandParent->get<PhysicsComponent>()->getPosition(interpolationFactor);
        }

        return position;
    }

    vec3<F32> TransformComponent::getLocalPosition(D64 interpolationFactor) const {
        return Lerp(_prevTransformValues._translation,
                    getLocalPosition(),
                    to_F32(interpolationFactor));
    }

    vec3<F32> TransformComponent::getScale() const {
        vec3<F32> scale(getLocalScale());

        SceneGraphNode_ptr grandParent = _parentSGN.getParent().lock();
        if (grandParent) {
            scale *= grandParent->get<PhysicsComponent>()->getScale();
        }

        return scale;
    }

    vec3<F32> TransformComponent::getLocalScale() const {
        vec3<F32> scale;
        getScale(scale);
        return scale;
    }

    /// Return the scale factor
    vec3<F32> TransformComponent::getScale(D64 interpolationFactor) const {
        vec3<F32> scale(getLocalScale(interpolationFactor));

        SceneGraphNode_ptr grandParent = _parentSGN.getParent().lock();
        if (grandParent) {
            scale *= grandParent->get<PhysicsComponent>()->getScale(interpolationFactor);
        }

        return scale;
    }

    vec3<F32> TransformComponent::getLocalScale(D64 interpolationFactor) const {
        return Lerp(_prevTransformValues._scale,
                    getLocalScale(),
                    to_F32(interpolationFactor));
    }

    /// Return the orientation quaternion
    Quaternion<F32> TransformComponent::getOrientation() const {
        Quaternion<F32> orientation(getLocalOrientation());

        SceneGraphNode_ptr grandParent = _parentSGN.getParent().lock();
        if (grandParent) {
            orientation.set(grandParent->get<PhysicsComponent>()->getOrientation() * orientation);
        }

        return orientation;
    }

    Quaternion<F32> TransformComponent::getLocalOrientation() const {
        Quaternion<F32> quat;
        getOrientation(quat);
        return quat;
    }

    Quaternion<F32> TransformComponent::getOrientation(D64 interpolationFactor) const {
        Quaternion<F32> orientation(getLocalOrientation(interpolationFactor));

        SceneGraphNode_ptr grandParent = _parentSGN.getParent().lock();
        if (grandParent) {
            orientation.set(grandParent->get<PhysicsComponent>()->getOrientation(interpolationFactor) * orientation);
        }

        return orientation;
    }

    Quaternion<F32> TransformComponent::getLocalOrientation(D64 interpolationFactor) const {
        return Slerp(_prevTransformValues._orientation,
                     getLocalOrientation(),
                     to_F32(interpolationFactor));
    }

    // Transform interface access
    void TransformComponent::getScale(vec3<F32>& scaleOut) const {
        ReadLock r_lock(_lock);

        _transformInterface->getScale(scaleOut);
    }

    void TransformComponent::getPosition(vec3<F32>& posOut) const {
        ReadLock r_lock(_lock);

        _transformInterface->getPosition(posOut);
    }

    void TransformComponent::getOrientation(Quaternion<F32>& quatOut) const {
        ReadLock r_lock(_lock);

        _transformInterface->getOrientation(quatOut);
    }

    bool TransformComponent::isUniformScaled() const {
        return getLocalScale().isUniform();
    }

    bool TransformComponent::dirty() const {
        return _dirty || _dirtyInterp;
    }

    void TransformComponent::clean(bool interp) {
        if (interp) {
            _dirtyInterp = false;
        } else {
            _dirty = false;
            ECS::ECS_Engine->SendEvent<TransformClean>(GetOwner());
        }
    }
}; //namespace