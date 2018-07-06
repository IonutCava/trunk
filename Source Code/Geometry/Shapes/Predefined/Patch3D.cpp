#include "stdafx.h"

#include "Headers/Patch3D.h"

namespace Divide {

Patch3D::Patch3D(GFXDevice& context,
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

void Patch3D::build(const DELEGATE_CBK<void, const vec2<U16>&, VertexBuffer*>& buildFunction) {
    buildFunction(_dimensions, getGeometryVB());
}

void Patch3D::build() {
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
    vector<U32> indices(i_width*i_height * 4, 0);
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
    setBoundsChanged();
}

void Patch3D::updateBoundsInternal() {
    // add some depth padding for collision and nav meshes
    VertexBuffer* vb = getGeometryVB();
    for (U32 i = 0; i < vb->getVertexCount(); ++i) {
        _boundingBox.add(vb->getVertices()[i]._position);
    }
    if (COMPARE(_boundingBox.getMax().y, _boundingBox.getMin().y)) {
        _boundingBox.setMax(_boundingBox.getMax() + vec3<F32>(0.0f, 0.025f, 0.0f));
    }
    Object3D::updateBoundsInternal();
}

}; //namespace Divide