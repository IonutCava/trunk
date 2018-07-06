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

#ifndef _PATCH_3D_H_
#define _PATCH_3D_H_

#include "Geometry/Shapes/Headers/Object3D.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

namespace Divide {

class ShaderProgram;
// Class used only for tessellated  geometry
class Patch3D : public Object3D {
public:
    explicit Patch3D(GFXDevice& context,
                        ResourceCache& parentCache,
                        size_t descriptorHash,
                        const stringImpl& name,
                        vec2<U16> dim)
        : Object3D(context,
                    parentCache,
                    descriptorHash,
                    name,
                    ObjectType::PATCH_3D,
                    ObjectFlag::OBJECT_FLAG_NONE),
        _dimensions(dim)

    {
    }

    void build(const DELEGATE_CBK<void, const vec2<U16>&, VertexBuffer*>& buildFunction) {
        buildFunction(_dimensions, getGeometryVB());
    }

    void build() {
        U16 width = _dimensions.w;
        U16 height = _dimensions.h;

        VertexBuffer* vb = getGeometryVB();
        vb->setVertexCount(width * height);

        F32 width_factor = 1.0f / width;
        F32 height_factor = 1.0f / height;
        U32 vert = 0;
        for (U16 y = 0; y < height; ++y) {
            for (U16 x = 0; x < width; ++x) {
                vb->modifyPositionValue(vert,
                                        x * width_factor,
                                        0.0f,
                                        y * height_factor);
                vert++;
            }
        }

        U16 i_width = width - 1;
        U16 i_height = height - 1;
        vectorImpl<U32> indices(i_width*i_height * 4, 0);
        for (U16 y = 0; y < i_height; ++y) {
            for (U16 x = 0; x < i_width; ++x) {
                U32 offset = (x + y * i_width) * 4;
                U32 p1 = x + y*width;
                U32 p2 = p1 + width;
                U32 p4 = p1 + 1;
                U32 p3 = p2 + 1;
                indices[offset + 0] = p1;
                indices[offset + 1] = p2;
                indices[offset + 2] = p3;
                indices[offset + 3] = p4;
            }
        }
        vb->useLargeIndices(true);
        vb->addIndices(indices, false);

        vb->create();
        setFlag(UpdateFlag::BOUNDS_CHANGED);
    }

    inline void updateBoundsInternal(SceneGraphNode& sgn) override {
        // add some depth padding for collision and nav meshes
        VertexBuffer* vb = getGeometryVB();
        for (U32 i = 0; i < vb->getVertexCount(); ++i) {
            _boundingBox.add(vb->getVertices()[i]._position);
        }
        if (COMPARE(_boundingBox.getMax().y,_boundingBox.getMin().y)) {
            _boundingBox.setMax(_boundingBox.getMax() + vec3<F32>(0.0f, 0.025f, 0.0f));
        }
        Object3D::updateBoundsInternal(sgn);
    }

protected:
    vec2<U16> _dimensions;
};

TYPEDEF_SMART_POINTERS_FOR_CLASS(Patch3D);

};  // namespace Divide

#endif //_PATCH_3D_H_
