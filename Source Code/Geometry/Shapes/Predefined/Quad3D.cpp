#include "stdafx.h"

#include "Headers/Quad3D.h"

namespace Divide {

Quad3D::Quad3D(GFXDevice& context,
               ResourceCache& parentCache,
               size_t descriptorHash,
               const stringImpl& name,
               const bool doubleSided)
    : Object3D(context,
                parentCache,
                descriptorHash,
                name,
                ObjectType::QUAD_3D,
                ObjectFlag::OBJECT_FLAG_NONE)
{
    U16 indices[] = { 2, 0, 1, 1, 2, 3, 1, 0, 2, 2, 1, 3 };

    getGeometryVB()->setVertexCount(4);
    getGeometryVB()->keepData(true);
    getGeometryVB()->modifyPositionValue(0, -1.0f, 1.0f, 0.0f); // TOP LEFT
    getGeometryVB()->modifyPositionValue(1, 1.0f, 1.0f, 0.0f); // TOP RIGHT
    getGeometryVB()->modifyPositionValue(2, -1.0f, -1.0f, 0.0f); // BOTTOM LEFT
    getGeometryVB()->modifyPositionValue(3, 1.0f, -1.0f, 0.0f); // BOTTOM RIGHT

    getGeometryVB()->modifyNormalValue(0, 0, 0, -1);
    getGeometryVB()->modifyNormalValue(1, 0, 0, -1);
    getGeometryVB()->modifyNormalValue(2, 0, 0, -1);
    getGeometryVB()->modifyNormalValue(3, 0, 0, -1);

    getGeometryVB()->modifyTexCoordValue(0, 0, 1);
    getGeometryVB()->modifyTexCoordValue(1, 1, 1);
    getGeometryVB()->modifyTexCoordValue(2, 0, 0);
    getGeometryVB()->modifyTexCoordValue(3, 1, 0);

    U8 indiceCount = doubleSided ? 12 : 6;
    for (U8 i = 0; i < indiceCount; i++) {
        // CCW draw order
        getGeometryVB()->addIndex(indices[i]);
        //  v0----v1
        //   |    |
        //   |    |
        //  v2----v3
    }

    getGeometryVB()->computeTangents();
    getGeometryVB()->create();
    setFlag(UpdateFlag::BOUNDS_CHANGED);
}

vec3<F32> Quad3D::getCorner(CornerLocation corner) {
    switch (corner) {
        case CornerLocation::TOP_LEFT:
            return getGeometryVB()->getPosition(0);
        case CornerLocation::TOP_RIGHT:
            return getGeometryVB()->getPosition(1);
        case CornerLocation::BOTTOM_LEFT:
            return getGeometryVB()->getPosition(2);
        case CornerLocation::BOTTOM_RIGHT:
            return getGeometryVB()->getPosition(3);
        default:
            break;
    }
    // Default returns top left corner. Why? Don't care ... seems like a
    // good idea. - Ionut
    return getGeometryVB()->getPosition(0);
}

void Quad3D::setNormal(CornerLocation corner, const vec3<F32>& normal) {
    switch (corner) {
        case CornerLocation::TOP_LEFT:
            getGeometryVB()->modifyNormalValue(0, normal);
            break;
        case CornerLocation::TOP_RIGHT:
            getGeometryVB()->modifyNormalValue(1, normal);
            break;
        case CornerLocation::BOTTOM_LEFT:
            getGeometryVB()->modifyNormalValue(2, normal);
            break;
        case CornerLocation::BOTTOM_RIGHT:
            getGeometryVB()->modifyNormalValue(3, normal);
            break;
        case CornerLocation::CORNER_ALL: {
            getGeometryVB()->modifyNormalValue(0, normal);
            getGeometryVB()->modifyNormalValue(1, normal);
            getGeometryVB()->modifyNormalValue(2, normal);
            getGeometryVB()->modifyNormalValue(3, normal);
        }
        default:
            break;
    }
    getGeometryVB()->queueRefresh();
}

void Quad3D::setCorner(CornerLocation corner, const vec3<F32>& value) {
    switch (corner) {
        case CornerLocation::TOP_LEFT:
            getGeometryVB()->modifyPositionValue(0, value);
            break;
        case CornerLocation::TOP_RIGHT:
            getGeometryVB()->modifyPositionValue(1, value);
            break;
        case CornerLocation::BOTTOM_LEFT:
            getGeometryVB()->modifyPositionValue(2, value);
            break;
        case CornerLocation::BOTTOM_RIGHT:
            getGeometryVB()->modifyPositionValue(3, value);
            break;
        default:
            break;
    }
    getGeometryVB()->queueRefresh();
    setFlag(UpdateFlag::BOUNDS_CHANGED);
}

// rect.xy = Top Left; rect.zw = Bottom right
// Remember to invert for 2D mode
void Quad3D::setDimensions(const vec4<F32>& rect) {
    getGeometryVB()->modifyPositionValue(0, rect.x, rect.w, 0);
    getGeometryVB()->modifyPositionValue(1, rect.z, rect.w, 0);
    getGeometryVB()->modifyPositionValue(2, rect.x, rect.y, 0);
    getGeometryVB()->modifyPositionValue(3, rect.z, rect.y, 0);
    getGeometryVB()->queueRefresh();
    setFlag(UpdateFlag::BOUNDS_CHANGED);
}

void Quad3D::updateBoundsInternal(SceneGraphNode& sgn) {
    // add some depth padding for collision and nav meshes
    _boundingBox.setMax(getGeometryVB()->getPosition(1));
    _boundingBox.setMin(getGeometryVB()->getPosition(2) + vec3<F32>(0.0f, 0.0f, 0.0025f));
    Object3D::updateBoundsInternal(sgn);
}
}; //namespace Divide