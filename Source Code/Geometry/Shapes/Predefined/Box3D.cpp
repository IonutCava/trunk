#include "stdafx.h"

#include "Headers/Box3D.h"

namespace Divide {

Box3D::Box3D(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, const vec3<F32>& size)
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
        vb->modifyPositionValue(i, vertices[i] * _halfExtent);
        vb->modifyNormalValue(i, vertices[i]);
    }

    for (U8 i = 0; i < 14; i++) {
        vb->addIndex(indices[i]);
    }

    vb->create(false);
    setFlag(UpdateFlag::BOUNDS_CHANGED);
}

void Box3D::setHalfExtent(const vec3<F32>& halfExtent) {
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

    _halfExtent = halfExtent;

    VertexBuffer* vb = getGeometryVB();
    for (U8 i = 0; i < 8; ++i) {
        vb->modifyPositionValue(i, vertices[i] * _halfExtent);
    }

    vb->queueRefresh();
    setFlag(UpdateFlag::BOUNDS_CHANGED);
}

void Box3D::fromPoints(const std::initializer_list<vec3<F32>>& points,
                        const vec3<F32>& halfExtent) {

    VertexBuffer* vb = getGeometryVB();
    vb->modifyPositionValues(0, points);
    vb->queueRefresh();
    _halfExtent = halfExtent;
    setFlag(UpdateFlag::BOUNDS_CHANGED);
}

void Box3D::updateBoundsInternal(SceneGraphNode& sgn) {
    _boundingBox.set(-_halfExtent * 0.5f, _halfExtent * 0.5f);
    Object3D::updateBoundsInternal(sgn);
}

}; //namespace Divide