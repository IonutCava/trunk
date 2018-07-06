/*
Copyright (c) 2018 DIVIDE-Studio
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
    namespace Attorney {
        class TransformComponentSGN;
    };
    
    enum class TransformType : U32 {
        NONE = 0,
        TRANSLATION = toBit(1),
        SCALE = toBit(2),
        ROTATION = toBit(3),
        VIEW_OFFSET = toBit(4),
        WORLD_TRANSFORMS = TRANSLATION | SCALE | ROTATION,
        ALL = WORLD_TRANSFORMS | VIEW_OFFSET,
        COUNT = 4
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
        friend class Attorney::TransformComponentSGN;

        public:
         TransformComponent(SceneGraphNode& parentSGN);
         ~TransformComponent();

         void reset();

         const mat4<F32>& getWorldMatrix() const;
         mat4<F32>        getWorldMatrix(D64 interpolationFactor) const;

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

         void setTransform(const TransformValues& values);

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

         inline const mat4<F32>& getLocalMatrix() {
             return getMatrix();
         }

         void setTransforms(const mat4<F32>& transform);

         void getValues(TransformValues& valuesOut) const override;

         void pushTransforms();
         bool popTransforms();

         inline bool ignoreView(const I64 cameraGUID) const {
             return _ignoreViewSettings._cameraGUID == cameraGUID;
         }

         void setViewOffset(const vec3<F32>& posOffset, const mat4<F32>& rotationOffset);

         void ignoreView(const bool state, const I64 cameraGUID);

         bool save(ByteBuffer& outputBuffer) const override;
         bool load(ByteBuffer& inputBuffer) override;

      protected:
         friend class TransformSystem;
         void setTransformDirty(TransformType type);
         void setTransformDirty(U32 typeMask);

         void PreUpdate(const U64 deltaTimeUS) override;
         void Update(const U64 deltaTimeUS) override;
         void PostUpdate(const U64 deltaTimeUS) override;

         void onParentTransformDirty(U32 transformMask);

         const mat4<F32>& getMatrix() override;
         const mat4<F32>& updateWorldMatrix();

         // Local transform interface access (all are in local space)
         void getScale(vec3<F32>& scaleOut) const override;
         void getPosition(vec3<F32>& posOut) const override;
         void getOrientation(Quaternion<F32>& quatOut) const override;

      private:
        IgnoreViewSettings _ignoreViewSettings;

        typedef std::stack<TransformValues> TransformStack;

        U32             _transformUpdatedMask;
        TransformValues _prevTransformValues;
        TransformStack  _transformStack;
        Transform       _transformInterface;

        D64 _prevInterpValue;

        std::atomic_bool _worldMatrixDirty;
        mat4<F32> _worldMatrix;

        mutable SharedLock _lock;
    };


    namespace Attorney {
        class TransformComponentSGN {
            static void onParentTransformDirty(TransformComponent& comp, U32 transformMask) {
                comp.onParentTransformDirty(transformMask);
            }

            friend class Divide::SceneGraphNode;
        };

    }; //namespace Attorney

};

#endif //_TRANSFORM_COMPONENT_H_