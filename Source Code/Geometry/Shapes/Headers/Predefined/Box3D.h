/*
   Copyright (c) 2015 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _BOX_3D_H_
#define _BOX_3D_H_

#include "Geometry/Shapes/Headers/Object3D.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

namespace Divide {

class Box3D : public Object3D {
   public:
    Box3D(const vec3<F32>& size) : Object3D(ObjectType::BOX_3D, ObjectFlag::OBJECT_FLAG_NONE) {
        vec3<F32> vertices[] = {
            vec3<F32>(-1.0, -1.0, 1.0),  vec3<F32>(1.0, -1.0, 1.0),
            vec3<F32>(-1.0, 1.0, 1.0),   vec3<F32>(1.0, 1.0, 1.0),
            vec3<F32>(-1.0, -1.0, -1.0), vec3<F32>(1.0, -1.0, -1.0),
            vec3<F32>(-1.0, 1.0, -1.0),  vec3<F32>(1.0, 1.0, -1.0)};

        vec3<F32> normals[] = {vec3<F32>(-1, -1, 1),  vec3<F32>(1, -1, 1),
                               vec3<F32>(-1, 1, 1),   vec3<F32>(1, 1, 1),
                               vec3<F32>(-1, -1, -1), vec3<F32>(1, -1, -1),
                               vec3<F32>(-1, 1, -1),  vec3<F32>(1, 1, -1)};

        U16 indices[] = {0, 1, 2, 3, 7, 1, 5, 4, 7, 6, 2, 4, 0, 1};

        _halfExtent.set(size / 2);

        getGeometryVB()->reservePositionCount(8);
        getGeometryVB()->reserveNormalCount(8);

        for (U8 i = 0; i < 8; i++) {
            getGeometryVB()->addPosition(vertices[i] * _halfExtent);
            getGeometryVB()->addNormal(normals[i]);
        }

        for (U8 i = 0; i < 14; i++) {
            getGeometryVB()->addIndex(indices[i]);
        }

        getGeometryVB()->Create();
    }

    inline void setHalfExtent(const vec3<F32> halfExtent) {
        vec3<F32> vertices[] = {
            vec3<F32>(-1.0, -1.0, 1.0),  vec3<F32>(1.0, -1.0, 1.0),
            vec3<F32>(-1.0, 1.0, 1.0),   vec3<F32>(1.0, 1.0, 1.0),
            vec3<F32>(-1.0, -1.0, -1.0), vec3<F32>(1.0, -1.0, -1.0),
            vec3<F32>(-1.0, 1.0, -1.0),  vec3<F32>(1.0, 1.0, -1.0)};

        _halfExtent = halfExtent;

        for (U8 i = 0; i < 8; ++i) {
            getGeometryVB()->modifyPositionValue(i, vertices[i] * _halfExtent);
        }

        getGeometryVB()->queueRefresh();
        
    }

    inline void mirrorBoundingBox(const BoundingBox& boundingBox) {
        const vec3<F32>* points = boundingBox.getPoints();
        for (U8 i = 0; i < 8; ++i) {
            getGeometryVB()->modifyPositionValue(i, points[i]);
        }
         getGeometryVB()->queueRefresh();
        _halfExtent = boundingBox.getHalfExtent();
    }

    virtual bool computeBoundingBox(SceneGraphNode& sgn) {
        if (sgn.getBoundingBoxConst().isComputed()) {
            return true;
        }
        sgn.getBoundingBox().set(-_halfExtent, _halfExtent);
        sgn.getBoundingBox().Multiply(0.5f);
        return SceneNode::computeBoundingBox(sgn);
    }

   private:
    vec3<F32> _halfExtent;
};

};  // namespace Divide

#endif