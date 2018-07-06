/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _SPHERE_3D_H_
#define _SPHERE_3D_H_

#include "Geometry/Shapes/Headers/Object3D.h"
#include "Hardware/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

class Sphere3D : public Object3D {
public:
    ///Change resolution to affect the spacing between vertices
    Sphere3D(F32 radius, F32 resolution) : Object3D(SPHERE_3D),
                                          _radius(radius),
                                          _resolution(resolution)
    {
        _dirty = true;
        _vertexCount = resolution * resolution;
        getGeometryVB()->reservePositionCount(_vertexCount);
        getGeometryVB()->reserveNormalCount(_vertexCount);
        getGeometryVB()->getTexcoord().reserve(_vertexCount);
        getGeometryVB()->reserveIndexCount(_vertexCount);
        if(_vertexCount+1 > std::numeric_limits<U16>::max()){
            getGeometryVB()->useLargeIndices(true);
        }
        clean();
    }

    inline F32	  getRadius()     {return _radius;}
    inline F32    getResolution() {return _resolution;}
    inline void   setRadius(F32 radius) {_radius = radius; _dirty = true; getGeometryVB()->queueRefresh();}
    inline void   setResolution(F32 resolution) {_resolution = resolution; _dirty = true; getGeometryVB()->queueRefresh();}

    virtual bool computeBoundingBox(SceneGraphNode* const sgn){
        if (sgn->getBoundingBoxConst().isComputed()) return true;
        sgn->getBoundingBox().set(vec3<F32>(-_radius), vec3<F32>(_radius));
        return SceneNode::computeBoundingBox(sgn);
    }

    bool onDraw(SceneGraphNode* const sgn, const RenderStage& currentStage){
        clean();
        return Object3D::onDraw(sgn, currentStage);
    }

protected:
    void clean(){
        if(_dirty){
            createSphere(_resolution,_resolution);
            _dirty = false;
        }
    }

private:

    //SuperBible stuff
    void createSphere(U32 slices, U32 stacks) {
        getGeometryVB()->Reset();
        F32 drho = M_PI / (F32) stacks;
        F32 dtheta = 2.0f * M_PI / (F32) slices;
        F32 ds = 1.0f / (F32) slices;
        F32 dt = 1.0f / (F32) stacks;
        F32 t = 1.0f;
        F32 s = 0.0f;
        U32 i, j;     // Looping variables
        U32 index = 0; ///for the index buffer

        for (i = 0; i < stacks; i++) {
            F32 rho  = i * drho;
            F32 srho = sin(rho);
            F32 crho = cos(rho);
            F32 srhodrho = sin(rho + drho);
            F32 crhodrho = cos(rho + drho);

            // Many sources of OpenGL sphere drawing code uses a triangle fan
            // for the caps of the sphere. This however introduces texturing
            // artifacts at the poles on some OpenGL implementations
            s = 0.0f;
            for ( j = 0; j <= slices; j++) {
                F32 theta  = (j == slices) ? 0.0f : j * dtheta;
                F32 stheta = -sin(theta);
                F32 ctheta = cos(theta);

                F32 x = stheta * srho;
                F32 y = ctheta * srho;
                F32 z = crho;

                getGeometryVB()->addPosition(vec3<F32>(x * _radius, y * _radius, z * _radius));
                getGeometryVB()->getTexcoord().push_back(vec2<F32>(s,t));
                getGeometryVB()->addNormal(vec3<F32>(x,y,z));
                getGeometryVB()->addIndex(index++);

                x = stheta * srhodrho;
                y = ctheta * srhodrho;
                z = crhodrho;
                s += ds;

                getGeometryVB()->addPosition(vec3<F32>(x * _radius, y * _radius, z * _radius));
                getGeometryVB()->getTexcoord().push_back(vec2<F32>(s,t - dt));
                getGeometryVB()->addNormal(vec3<F32>(x,y,z));
                getGeometryVB()->addIndex(index++);
            }
            t -= dt;
        }

        vec2<U32> indiceLimits;
        for(i = 0 ; i < getGeometryVB()->getIndexCount(); i++){
            if(indiceLimits.x > getGeometryVB()->getIndex(i))
                indiceLimits.x = getGeometryVB()->getIndex(i);
            if(indiceLimits.y < getGeometryVB()->getIndex(i))
                indiceLimits.y = getGeometryVB()->getIndex(i);
        }

        getGeometryVB()->Create();
    }

protected:
    F32 _radius, _resolution;
    U32 _vertexCount;
    bool _dirty;
};

#endif