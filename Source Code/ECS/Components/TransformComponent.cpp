#include "stdafx.h"

#include "Headers/TransformComponent.h"

#include "Core/Headers/ByteBuffer.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {
    TransformComponent::TransformComponent(SceneGraphNode& parentSGN, PlatformContext& context)
      : BaseComponentType<TransformComponent, ComponentType::TRANSFORM>(parentSGN, context),
        _uniformScaled(false),
        _parentUsageContext(parentSGN.usageContext())
    {
        _transformUpdatedMask.store(to_base(TransformType::ALL) | to_base(TransformType::PREVIOUS_MAT));

        EditorComponentField transformField = {};
        transformField._name = "Transform";
        transformField._data = this;
        transformField._type = EditorComponentFieldType::TRANSFORM;
        transformField._readOnly = false;

        _editorComponent.registerField(std::move(transformField));


        EditorComponentField worldMatField = {};
        worldMatField._name = "WorldMat";
        worldMatField._dataGetter = [this](void* dataOut) { getWorldMatrix(*static_cast<mat4<F32>*>(dataOut)); };
        worldMatField._type = EditorComponentFieldType::PUSH_TYPE;
        worldMatField._readOnly = true;
        worldMatField._serialise = false;
        worldMatField._basicType = GFX::PushConstantType::MAT4;

        _editorComponent.registerField(std::move(worldMatField));

        if (_transformOffset.first) {
            EditorComponentField transformOffsetField = {};
            transformOffsetField._name = "Transform Offset";
            transformOffsetField._data = _transformOffset.second;
            transformOffsetField._type = EditorComponentFieldType::PUSH_TYPE;
            transformOffsetField._readOnly = true;
            transformOffsetField._basicType = GFX::PushConstantType::MAT4;

            _editorComponent.registerField(std::move(transformOffsetField));
        }

        EditorComponentField recomputeMatrixField = {};
        recomputeMatrixField._name = "Recompute WorldMatrix";
        recomputeMatrixField._range = { recomputeMatrixField._name.length() * 10.0f, 20.0f };//dimensions
        recomputeMatrixField._type = EditorComponentFieldType::BUTTON;
        recomputeMatrixField._readOnly = false; //disabled/enabled
        _editorComponent.registerField(std::move(recomputeMatrixField));

        _editorComponent.onChangedCbk([this](std::string_view field) {
            if (field == "Transform") {
                setTransformDirty(to_base(TransformType::ALL));
            } else if (field == "Position Offset") {
                // view offset stuff
            } else if (field == "Recompute WorldMatrix") {
                _transformUpdatedMask.store(to_base(TransformType::ALL));
            }

            _hasChanged = true;
        });
    }

    TransformComponent::~TransformComponent()
    {
        
    }

    void TransformComponent::onParentTransformDirty(U32 transformMask) noexcept {
        if (transformMask != to_base(TransformType::NONE)) {
            setTransformDirty(transformMask);
        }
    }

    void TransformComponent::onParentUsageChanged(NodeUsageContext context) noexcept {
        _parentUsageContext = context;
    }

    void TransformComponent::reset() {
        _worldMatrix.identity();
        _prevWorldMatrix.identity();
        _prevTransformValues._translation.set(0.0f);

        _prevTransformValues._scale.set(1.0f);
        _prevTransformValues._orientation.identity();
        while (!_transformStack.empty()) {
            _transformStack.pop();
        }

        _transformUpdatedMask.store(to_base(TransformType::ALL) | to_base(TransformType::PREVIOUS_MAT));
    }

    void TransformComponent::PreUpdate(const U64 deltaTimeUS) {
        OPTICK_EVENT();

        // If we have dirty transforms, inform everybody
        const U32 updateMask = _transformUpdatedMask.load();
        if (updateMask != to_base(TransformType::NONE))
        {
            Attorney::SceneGraphNodeComponent::setTransformDirty(_parentSGN, updateMask);
            SharedLock<SharedMutex> r_lock(_lock);
            _prevTransformValues = _transformInterface.getValues();
        }

        BaseComponentType<TransformComponent, ComponentType::TRANSFORM>::PreUpdate(deltaTimeUS);
    }

    void TransformComponent::Update(const U64 deltaTimeUS) {
        // Cleanup our dirty transforms
        const U32 previousMask = _transformUpdatedMask.exchange(to_U32(TransformType::NONE));
        if (previousMask != to_U32(TransformType::NONE)) {
            updateWorldMatrix(previousMask);

            _parentSGN.SendEvent(
            {
                ECS::CustomEvent::Type::TransformUpdated,
                this,
                previousMask
            });
        }

        BaseComponentType<TransformComponent, ComponentType::TRANSFORM>::Update(deltaTimeUS);
    }

    void TransformComponent::setOffset(bool state, const mat4<F32>& offset) {
        _transformOffset.first = state;
        _transformOffset.second.set(offset);
        setTransformDirty(TransformType::ALL);
    }

    void TransformComponent::setTransformDirty(TransformType type) noexcept {
        setTransformDirty(to_U32(type));
    }

    void TransformComponent::setTransformDirty(U32 typeMask) noexcept {
        SetBit(_transformUpdatedMask, typeMask);
    }

    void TransformComponent::setPosition(const vec3<F32>& position) {
        setPosition(position.x, position.y, position.z);
    }

    void TransformComponent::setPosition(const F32 x, const F32 y, const F32 z) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.setPosition(x, y, z);
        }

        setTransformDirty(TransformType::TRANSLATION);
    }

    void TransformComponent::setScale(const vec3<F32>& scale) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.setScale(scale);
            _uniformScaled = scale.isUniform();
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::setRotation(const Quaternion<F32>& quat) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.setRotation(quat);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setRotation(const vec3<F32>& axis, Angle::DEGREES<F32> degrees) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.setRotation(axis, degrees);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setRotation(Angle::DEGREES<F32> pitch, Angle::DEGREES<F32> yaw, Angle::DEGREES<F32> roll) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.setRotation(pitch, yaw, roll);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::translate(const vec3<F32>& axisFactors) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.translate(axisFactors);
        }

        setTransformDirty(TransformType::TRANSLATION);
    }

    void TransformComponent::scale(const vec3<F32>& axisFactors) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.scale(axisFactors);
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::rotate(const vec3<F32>& axis, Angle::DEGREES<F32> degrees) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.rotate(axis, degrees);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::rotate(Angle::DEGREES<F32> pitch, Angle::DEGREES<F32> yaw, Angle::DEGREES<F32> roll) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.rotate(pitch, yaw, roll);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::rotate(const Quaternion<F32>& quat) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.rotate(quat);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::rotateSlerp(const Quaternion<F32>& quat, const D64 deltaTime) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.rotateSlerp(quat, deltaTime);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setScaleX(const F32 ammount) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.setScaleX(ammount);
            _uniformScaled = _transformInterface.isUniformScale();
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::setScaleY(const F32 ammount) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.setScaleY(ammount);
            _uniformScaled = _transformInterface.isUniformScale();
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::setScaleZ(const F32 ammount) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.setScaleZ(ammount);
            _uniformScaled = _transformInterface.isUniformScale();
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::scaleX(const F32 scale) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.scaleX(scale);
            _uniformScaled = _transformInterface.isUniformScale();
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::scaleY(const F32 scale) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.scaleY(scale);
            _uniformScaled = _transformInterface.isUniformScale();
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::scaleZ(const F32 scale) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.scaleZ(scale);
            _uniformScaled = _transformInterface.isUniformScale();
        }

        setTransformDirty(TransformType::SCALE);
    }

    void TransformComponent::rotateX(const Angle::DEGREES<F32> angle) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.rotateX(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::rotateY(const Angle::DEGREES<F32> angle) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.rotateY(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::rotateZ(const Angle::DEGREES<F32> angle) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.rotateZ(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setRotationX(const Angle::DEGREES<F32> angle) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.setRotationX(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setRotationY(const Angle::DEGREES<F32> angle) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.setRotationY(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setRotationZ(const Angle::DEGREES<F32> angle) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.setRotationZ(angle);
        }

        setTransformDirty(TransformType::ROTATION);
    }

    void TransformComponent::setPositionX(const F32 positionX) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.setPositionX(positionX);
        }

        setTransformDirty(TransformType::TRANSLATION);
    }

    void TransformComponent::setPositionY(const F32 positionY) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.setPositionY(positionY);
        }

        setTransformDirty(TransformType::TRANSLATION);
    }

    void TransformComponent::setPositionZ(const F32 positionZ) {
        {
            UniqueLock<SharedMutex> w_lock(_lock);
            _transformInterface.setPositionZ(positionZ);
        }

        setTransformDirty(TransformType::TRANSLATION);
    }

    void TransformComponent::pushTransforms() {
        SharedLock<SharedMutex> r_lock(_lock);
        _transformStack.push(_transformInterface.getValues());
    }

    bool TransformComponent::popTransforms() {
        if (!_transformStack.empty()) {
            _prevTransformValues = _transformStack.top();
            {
                UniqueLock<SharedMutex> w_lock(_lock);
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
            UniqueLock<SharedMutex> r_lock(_lock);
            _transformInterface.setValues(values);
        }
        setTransformDirty(TransformType::ALL);
    }

    void TransformComponent::setTransforms(const mat4<F32>& transform) {
        {
            UniqueLock<SharedMutex> r_lock(_lock);
            _transformInterface.setTransforms(transform);
        }
        setTransformDirty(TransformType::ALL);
    }

    TransformValues TransformComponent::getValues() const {
        SharedLock<SharedMutex> r_lock(_lock);
        return _transformInterface.getValues();
    }

    void TransformComponent::getMatrix(mat4<F32>& matOut) {
        SharedLock<SharedMutex> r_lock(_lock);
        _transformInterface.getMatrix(matOut);

        if (_transformOffset.first) {
            matOut *= _transformOffset.second;
        }
    }

    void TransformComponent::getMatrix(D64 interpolationFactor, mat4<F32>& matOut) const {
        {
            SharedLock<SharedMutex> r_lock(_lock);
            matOut.set(mat4<F32>
                       {
                            getLocalPositionLocked(interpolationFactor),
                            getLocalScaleLocked(interpolationFactor),
                            GetMatrix(getLocalOrientationLocked(interpolationFactor))
                        });
        }
        if (_transformOffset.first) {
            matOut *= _transformOffset.second;
        }
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

    void TransformComponent::updateWorldMatrix(U32 updateMask) {
        const bool initPrevMatrix = BitCompare(updateMask, to_U32(TransformType::PREVIOUS_MAT));
        UniqueLock<SharedMutex> w_lock(_worldMatrixLock);
        if (!initPrevMatrix) {
            _prevWorldMatrix = _worldMatrix;
        }
        getMatrix(_worldMatrix);

        if (initPrevMatrix) {
            _prevWorldMatrix = _worldMatrix;
        }
    }

    void TransformComponent::getPreviousWorldMatrix(mat4<F32>& matOut) const {
        mat4<F32> parentMat = MAT4_IDENTITY;
        SceneGraphNode* grandParentPtr = _parentSGN.parent();
        if (grandParentPtr) {
            grandParentPtr->get<TransformComponent>()->getPreviousWorldMatrix(parentMat);
        }

        matOut.set(_prevWorldMatrix);
        matOut *= parentMat;
    }

    void TransformComponent::getWorldMatrix(mat4<F32>& matOut) const {
        mat4<F32> parentMat = MAT4_IDENTITY;
        SceneGraphNode* grandParentPtr = _parentSGN.parent();
        if (grandParentPtr) {
            grandParentPtr->get<TransformComponent>()->getWorldMatrix(parentMat);
        }

        {
            SharedLock<SharedMutex> r_lock(_worldMatrixLock);
            matOut.set(_worldMatrix);
        }

        matOut *= parentMat;
    }

    mat4<F32> TransformComponent::getWorldMatrix(D64 interpolationFactor) const {
        mat4<F32> ret = {};
        getWorldMatrix(interpolationFactor, ret);
        return ret;
    }

    void TransformComponent::getWorldMatrix(D64 interpolationFactor, mat4<F32>& matrixOut) const {
        OPTICK_EVENT();

        if (_parentUsageContext == NodeUsageContext::NODE_STATIC || interpolationFactor > 0.985) {
            getWorldMatrix(matrixOut);
        } else {
            getMatrix(interpolationFactor, matrixOut);

            SceneGraphNode* grandParentPtr = _parentSGN.parent();
            if (grandParentPtr) {
                matrixOut *= grandParentPtr->get<TransformComponent>()->getWorldMatrix(interpolationFactor);
            }
        }
    }

    /// Return the position
    vec3<F32> TransformComponent::getPosition() const {
        SceneGraphNode* grandParent = _parentSGN.parent();
        if (grandParent) {
            return getLocalPosition() + grandParent->get<TransformComponent>()->getPosition();
        }

        return getLocalPosition();
    }

    /// Return the position
    vec3<F32> TransformComponent::getPosition(D64 interpolationFactor) const {
        SceneGraphNode* grandParent = _parentSGN.parent();
        if (grandParent) {
            return getLocalPosition(interpolationFactor) + grandParent->get<TransformComponent>()->getPosition(interpolationFactor);
        }

        return getLocalPosition(interpolationFactor);
    }

    vec3<F32> TransformComponent::getScale() const {
        SceneGraphNode* grandParent = _parentSGN.parent();
        if (grandParent) {
            return getLocalScale() * grandParent->get<TransformComponent>()->getScale();
        }

        return getLocalScale();
    }

    /// Return the scale factor
    vec3<F32> TransformComponent::getScale(D64 interpolationFactor) const {
        SceneGraphNode* grandParent = _parentSGN.parent();
        if (grandParent) {
            return getLocalScale(interpolationFactor) * grandParent->get<TransformComponent>()->getScale(interpolationFactor);
        }

        return getLocalScale(interpolationFactor);
    }

    /// Return the orientation quaternion
    Quaternion<F32> TransformComponent::getOrientation() const {
        SceneGraphNode* grandParent = _parentSGN.parent();
        if (grandParent) {
            return grandParent->get<TransformComponent>()->getOrientation() * getLocalOrientation();
        }

        return getLocalOrientation();
    }

    Quaternion<F32> TransformComponent::getOrientation(D64 interpolationFactor) const {
        SceneGraphNode* grandParent = _parentSGN.parent();
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
        SharedLock<SharedMutex> r_lock(_lock);

        _transformInterface.getScale(scaleOut);
    }

    void TransformComponent::getPosition(vec3<F32>& posOut) const {
        SharedLock<SharedMutex> r_lock(_lock);

        _transformInterface.getPosition(posOut);
    }

    void TransformComponent::getOrientation(Quaternion<F32>& quatOut) const {
        SharedLock<SharedMutex> r_lock(_lock);

        _transformInterface.getOrientation(quatOut);
    }

    bool TransformComponent::isUniformScaled() const {
        return _uniformScaled;
    }

    bool TransformComponent::saveCache(ByteBuffer& outputBuffer) const {
        outputBuffer << _hasChanged.load();

        if (_hasChanged.exchange(false)) {
            SharedLock<SharedMutex> r_lock(_lock);
            TransformValues values = _transformInterface.getValues();
            
            outputBuffer << values._translation;
            outputBuffer << values._scale;
            outputBuffer << values._orientation;
        }

        return BaseComponentType<TransformComponent, ComponentType::TRANSFORM>::saveCache(outputBuffer);
    }

    bool TransformComponent::loadCache(ByteBuffer& inputBuffer) {
        bool hasChanged = false;
        inputBuffer >> hasChanged;

        if (hasChanged) {
            TransformValues valuesIn = {};
            inputBuffer >> valuesIn._translation;
            inputBuffer >> valuesIn._scale;
            inputBuffer >> valuesIn._orientation;

            setTransform(valuesIn);
        }

        return BaseComponentType<TransformComponent, ComponentType::TRANSFORM>::loadCache(inputBuffer);
    }
}; //namespace