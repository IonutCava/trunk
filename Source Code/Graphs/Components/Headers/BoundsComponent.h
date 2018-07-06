/*
Copyright (c) 2016 DIVIDE-Studio
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
    class BoundsComponent : public SGNComponent {
    public:
        inline BoundingBox& getBoundingBox() { return _boundingBox; }
        inline const BoundingBox& getBoundingBoxConst() const { return _boundingBox; }
        inline BoundingSphere& getBoundingSphere() { return _boundingSphere; }
        inline const BoundingSphere& getBoundingSphereConst() const { return _boundingSphere; }
        inline bool lockBBTransforms() const { return _lockBBTransforms; }
        inline void lockBBTransforms(const bool state) { _lockBBTransforms = state; }

    protected:
        friend class SceneGraphNode;
        BoundsComponent(SceneGraphNode& sgn);
        ~BoundsComponent();
        // ToDo: THIS IS AN UNGLY HACK. PLEASE REMOVE AND USE THE PROPER UPDATE SYSTEM! - Ionut
        void parseBounds(const BoundingBox& nodeBounds, const bool force, const mat4<F32>& worldTransform);

        inline void flagBoundingBoxDirty() {
            _boundingBoxDirty = true;
        }

    private:
        std::atomic<bool> _boundingBoxDirty;
        bool _lockBBTransforms;
        BoundingBox _boundingBox;
        BoundingSphere _boundingSphere;
    };
};

#endif //_BOUNDS_COMPONENT_H_