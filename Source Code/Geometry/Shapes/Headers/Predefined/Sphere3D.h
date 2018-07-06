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

#ifndef _SPHERE_3D_H_
#define _SPHERE_3D_H_

#include "Geometry/Shapes/Headers/Object3D.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

namespace Divide {

class Sphere3D : public Object3D {
   public:
    /// Change resolution to affect the spacing between vertices
    explicit Sphere3D(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, F32 radius, U32 resolution)
        : Object3D(context, parentCache, descriptorHash, name, ObjectType::SPHERE_3D, ObjectFlag::OBJECT_FLAG_NONE),
          _radius(radius),
          _resolution(resolution)
    {
        _dirty = true;
        _vertexCount = resolution * resolution;
        getGeometryVB()->setVertexCount(_vertexCount);
        getGeometryVB()->reserveIndexCount(_vertexCount);
        if (_vertexCount + 1 > std::numeric_limits<U16>::max()) {
            getGeometryVB()->useLargeIndices(true);
        }
        clean();
    }

    virtual ~Sphere3D() {}
    inline F32 getRadius() { return _radius; }
    inline U32 getResolution() { return _resolution; }
    inline void setRadius(F32 radius) {
        _radius = radius;
        _dirty = true;
    }

    inline void setResolution(U32 resolution) {
        _resolution = resolution;
        _dirty = true;
    }

    bool onRender(SceneGraphNode& sgn,
                  const SceneRenderState& sceneRenderState,
                  const RenderStagePass& renderStagePass) {
        clean();
        return Object3D::onRender(sgn, sceneRenderState, renderStagePass);
    }

   protected:
    void clean() {
        if (_dirty) {
            createSphere(_resolution, _resolution);
            _dirty = false;
        }
    }

   private:
    // SuperBible stuff
    void createSphere(U32 slices, U32 stacks) {
        VertexBuffer* const vb = getGeometryVB();

        vb->reset();
        F32 drho = to_F32(M_PI) / stacks;
        F32 dtheta = 2.0f * to_F32(M_PI) / slices;
        F32 ds = 1.0f / slices;
        F32 dt = 1.0f / stacks;
        F32 t = 1.0f;
        F32 s = 0.0f;
        U32 i, j;  // Looping variables
        U32 index = 0;  /// for the index buffer

        vb->setVertexCount(stacks * ((slices + 1) * 2));

        for (i = 0; i < stacks; i++) {
            F32 rho = i * drho;
            F32 srho = std::sin(rho);
            F32 crho = std::cos(rho);
            F32 srhodrho = std::sin(rho + drho);
            F32 crhodrho = std::cos(rho + drho);

            // Many sources of OpenGL sphere drawing code uses a triangle fan
            // for the caps of the sphere. This however introduces texturing
            // artifacts at the poles on some OpenGL implementations
            s = 0.0f;
            for (j = 0; j <= slices; j++) {
                F32 theta = (j == slices) ? 0.0f : j * dtheta;
                F32 stheta = -std::sin(theta);
                F32 ctheta = std::cos(theta);

                F32 x = stheta * srho;
                F32 y = ctheta * srho;
                F32 z = crho;

                vb->modifyPositionValue(index, x * _radius, y * _radius, z * _radius);
                vb->modifyTexCoordValue(index, s, t);
                vb->modifyNormalValue(index, x, y, z);
                vb->addIndex(index++);

                x = stheta * srhodrho;
                y = ctheta * srhodrho;
                z = crhodrho;
                s += ds;

                vb->modifyPositionValue(index, x * _radius, y * _radius, z * _radius);
                vb->modifyTexCoordValue(index, s, t - dt);
                vb->modifyNormalValue(index, x, y, z);
                vb->addIndex(index++);
            }
            t -= dt;
        }

        vb->create();
        vb->queueRefresh();
        setFlag(UpdateFlag::BOUNDS_CHANGED);
    }

    inline void updateBoundsInternal(SceneGraphNode& sgn) override {
        // add some depth padding for collision and nav meshes
        _boundingBox.set(vec3<F32>(-_radius), vec3<F32>(_radius));
        Object3D::updateBoundsInternal(sgn);
    }

   protected:
    F32 _radius;
    U32 _resolution;
    U32 _vertexCount;
    bool _dirty;
};

TYPEDEF_SMART_POINTERS_FOR_CLASS(Sphere3D);

};  // namespace Divide

#endif