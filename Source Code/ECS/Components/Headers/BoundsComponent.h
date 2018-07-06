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

#ifndef _BOUNDS_COMPONENT_H_
#define _BOUNDS_COMPONENT_H_

#include "SGNComponent.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingSphere.h"

namespace Divide {
    struct TransformDirty;
    class BoundsComponent : public SGNComponent<BoundsComponent>{
    public:
        BoundsComponent(SceneGraphNode& sgn);
        ~BoundsComponent();

        inline const BoundingBox& getBoundingBox() const { return _boundingBox; }
        inline const BoundingSphere& getBoundingSphere() const { return _boundingSphere; }
        inline bool lockBBTransforms() const { return _lockBBTransforms; }
        inline void lockBBTransforms(const bool state) { _lockBBTransforms = state; }


        void onBoundsChange(const BoundingBox& nodeBounds);

    protected:
        friend class SceneGraph;
        friend class BoundsSystem;
                
        void update(const U64 deltaTimeUS);

        inline void flagBoundingBoxDirty() { _boundingBoxDirty = true; }

        void OnTransformDirty(const TransformDirty* event);

    private:
        std::atomic<bool> _transformDirty;
        std::atomic<bool> _boundingBoxDirty;
        bool _lockBBTransforms;
        mat4<F32> _worldMatrix;
        BoundingBox _boundingBox;
        BoundingBox _refBoundingBox;
        BoundingSphere _boundingSphere;
    };
};

#endif //_BOUNDS_COMPONENT_H_