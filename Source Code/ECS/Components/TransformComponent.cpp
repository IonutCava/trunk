#include "stdafx.h"

#include "Headers/TransformComponent.h"

#include "Core/Headers/ByteBuffer.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "ECS/Events/Headers/TransformEvents.h"

namespace Divide {
    TransformComponent::TransformComponent(SceneGraphNode& parentSGN, PlatformContext& context)
      : BaseComponentType<TransformComponent, ComponentType::TRANSFORM>(parentSGN, context),
        _uniformScaled(false),
        _parentUsageContext(parentSGN.usageContext())
    {
        _worldMatrixUpToDate.clear();

        _transformUpdatedMask.store(to_base(TransformType::ALL));

        _editorComponent.registerField("Transform",
                                       this,
                                       EditorComponentFieldType::TRANSFORM,
                                       false);
        _editorComponent.registerField("WorldMat", &_worldMatrix, EditorComponentFieldType::PUSH_TYPE, true, GFX::PushConstantType::MAT4);
        if (_transformOffset.first) {
            _editorComponent.registerField("Transform Offset", &_transformOffset.second, EditorComponentFieldType::PUSH_TYPE, true, GFX::PushConstantType::MAT4);
        }

        _editorComponent.onChangedCbk([this](EditorComponentField& field) {
            if (field._name == "Transform") {
                setTransformDirty(to_base(TransformType::ALL));
            } else if (field._name == "Position Offset") {
                // view offset stuff
            }

            _hasChanged = true;
        });
    }

    TransformComponent::~TransformComponent()
    {
        
    }

    void TransformComponent::onParentTransformDirty(U32 transformMask) {
        if (transformMask != to_base(TransformType::NONE)) {
            _worldMatrixUpToDate.clear();
        }
    }

    void TransformComponent::onParentUsageChanged(NodeUsageContext context) {
        _parentUsageContext = context;
    }

    void TransformComponent::reset() {
        _worldMatrix.identity();
        _prevTransformValues._translation.set(0.0f);
        _prevTransformValues._scale.set(1.0f);
        _prevTransformValues._orientation.identity();
        while (!_transformStack.empty()) {
            _transformStack.pop();
        }
        
        _worldMatrixUpToDate.clear();

        _transformUpdatedMask.store(to_base(TransformType::ALL));
    }

    void TransformComponent::PreUpdate(const U64 deltaTimeUS) {
        // If we have dirty transforms, inform everybody
        if (_transformUpdatedMask.load() != to_base(TransformType::NONE))
        {
            Attorney::SceneGraphNodeComponent::setTransformDirty(_parentSGN, _transformUpdatedMask);
        }

        BaseComponentType<TransformComponent, ComponentType::TRANSFORM>::PreUpdate(deltaTimeUS);
    }

    void TransformComponent::Update(const U64 deltaTimeUS) {
        // Cleanup our dirty transforms
        if (_transformUpdatedMask.exchange(to_U32(TransformType::NONE) != to_U32(TransformType::NONE)))
        {
            _worldMatrixUpToDate.clear();
        }

        BaseComponentType<TransformComponent, ComponentType::TRANSFORM>::Update(deltaTimeUS);
    }

    void TransformComponent::PostUpdate(const U64 deltaTimeUS) {
        updateWorldMatrix();
        BaseComponentType<TransformComponent, ComponentType::TRANSFORM>::PostUpdate(deltaTimeUS);
    }

    void TransformComponent::OnUpdateLoop() {
        SharedLock r_lock(_lock);
        _transformInterface.getValues(_prevTransformValues);
    }

    void TransformComponent::setOffset(bool state, const mat4<F32>& offset) {
        _transformOffset.first = state;
        _transformOffset.second.set(offset);
        setTransformDirty(TransformType::ALL);
    }

    void TransformComponent::setTransformDirty(TransformType type) {
        setTransformDirty(to_U32(type));
    }

    void TransformComponent::setTransformDirty(U32 typeMask) {
        SetBit(_transformUpdatedMask, typeMask);
    }

    void TransformComponent::setPosition(const vec3<F32>& position) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.setPosition(position);
        }

        setTransformDirty(TransformType::TRANSLATION);
    }

    void TransformComponent::setScale(const vec3<F32>& scale) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.setScale(scale);
            _uniformScaled = scale.isUniform();
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::setRotation(const Quaternion<F32>& quat) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.setRotation(quat);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setRotation(const vec3<F32>& axis, Angle::DEGREES<F32> degrees) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.setRotation(axis, degrees);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setRotation(Angle::DEGREES<F32> pitch, Angle::DEGREES<F32> yaw, Angle::DEGREES<F32> roll) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.setRotation(pitch, yaw, roll);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::translate(const vec3<F32>& axisFactors) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.translate(axisFactors);
        }

        setTransformDirty(TransformType::TRANSLATION);
    }

    void TransformComponent::scale(const vec3<F32>& axisFactors) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.scale(axisFactors);
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::rotate(const vec3<F32>& axis, Angle::DEGREES<F32> degrees) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.rotate(axis, degrees);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::rotate(Angle::DEGREES<F32> pitch, Angle::DEGREES<F32> yaw, Angle::DEGREES<F32> roll) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.rotate(pitch, yaw, roll);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::rotate(const Quaternion<F32>& quat) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.rotate(quat);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::rotateSlerp(const Quaternion<F32>& quat, const D64 deltaTime) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.rotateSlerp(quat, deltaTime);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setScaleX(const F32 ammount) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.setScaleX(ammount);
            _uniformScaled = _transformInterface.isUniformScale();
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::setScaleY(const F32 ammount) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.setScaleY(ammount);
            _uniformScaled = _transformInterface.isUniformScale();
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::setScaleZ(const F32 ammount) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.setScaleZ(ammount);
            _uniformScaled = _transformInterface.isUniformScale();
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::scaleX(const F32 scale) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.scaleX(scale);
            _uniformScaled = _transformInterface.isUniformScale();
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::scaleY(const F32 scale) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.scaleY(scale);
            _uniformScaled = _transformInterface.isUniformScale();
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::scaleZ(const F32 scale) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.scaleZ(scale);
            _uniformScaled = _transformInterface.isUniformScale();
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::rotateX(const Angle::DEGREES<F32> angle) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.rotateX(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::rotateY(const Angle::DEGREES<F32> angle) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.rotateY(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::rotateZ(const Angle::DEGREES<F32> angle) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.rotateZ(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setRotationX(const Angle::DEGREES<F32> angle) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.setRotationX(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setRotationY(const Angle::DEGREES<F32> angle) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.setRotationY(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setRotationZ(const Angle::DEGREES<F32> angle) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.setRotationZ(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setPositionX(const F32 positionX) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.setPositionX(positionX);
        }

        setTransformDirty(TransformType::TRANSLATION);
    }

    void TransformComponent::setPositionY(const F32 positionY) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.setPositionY(positionY);
        }

        setTransformDirty(TransformType::TRANSLATION);
    }

    void TransformComponent::setPositionZ(const F32 positionZ) {
        {
            UniqueLockShared w_lock(_lock);
            _transformInterface.setPositionZ(positionZ);
        }

        setTransformDirty(TransformType::TRANSLATION);
    }

    void TransformComponent::pushTransforms() {
        TransformValues temp;
        {
            SharedLock r_lock(_lock);
            _transformInterface.getValues(temp);
        }

        _transformStack.push(temp);
    }

    bool TransformComponent::popTransforms() {
        if (!_transformStack.empty()) {
            _prevTransformValues = _transformStack.top();
            {
                UniqueLockShared w_lock(_lock);
                _transformInterface.setValues(_prevTransformValues);
            }

            _transformStack.pop();

            setTransformDirty(TransformType::ALL);
            return true;
        }
        return false;
    }
    
    void TransformComponent::setTransform(const TransformValues& values) {
        {
            UniqueLockShared r_lock(_lock);
            _transformInterface.setValues(values);
        }
        setTransformDirty(TransformType::ALL);
    }

    void TransformComponent::setTransforms(const mat4<F32>& transform) {
        {
            UniqueLockShared r_lock(_lock);
            _transformInterface.setTransforms(transform);
        }
        setTransformDirty(TransformType::ALL);
    }

    void TransformComponent::getValues(TransformValues& valuesOut) const {
        SharedLock r_lock(_lock);
        _transformInterface.getValues(valuesOut);
    }

    mat4<F32> TransformComponent::getMatrix() {
        SharedLock r_lock(_lock);
        
        if (_transformOffset.first) {
            return _transformInterface.getMatrix() * _transformOffset.second;
        } 
        return _transformInterface.getMatrix();
    }

    mat4<F32> TransformComponent::getMatrix(D64 interpolationFactor) const {
        SharedLock r_lock(_lock);
        mat4<F32> worldMatrixInterp(getLocalPositionLocked(interpolationFactor),
                                    getLocalScaleLocked(interpolationFactor),
                                    GetMatrix(getLocalOrientationLocked(interpolationFactor)));

        if (_transformOffset.first) {
            return worldMatrixInterp * _transformOffset.second;
        }

        return worldMatrixInterp;
    }

    vec3<F32> TransformComponent::getLocalPositionLocked(D64 interpolationFactor) const {
        vec3<F32> pos;
        _transformInterface.getPosition(pos);
        return Lerp(_prevTransformValues._translation, pos, to_F32(interpolationFactor));
    }

    vec3<F32> TransformComponent::getLocalScaleLocked(D64 interpolationFactor) const {
        vec3<F32> scale;
        _transformInterface.getScale(scale);
        return Lerp(_prevTransformValues._scale, scale, to_F32(interpolationFactor));
    }

    Quaternion<F32> TransformComponent::getLocalOrientationLocked(D64 interpolationFactor) const {
        Quaternion<F32> quat;
        _transformInterface.getOrientation(quat);
        return Slerp(_prevTransformValues._orientation, quat, to_F32(interpolationFactor));
    }

    const mat4<F32>& TransformComponent::updateWorldMatrix() {
        if (!_worldMatrixUpToDate.test_and_set()) {
            _worldMatrix.set(getMatrix());
            
            SceneGraphNode* grandParentPtr = _parentSGN.getParent();
            if (grandParentPtr) {
                _worldMatrix *= grandParentPtr->get<TransformComponent>()->updateWorldMatrix();
            }

            _parentSGN.SendEvent<TransformUpdated>(GetOwner());
        }

        return _worldMatrix;
    }

    const mat4<F32>& TransformComponent::getWorldMatrix() const {
        return _worldMatrix;
    }

    mat4<F32> TransformComponent::getWorldMatrix(D64 interpolationFactor) const {
        if (_parentUsageContext != NodeUsageContext::NODE_STATIC && interpolationFactor < 0.99) {
            mat4<F32> ret(getMatrix(interpolationFactor));

            SceneGraphNode* grandParentPtr = _parentSGN.getParent();
            if (grandParentPtr) {
                ret *= grandParentPtr->get<TransformComponent>()->getWorldMatrix(interpolationFactor);
            }

            return ret;
        }

        return getWorldMatrix();
    }

    /// Return the position
    vec3<F32> TransformComponent::getPosition() const {
        SceneGraphNode* grandParent = _parentSGN.getParent();
        if (grandParent) {
            return getLocalPosition() + grandParent->get<TransformComponent>()->getPosition();
        }

        return getLocalPosition();
    }

    /// Return the position
    vec3<F32> TransformComponent::getPosition(D64 interpolationFactor) const {
        SceneGraphNode* grandParent = _parentSGN.getParent();
        if (grandParent) {
            return getLocalPosition(interpolationFactor) + grandParent->get<TransformComponent>()->getPosition(interpolationFactor);
        }

        return getLocalPosition(interpolationFactor);
    }

    vec3<F32> TransformComponent::getScale() const {
        SceneGraphNode* grandParent = _parentSGN.getParent();
        if (grandParent) {
            return getLocalScale() * grandParent->get<TransformComponent>()->getScale();
        }

        return getLocalScale();
    }

    /// Return the scale factor
    vec3<F32> TransformComponent::getScale(D64 interpolationFactor) const {
        SceneGraphNode* grandParent = _parentSGN.getParent();
        if (grandParent) {
            return getLocalScale(interpolationFactor) * grandParent->get<TransformComponent>()->getScale(interpolationFactor);
        }

        return getLocalScale(interpolationFactor);
    }

    /// Return the orientation quaternion
    Quaternion<F32> TransformComponent::getOrientation() const {
        SceneGraphNode* grandParent = _parentSGN.getParent();
        if (grandParent) {
            return grandParent->get<TransformComponent>()->getOrientation() * getLocalOrientation();
        }

        return getLocalOrientation();
    }

    Quaternion<F32> TransformComponent::getOrientation(D64 interpolationFactor) const {
        SceneGraphNode* grandParent = _parentSGN.getParent();
        if (grandParent) {
            return grandParent->get<TransformComponent>()->getOrientation(interpolationFactor) * getLocalOrientation(interpolationFactor);
        }

        return getLocalOrientation(interpolationFactor);
    }

    vec3<F32> TransformComponent::getLocalPosition() const {
        vec3<F32> pos;
        getPosition(pos);
        return pos;
    }

    vec3<F32> TransformComponent::getLocalScale() const {
        vec3<F32> scale;
        getScale(scale);
        return scale;
    }

    Quaternion<F32> TransformComponent::getLocalOrientation() const {
        Quaternion<F32> quat;
        getOrientation(quat);
        return quat;
    }

    vec3<F32> TransformComponent::getLocalPosition(D64 interpolationFactor) const {
        return Lerp(_prevTransformValues._translation, getLocalPosition(), to_F32(interpolationFactor));
    }

    vec3<F32> TransformComponent::getLocalScale(D64 interpolationFactor) const {
        return Lerp(_prevTransformValues._scale, getLocalScale(), to_F32(interpolationFactor));
    }

    vec3<F32> TransformComponent::getFwdVector() const {
        return Rotate(WORLD_Z_NEG_AXIS, getOrientation());
    }

    vec3<F32> TransformComponent::getUpVector() const {
        return Rotate(WORLD_Y_AXIS, getOrientation());
    }

    vec3<F32> TransformComponent::getRightVector() const {
        return Rotate(WORLD_X_AXIS, getOrientation());
    }

    Quaternion<F32> TransformComponent::getLocalOrientation(D64 interpolationFactor) const {
        return Slerp(_prevTransformValues._orientation, getLocalOrientation(), to_F32(interpolationFactor));
    }

    // Transform interface access
    void TransformComponent::getScale(vec3<F32>& scaleOut) const {
        SharedLock r_lock(_lock);

        _transformInterface.getScale(scaleOut);
    }

    void TransformComponent::getPosition(vec3<F32>& posOut) const {
        SharedLock r_lock(_lock);

        _transformInterface.getPosition(posOut);
    }

    void TransformComponent::getOrientation(Quaternion<F32>& quatOut) const {
        SharedLock r_lock(_lock);

        _transformInterface.getOrientation(quatOut);
    }

    bool TransformComponent::isUniformScaled() const {
        return _uniformScaled;
    }

    bool TransformComponent::saveCache(ByteBuffer& outputBuffer) const {
        outputBuffer << uniqueID();
        outputBuffer << _hasChanged;

        if (_hasChanged) {

            vec3<F32> localPos;
            getPosition(localPos);
            outputBuffer << localPos;

            vec3<F32> localScale;
            getScale(localScale);
            outputBuffer << localScale;

            Quaternion<F32> localRotation;
            getOrientation(localRotation);
            outputBuffer << localRotation.asVec4();
            _hasChanged = false;
        }

        return BaseComponentType<TransformComponent, ComponentType::TRANSFORM>::saveCache(outputBuffer);
    }

    bool TransformComponent::loadCache(ByteBuffer& inputBuffer) {
        I64 tempID = -1;
        inputBuffer >> tempID;
        if (tempID != uniqueID()) {
            // corrupt save
            return false;
        }

        bool hasChanged = false;
        inputBuffer >> hasChanged;

        if (hasChanged) {
            inputBuffer >> tempID;

            vec3<F32> localPos;
            inputBuffer >> localPos;
            setPosition(localPos);

            vec3<F32> localScale;
            inputBuffer >> localScale;
            setScale(localScale);

            vec4<F32> localRotation;
            inputBuffer >> localRotation;
            setRotation(Quaternion<F32>(localRotation));
        }

        return BaseComponentType<TransformComponent, ComponentType::TRANSFORM>::saveCache(inputBuffer);
    }
}; //namespace