#include "stdafx.h"

#include "Headers/Sphere3D.h"

namespace Divide {

Sphere3D::Sphere3D(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, F32 radius, U32 resolution)
    : Object3D(context, parentCache, descriptorHash, name, ObjectType::SPHERE_3D),
    _radius(radius),
    _resolution(resolution)
{
    _vertexCount = resolution * resolution;
    getGeometryVB()->setVertexCount(_vertexCount);
    getGeometryVB()->reserveIndexCount(_vertexCount);
    if (_vertexCount + 1 > std::numeric_limits<U16>::max()) {
        getGeometryVB()->useLargeIndices(true);
    }
    rebuild();
}

void Sphere3D::setRadius(F32 radius) {
    _radius = radius;
    setGeometryVBDirty();
}

void Sphere3D::setResolution(U32 resolution) {
    _resolution = resolution;
    setGeometryVBDirty();
}

// SuperBible stuff
void Sphere3D::rebuildVB() {
    U32 slices = _resolution;
    U32 stacks = _resolution;

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
    _geometryTriangles.resize(0);
    setBoundsChanged();
}

void Sphere3D::updateBoundsInternal() {
    // add some depth padding for collision and nav meshes
    _boundingBox.set(vec3<F32>(-_radius), vec3<F32>(_radius));
    Object3D::updateBoundsInternal();
}

}; //namespace Divide