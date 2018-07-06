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

#ifndef _QUAD_3D_H_
#define _QUAD_3D_H_

#include "Geometry/Shapes/Headers/Object3D.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

namespace Divide {

class ShaderProgram;
class Quad3D : public Object3D {
   public:
    Quad3D(const bool doubleSided = true) : Object3D(ObjectType::QUAD_3D) {
        vec3<F32> vertices[] = {vec3<F32>(-1.0f, 1.0f, 0.0f),  // TOP LEFT
                                vec3<F32>(1.0f, 1.0f, 0.0f),  // TOP RIGHT
                                vec3<F32>(-1.0f, -1.0f, 0.0f),  // BOTTOM LEFT
                                vec3<F32>(1.0f, -1.0f, 0.0f)};  // BOTTOM RIGHT

        vec3<F32> normals[] = {vec3<F32>(0, 0, -1), vec3<F32>(0, 0, -1),
                               vec3<F32>(0, 0, -1), vec3<F32>(0, 0, -1)};

        vec2<F32> texcoords[] = {vec2<F32>(0, 1), vec2<F32>(1, 1),
                                 vec2<F32>(0, 0), vec2<F32>(1, 0)};
        U16 indices[] = {2, 0, 1, 1, 2, 3, 1, 0, 2, 2, 1, 3};

        getGeometryVB()->reservePositionCount(4);
        getGeometryVB()->reserveNormalCount(4);
        getGeometryVB()->reserveTangentCount(4);
        getGeometryVB()->getTexcoord().reserve(4);

        for (U8 i = 0; i < 4; i++) {
            getGeometryVB()->addPosition(vertices[i]);
            getGeometryVB()->addNormal(normals[i]);
            getGeometryVB()->getTexcoord().push_back(texcoords[i]);
        }
        U8 indiceCount = doubleSided ? 12 : 6;
        for (U8 i = 0; i < indiceCount; i++) {
            // CCW draw order
            getGeometryVB()->addIndex(indices[i]);
            //  v0----v1
            //   |    |
            //   |    |
            //  v2----v3
        }

        computeTangents();

        getGeometryVB()->Create();
    }

    enum CornerLocation {
        TOP_LEFT,
        TOP_RIGHT,
        BOTTOM_LEFT,
        BOTTOM_RIGHT,
        CORNER_ALL
    };

    vec3<F32> getCorner(CornerLocation corner) {
        switch (corner) {
            case TOP_LEFT:
                return getGeometryVB()->getPosition()[0];
            case TOP_RIGHT:
                return getGeometryVB()->getPosition()[1];
            case BOTTOM_LEFT:
                return getGeometryVB()->getPosition()[2];
            case BOTTOM_RIGHT:
                return getGeometryVB()->getPosition()[3];
            default:
                break;
        }
        // Default returns top left corner. Why? Don't care ... seems like a
        // good idea. - Ionut
        return getGeometryVB()->getPosition()[0];
    }

    void setNormal(CornerLocation corner, const vec3<F32>& normal) {
        switch (corner) {
            case TOP_LEFT:
                getGeometryVB()->modifyNormalValue(0, normal);
                break;
            case TOP_RIGHT:
                getGeometryVB()->modifyNormalValue(1, normal);
                break;
            case BOTTOM_LEFT:
                getGeometryVB()->modifyNormalValue(2, normal);
                break;
            case BOTTOM_RIGHT:
                getGeometryVB()->modifyNormalValue(3, normal);
                break;
            case CORNER_ALL: {
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

    void setCorner(CornerLocation corner, const vec3<F32>& value) {
        switch (corner) {
            case TOP_LEFT:
                getGeometryVB()->modifyPositionValue(0, value);
                break;
            case TOP_RIGHT:
                getGeometryVB()->modifyPositionValue(1, value);
                break;
            case BOTTOM_LEFT:
                getGeometryVB()->modifyPositionValue(2, value);
                break;
            case BOTTOM_RIGHT:
                getGeometryVB()->modifyPositionValue(3, value);
                break;
            default:
                break;
        }
        getGeometryVB()->queueRefresh();
    }

    // rect.xy = Top Left; rect.zw = Bottom right
    // Remember to invert for 2D mode
    void setDimensions(const vec4<F32>& rect) {
        getGeometryVB()->modifyPositionValue(0, vec3<F32>(rect.x, rect.w, 0));
        getGeometryVB()->modifyPositionValue(1, vec3<F32>(rect.z, rect.w, 0));
        getGeometryVB()->modifyPositionValue(2, vec3<F32>(rect.x, rect.y, 0));
        getGeometryVB()->modifyPositionValue(3, vec3<F32>(rect.z, rect.y, 0));
        getGeometryVB()->queueRefresh();
    }

    virtual bool computeBoundingBox(SceneGraphNode& sgn) {
        if (sgn.getBoundingBoxConst().isComputed()) {
            return true;
        }
        vec3<F32> min = getGeometryVB()->getPosition()[2];
        min.z =
            +0.0025f;  //<add some depth padding for collision and nav meshes
        sgn.getBoundingBox().setMax(getGeometryVB()->getPosition()[1]);
        sgn.getBoundingBox().setMin(min);
        return SceneNode::computeBoundingBox(sgn);
    }
};

};  // namespace Divide

#endif