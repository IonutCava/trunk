/*
   Copyright (c) 2017 DIVIDE-Studio
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
    explicit Box3D(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, const vec3<F32>& size)
        : Object3D(context, parentCache, descriptorHash, name, ObjectType::BOX_3D, ObjectFlag::OBJECT_FLAG_NONE)
    {
        _halfExtent.set(size / 2);

        static const vec3<F32> vertices[] = {
            vec3<F32>(-1.0f, -1.0f,  1.0f),
            vec3<F32>(1.0f, -1.0f,  1.0f),
            vec3<F32>(-1.0f,  1.0f,  1.0f),
            vec3<F32>(1.0f,  1.0f,  1.0f),
            vec3<F32>(-1.0f, -1.0f, -1.0f),
            vec3<F32>(1.0f, -1.0f, -1.0f),
            vec3<F32>(-1.0f,  1.0f, -1.0f),
            vec3<F32>(1.0f,  1.0f, -1.0f)
        };
 
        static const U16 indices[] = {
            0, 1, 2,
            3, 7, 1,
            5, 4, 7,
            6, 2, 4,
            0, 1
        };
        
        VertexBuffer* vb = getGeometryVB();

        vb->setVertexCount(8);

        for (U8 i = 0; i < 8; i++) {
            vb->modifyPositionValue(i , vertices[i] * _halfExtent);
            vb->modifyNormalValue(i, vertices[i]);
        }

        for (U8 i = 0; i < 14; i++) {
            vb->addIndex(indices[i]);
        }

        vb->create(false);
        setFlag(UpdateFlag::BOUNDS_CHANGED);
    }

    inline void setHalfExtent(const vec3<F32>& halfExtent) {
        static const vec3<F32> vertices[] = {
            vec3<F32>(-1.0f, -1.0f,  1.0f),
            vec3<F32>( 1.0f, -1.0f,  1.0f),
            vec3<F32>(-1.0f,  1.0f,  1.0f),
            vec3<F32>( 1.0f,  1.0f,  1.0f),
            vec3<F32>(-1.0f, -1.0f, -1.0f),
            vec3<F32>( 1.0f, -1.0f, -1.0f),
            vec3<F32>(-1.0f,  1.0f, -1.0f),
            vec3<F32>( 1.0f,  1.0f, -1.0f)
        };

        _halfExtent = halfExtent;

        VertexBuffer* vb = getGeometryVB();
        for (U8 i = 0; i < 8; ++i) {
            vb->modifyPositionValue(i, vertices[i] * _halfExtent);
        }

        vb->queueRefresh();
        setFlag(UpdateFlag::BOUNDS_CHANGED);
    }

    inline void fromPoints(const std::initializer_list<vec3<F32>>& points,
                           const vec3<F32>& halfExtent) {

        VertexBuffer* vb = getGeometryVB();
        vb->modifyPositionValues(0, points);
        vb->queueRefresh();
        _halfExtent = halfExtent;
        setFlag(UpdateFlag::BOUNDS_CHANGED);
    }

    inline void updateBoundsInternal(SceneGraphNode& sgn) override {
        _boundingBox.set(-_halfExtent * 0.5f, _halfExtent * 0.5f);
        Object3D::updateBoundsInternal(sgn);
    }

   private:
    vec3<F32> _halfExtent;
};

TYPEDEF_SMART_POINTERS_FOR_CLASS(Box3D);

};  // namespace Divide

#endif