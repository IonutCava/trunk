#include "stdafx.h"

#include "Headers/TransformComponent.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "ECS/Events/Headers/TransformEvents.h"

namespace Divide {
    TransformComponent::TransformComponent(SceneGraphNode& parentSGN)
      : SGNComponent(parentSGN, "TRANSFORM"),
        _prevInterpValue(0.0),
        _transformUpdatedMask(to_base(TransformType::WORLD_TRANSFORMS))
    {
        _editorComponent.registerField("Transform",
                                       &_transformInterface,
                                       EditorComponentFieldType::TRANSFORM,
                                       false);
        _editorComponent.registerField("WorldMat", &_worldMatrix, EditorComponentFieldType::PUSH_TYPE, true, GFX::PushConstantType::MAT4);
        
        if (_ignoreViewSettings._cameraGUID != -1) {
            _editorComponent.addHeader("Ignore View Settings");

            _editorComponent.registerField("Camera GUID", &_ignoreViewSettings._cameraGUID, EditorComponentFieldType::PUSH_TYPE, true, GFX::PushConstantType::MAT4);
            _editorComponent.registerField("Position Offset", &_ignoreViewSettings._posOffset, EditorComponentFieldType::PUSH_TYPE, false, GFX::PushConstantType::VEC3);
            _editorComponent.registerField("Transform Offset", &_ignoreViewSettings._transformOffset, EditorComponentFieldType::PUSH_TYPE, true, GFX::PushConstantType::MAT4);
        }

        _editorComponent.onChangedCbk([this](EditorComponentField& field) {
            if (field._name == "Transform") {
                setTransformDirty(to_base(TransformType::WORLD_TRANSFORMS));
            } else if (field._name == "Position Offset") {
                setTransformDirty(TransformType::VIEW_OFFSET);
            }

            _hasChanged = true;
        });
    }

    TransformComponent::~TransformComponent()
    {
        
    }

    void TransformComponent::onParentTransformDirty(U32 transformMask) {
        if (transformMask != to_base(TransformType::NONE)) {
            _worldMatrixDirty = true;
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
        
        _worldMatrixDirty = true;
        _prevInterpValue = 0.0;

        WriteLock r_lock(_lock);
        SetBit(_transformUpdatedMask, TransformType::ALL);
    }

    void TransformComponent::PreUpdate(const U64 deltaTimeUS) {
        // If we have dirty transforms, inform everybody
        UpgradableReadLock r_lock(_lock);
        if (_transformUpdatedMask != to_base(TransformType::NONE))
        {
            Attorney::SceneGraphNodeComponent::setTransformDirty(_parentSGN, _transformUpdatedMask);
        }

        SGNComponent<TransformComponent>::PreUpdate(deltaTimeUS);
    }

    void TransformComponent::Update(const U64 deltaTimeUS) {
        // Cleanup our dirty transforms
        UpgradableReadLock r_lock(_lock);
        if (_transformUpdatedMask != to_base(TransformType::NONE))
        {
            UpgradeToWriteLock w_lock(r_lock);
            if (AnyCompare(_transformUpdatedMask, to_base(TransformType::WORLD_TRANSFORMS))) {
                _transformInterface.getValues(_prevTransformValues);
            }
            _worldMatrixDirty = true;
            _transformUpdatedMask = to_base(TransformType::NONE);
        }

        SGNComponent<TransformComponent>::Update(deltaTimeUS);
    }

    void TransformComponent::PostUpdate(const U64 deltaTimeUS) {
        updateWorldMatrix();
        _prevInterpValue = -1.0;
        SGNComponent<TransformComponent>::PostUpdate(deltaTimeUS);
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
        setTransformDirty(to_U32(type));
    }

    void TransformComponent::setTransformDirty(U32 typeMask) {
        SetBit(_transformUpdatedMask, typeMask);
    }

    void TransformComponent::setPosition(const vec3<F32>& position) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.setPosition(position);
        }

        setTransformDirty(TransformType::TRANSLATION);
    }

    void TransformComponent::setScale(const vec3<F32>& scale) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.setScale(scale);
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::setRotation(const Quaternion<F32>& quat) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.setRotation(quat);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setRotation(const vec3<F32>& axis, Angle::DEGREES<F32> degrees) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.setRotation(axis, degrees);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setRotation(Angle::DEGREES<F32> pitch, Angle::DEGREES<F32> yaw, Angle::DEGREES<F32> roll) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.setRotation(pitch, yaw, roll);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::translate(const vec3<F32>& axisFactors) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.translate(axisFactors);
        }

        setTransformDirty(TransformType::TRANSLATION);
    }

    void TransformComponent::scale(const vec3<F32>& axisFactors) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.scale(axisFactors);
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::rotate(const vec3<F32>& axis, Angle::DEGREES<F32> degrees) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.rotate(axis, degrees);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::rotate(Angle::DEGREES<F32> pitch, Angle::DEGREES<F32> yaw, Angle::DEGREES<F32> roll) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.rotate(pitch, yaw, roll);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::rotate(const Quaternion<F32>& quat) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.rotate(quat);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::rotateSlerp(const Quaternion<F32>& quat, const D64 deltaTime) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.rotateSlerp(quat, deltaTime);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setScaleX(const F32 ammount) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.setScaleX(ammount);
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::setScaleY(const F32 ammount) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.setScaleY(ammount);
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::setScaleZ(const F32 ammount) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.setScaleZ(ammount);
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::scaleX(const F32 scale) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.scaleX(scale);
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::scaleY(const F32 scale) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.scaleY(scale);
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::scaleZ(const F32 scale) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.scaleZ(scale);
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::rotateX(const Angle::DEGREES<F32> angle) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.rotateX(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::rotateY(const Angle::DEGREES<F32> angle) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.rotateY(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::rotateZ(const Angle::DEGREES<F32> angle) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.rotateZ(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setRotationX(const Angle::DEGREES<F32> angle) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.setRotationX(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setRotationY(const Angle::DEGREES<F32> angle) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.setRotationY(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setRotationZ(const Angle::DEGREES<F32> angle) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.setRotationZ(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setPositionX(const F32 positionX) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.setPositionX(positionX);
        }

        setTransformDirty(TransformType::TRANSLATION);
    }

    void TransformComponent::setPositionY(const F32 positionY) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.setPositionY(positionY);
        }

        setTransformDirty(TransformType::TRANSLATION);
    }

    void TransformComponent::setPositionZ(const F32 positionZ) {
        {
            WriteLock w_lock(_lock);
            _transformInterface.setPositionZ(positionZ);
        }

        setTransformDirty(TransformType::TRANSLATION);
    }

    void TransformComponent::pushTransforms() {
        TransformValues temp;
        {
            ReadLock r_lock(_lock);
            _transformInterface.getValues(temp);
        }

        _transformStack.push(temp);
    }

    bool TransformComponent::popTransforms() {
        if (!_transformStack.empty()) {
            _prevTransformValues = _transformStack.top();
            {
                WriteLock w_lock(_lock);
                _transformInterface.setValues(_prevTransformValues);
            }

            _transformStack.pop();

            setTransformDirty(TransformType::WORLD_TRANSFORMS);
            return true;
        }
        return false;
    }
    
    void TransformComponent::setTransform(const TransformValues& values) {
        WriteLock r_lock(_lock);
        _transformInterface.setValues(values);
        setTransformDirty(TransformType::WORLD_TRANSFORMS);
    }

    void TransformComponent::setTransforms(const mat4<F32>& transform) {
        WriteLock r_lock(_lock);
        _transformInterface.setTransforms(transform);
        setTransformDirty(TransformType::WORLD_TRANSFORMS);
    }

    void TransformComponent::getValues(TransformValues& valuesOut) const {
        ReadLock r_lock(_lock);
        _transformInterface.getValues(valuesOut);
    }

    const mat4<F32>& TransformComponent::getMatrix() {
        ReadLock r_lock(_lock);
        return _transformInterface.getMatrix();
    }

    const mat4<F32>& TransformComponent::updateWorldMatrix() {
        if (_worldMatrixDirty) {
            _worldMatrix.set(getMatrix());

            SceneGraphNode* grandParentPtr = _parentSGN.getParent();
            if (grandParentPtr) {
                _worldMatrix *= grandParentPtr->get<TransformComponent>()->updateWorldMatrix();
            }

            if (_ignoreViewSettings._cameraGUID != -1) {
                _worldMatrix = _worldMatrix * _ignoreViewSettings._transformOffset;
            }

            _worldMatrixDirty = false;

            _parentSGN.SendEvent<TransformUpdated>(GetOwner());
        }

        return _worldMatrix;
    }

    const mat4<F32>& TransformComponent::getWorldMatrix() const {
        return _worldMatrix;
    }

    mat4<F32> TransformComponent::getWorldMatrix(D64 interpolationFactor) const {
        mat4<F32> worldMatrixInterp(getLocalPosition(interpolationFactor),
                                    getLocalScale(interpolationFactor),
                                    GetMatrix(getLocalOrientation(interpolationFactor)));

        SceneGraphNode* grandParentPtr = _parentSGN.getParent();
        if (grandParentPtr) {
            worldMatrixInterp *= grandParentPtr->get<TransformComponent>()->getWorldMatrix(interpolationFactor);
        }

        if (_ignoreViewSettings._cameraGUID != -1) {
            return worldMatrixInterp * _ignoreViewSettings._transformOffset;
        }

        return worldMatrixInterp;
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

    Quaternion<F32> TransformComponent::getLocalOrientation(D64 interpolationFactor) const {
        return Slerp(_prevTransformValues._orientation, getLocalOrientation(), to_F32(interpolationFactor));
    }

    // Transform interface access
    void TransformComponent::getScale(vec3<F32>& scaleOut) const {
        ReadLock r_lock(_lock);

        _transformInterface.getScale(scaleOut);
    }

    void TransformComponent::getPosition(vec3<F32>& posOut) const {
        ReadLock r_lock(_lock);

        _transformInterface.getPosition(posOut);
    }

    void TransformComponent::getOrientation(Quaternion<F32>& quatOut) const {
        ReadLock r_lock(_lock);

        _transformInterface.getOrientation(quatOut);
    }

    bool TransformComponent::isUniformScaled() const {
        return getLocalScale().isUniform();
    }

    bool TransformComponent::save(ByteBuffer& outputBuffer) const {
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

        return SGNComponent<TransformComponent>::save(outputBuffer);
    }

    bool TransformComponent::load(ByteBuffer& inputBuffer) {
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

        return SGNComponent<TransformComponent>::save(inputBuffer);
    }
}; //namespace