/*
Copyright (c) 2017 DIVIDE-Studio
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

#ifndef _TRANSFORM_COMPONENT_H_
#define _TRANSFORM_COMPONENT_H_

#include "SGNComponent.h"
#include "Core/Math/Headers/Transform.h"

namespace Divide {
    struct ParentTransformDirty;
    struct ParentTransformClean;

    enum class TransformType : U32 {
        TRANSLATION = 0,
        SCALE,
        ROTATION,
        VIEW_OFFSET,
        COUNT
    };


    class TransformMask {
        public:
        TransformMask()
        {
            clearAllFlags();
        }

        inline bool hasSetFlags() const {
            for (bool flag : _transformFlags) {
                if (flag) {
                    return true;
                }
            }
            return false;
        }

        inline bool hasSetAllFlags() const {
            for (bool flag : _transformFlags) {
                if (!flag) {
                    return false;
                }
            }

            return true;
        }

        inline bool getFlag(TransformType type) const {
            return _transformFlags[to_U32(type)] == true;
        }

        inline void clearAllFlags() {
            _transformFlags[to_base(TransformType::SCALE)] = false;
            _transformFlags[to_base(TransformType::ROTATION)] = false;
            _transformFlags[to_base(TransformType::TRANSLATION)] = false;
            _transformFlags[to_base(TransformType::VIEW_OFFSET)] = false;
        }

        inline void setAllFlags() {
            _transformFlags[to_base(TransformType::SCALE)] = true;
            _transformFlags[to_base(TransformType::ROTATION)] = true;
            _transformFlags[to_base(TransformType::TRANSLATION)] = true;
        }

        inline bool clearFlag(TransformType type) {
            _transformFlags[to_U32(type)] = false;
        }

        inline void setFlag(TransformType type) {
            _transformFlags[to_U32(type)] = true;
        }

        inline void setFlags(const TransformMask& other) {
            _transformFlags[to_base(TransformType::SCALE)] = other.getFlag(TransformType::SCALE);
            _transformFlags[to_base(TransformType::ROTATION)] = other.getFlag(TransformType::ROTATION);
            _transformFlags[to_base(TransformType::TRANSLATION)] = other.getFlag(TransformType::TRANSLATION);
            _transformFlags[to_base(TransformType::VIEW_OFFSET)] = other.getFlag(TransformType::VIEW_OFFSET);

        }
        private:
        std::array<std::atomic_bool, to_base(TransformType::COUNT)> _transformFlags;
    };

    struct IgnoreViewSettings {
        IgnoreViewSettings() : _cameraGUID(-1)
        {
        }

        I64 _cameraGUID;
        vec3<F32> _posOffset;
        mat4<F32> _transformOffset;
    };

    class TransformComponent : public SGNComponent<TransformComponent>,
                               public ITransform
    {
        public:
         TransformComponent(SceneGraphNode& parentSGN);
         ~TransformComponent();

         void reset();

         const mat4<F32>& getWorldMatrix();

         const mat4<F32>& getWorldMatrix(D64 interpolationFactor);

         /// Component <-> Transform interface
         void setPosition(const vec3<F32>& position) override;
         void setPositionX(const F32 positionX) override;
         void setPositionY(const F32 positionY) override;
         void setPositionZ(const F32 positionZ) override;
         void translate(const vec3<F32>& axisFactors) override;
         using ITransform::setPosition;

         void setScale(const vec3<F32>& ammount) override;
         void setScaleX(const F32 ammount) override;
         void setScaleY(const F32 ammount) override;
         void setScaleZ(const F32 ammount) override;
         void scale(const vec3<F32>& axisFactors) override;
         void scaleX(const F32 ammount) override;
         void scaleY(const F32 ammount) override;
         void scaleZ(const F32 ammount) override;
         using ITransform::setScale;

         void setRotation(const vec3<F32>& axis, Angle::DEGREES<F32> degrees) override;
         void setRotation(Angle::DEGREES<F32> pitch, Angle::DEGREES<F32> yaw, Angle::DEGREES<F32> roll) override;
         void setRotation(const Quaternion<F32>& quat) override;
         void setRotationX(const Angle::DEGREES<F32> angle) override;
         void setRotationY(const Angle::DEGREES<F32> angle) override;
         void setRotationZ(const Angle::DEGREES<F32> angle) override;
         using ITransform::setRotation;

         void rotate(const vec3<F32>& axis, Angle::DEGREES<F32> degrees) override;
         void rotate(Angle::DEGREES<F32> pitch, Angle::DEGREES<F32> yaw, Angle::DEGREES<F32> roll) override;
         void rotate(const Quaternion<F32>& quat) override;
         void rotateSlerp(const Quaternion<F32>& quat, const D64 deltaTime) override;
         void rotateX(const Angle::DEGREES<F32> angle) override;
         void rotateY(const Angle::DEGREES<F32> angle) override;
         void rotateZ(const Angle::DEGREES<F32> angle) override;
         using ITransform::rotate;

         inline void setTransform(const TransformValues& values) {
             setPosition(values._translation);
             setRotation(values._orientation);
             setScale(values._scale);
         }

         bool isUniformScaled() const;

         /// Return the position
         vec3<F32> getPosition() const;
         /// Return the local position
         vec3<F32> getLocalPosition() const;
         /// Return the position
         vec3<F32> getPosition(D64 interpolationFactor) const;
         /// Return the local position
         vec3<F32> getLocalPosition(D64 interpolationFactor) const;

         /// Return the scale factor
         vec3<F32> getScale() const;
         /// Return the local scale factor
         vec3<F32> getLocalScale() const;
         /// Return the scale factor
         vec3<F32> getScale(D64 interpolationFactor) const;
         /// Return the local scale factor
         vec3<F32> getLocalScale(D64 interpolationFactor) const;

         /// Return the orientation quaternion
         Quaternion<F32> getOrientation() const;
         /// Return the local orientation quaternion
         Quaternion<F32> getLocalOrientation() const;
         /// Return the orientation quaternion
         Quaternion<F32> getOrientation(D64 interpolationFactor) const;
         /// Return the local orientation quaternion
         Quaternion<F32> getLocalOrientation(D64 interpolationFactor) const;

         const mat4<F32>& getMatrix() override;
         void getValues(TransformValues& valuesOut) const override;

         void pushTransforms();
         bool popTransforms();

         inline bool ignoreView(const I64 cameraGUID) const {
             return _ignoreViewSettings._cameraGUID == cameraGUID;
         }

         void setViewOffset(const vec3<F32>& posOffset, const mat4<F32>& rotationOffset);

         void ignoreView(const bool state, const I64 cameraGUID);

         // Local transform interface access
         void getScale(vec3<F32>& scaleOut) const override;
         void getPosition(vec3<F32>& posOut) const override;
         void getOrientation(Quaternion<F32>& quatOut) const override;

      protected:
         friend class TransformSystem;
         void setTransformDirty(TransformType type);
         void snapshot();
         bool dirty() const;
         void clean(bool interp);

         void RegisterEventCallbacks();
         void OnParentTransformDirty(const ParentTransformDirty* event);
         void OnParentTransformClean(const ParentTransformClean* event);

      private:
        IgnoreViewSettings _ignoreViewSettings;

        typedef std::stack<TransformValues> TransformStack;

        TransformValues _prevTransformValues;
        TransformStack  _transformStack;
        TransformMask   _transformUpdatedMask;
        Transform       _transformInterface;
        /// Transform cache values
        std::atomic_bool _dirty;
        std::atomic_bool _dirtyInterp;
        std::atomic_bool _parentDirty;

        D64 _prevInterpValue;

        mat4<F32> _worldMatrix;
        mat4<F32> _worldMatrixInterp;

        mutable SharedLock _lock;
    };


  

};

#endif //_TRANSFORM_COMPONENT_H_