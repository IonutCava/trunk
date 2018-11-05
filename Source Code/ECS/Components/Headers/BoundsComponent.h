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

#pragma once
#ifndef _BOUNDS_COMPONENT_H_
#define _BOUNDS_COMPONENT_H_

#include "SGNComponent.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingSphere.h"

namespace Divide {
    struct TransformUpdated;
    class BoundsComponent : public BaseComponentType<BoundsComponent, ComponentType::BOUNDS>{
    public:
        BoundsComponent(SceneGraphNode& sgn, PlatformContext& context);
        ~BoundsComponent();

        inline const BoundingBox& getBoundingBox() const { return _boundingBox; }
        inline const BoundingSphere& getBoundingSphere() const { return _boundingSphere; }

        inline bool ignoreTransform() const { return _ignoreTransform; }
        inline void ignoreTransform(bool state) { _ignoreTransform = state; }

        inline void boundsChanged() { _boundsChanged = true; }

    protected:
        friend class SceneGraph;
        friend class BoundsSystem;
                
        void Update(const U64 deltaTimeUS) override;

        // Flag the current BB as dirty and also flag all of the parents' bbs as dirty as well
        void flagBoundingBoxDirty();
        const BoundingBox& updateAndGetBoundingBox();
        void onTransformUpdated(const TransformUpdated* event);
        void onBoundsChange(const BoundingBox& nodeBounds);
        inline bool isBoundsChanged() const { return _boundsChanged; }

    private:
        std::atomic_bool _boundsChanged;
        std::atomic_bool _ignoreTransform;
        std::atomic_flag _boundingBoxNotDirty = ATOMIC_FLAG_INIT;
        BoundingBox _boundingBox;
        BoundingBox _refBoundingBox;
        BoundingSphere _boundingSphere;
    };
};

#endif //_BOUNDS_COMPONENT_H_