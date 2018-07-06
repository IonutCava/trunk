/*
   Copyright (c) 2013 DIVIDE-Studio
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

#ifndef _BOX_3D_H_
#define _BOX_3D_H_

#include "Geometry/Shapes/Headers/Object3D.h"
#include "Hardware/Video/Buffers/VertexBufferObject/Headers/VertexBufferObject.h"

class Box3D : public Object3D
{
public:
    Box3D(F32 size) :  Object3D(BOX_3D,TRIANGLE_STRIP), _size(size){
        vec3<F32> vertices[] = {vec3<F32>(-1.0, -1.0,  1.0),
                                vec3<F32>( 1.0, -1.0,  1.0),
                                vec3<F32>(-1.0,  1.0,  1.0),
                                vec3<F32>( 1.0,  1.0,  1.0),
                                vec3<F32>(-1.0, -1.0, -1.0),
                                vec3<F32>( 1.0, -1.0, -1.0),
                                vec3<F32>(-1.0,  1.0, -1.0),
                                vec3<F32>( 1.0,  1.0, -1.0)};

        vec3<F32> normals[] = {vec3<F32>(-1, -1,  1),
                               vec3<F32>( 1, -1,  1),
                               vec3<F32>(-1,  1,  1),
                               vec3<F32>( 1,  1,  1),
                               vec3<F32>(-1, -1, -1),
                               vec3<F32>( 1, -1, -1),
                               vec3<F32>(-1,  1, -1),
                               vec3<F32>( 1,  1, -1)};

        U16 indices[] = {0, 1, 2, 3, 7, 1, 5,
                         4, 7, 6, 2, 4, 0, 1};

        _geometry->reservePositionCount(8);
        _geometry->reserveNormalCount(8);
        _geometry->setIndiceLimits(vec2<U32>(0,7));
        F32 halfExtent = size*0.5f;

        for(U8 i = 0; i < 8; i++){
            _geometry->addPosition(vertices[i] * halfExtent);
            _geometry->addNormal(normals[i]);
        }

        for(U8 i = 0; i < 14; i++){
            _geometry->addIndex(indices[i]);
        }

        _geometry->queueRefresh();
    }

    inline F32 getSize()    {return _size;}

    inline void  setSize(F32 size) {
        ///Since the initial box is half of the full extent already (in the constructor)
        ///Each vertex is already multiplied by 0.5 so, just multiply by the new size
        ///IMPORTANT!! -be aware of this - Ionut
        F32 halfExtent = size;

        for(U8 i = 0; i < 8; i++){
            _geometry->modifyPositionValue(i,_geometry->getPosition()[i] * halfExtent);
        }

        _geometry->queueRefresh();
        _size = size;
    }

    virtual bool computeBoundingBox(SceneGraphNode* const sgn){
        if(sgn->getBoundingBox().isComputed()) return true;
        sgn->getBoundingBox().set(vec3<F32>(-_size),vec3<F32>(_size));
        sgn->getBoundingBox().Multiply(0.5f);
        return SceneNode::computeBoundingBox(sgn);
    }
private:
    F32 _size;
};

#endif