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

#ifndef _OBJECT_3D_H_
#define _OBJECT_3D_H_

#include "Graphs/Headers/SceneGraphNode.h"
#include "Hardware/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

class BoundingBox;
class RenderInstance;

class Object3D : public SceneNode {
public:
    enum ObjectType {
        SPHERE_3D = toBit(0),
        BOX_3D    = toBit(1),
        QUAD_3D   = toBit(2),
        TEXT_3D   = toBit(3),
        MESH      = toBit(4),
        SUBMESH   = toBit(5),
        TERRAIN   = toBit(6), 
        FLYWEIGHT = toBit(7),
        OBJECT_3D_PLACEHOLDER = toBit(8)
    };

    enum ObjectFlag {
        OBJECT_FLAG_NONE         = toBit(0),
        OBJECT_FLAG_SKINNED      = toBit(1),
        OBJECT_FLAG_NO_VB        = toBit(2),
        OBJECT_FLAG_PLACEHOLDER  = toBit(3)
    };

    Object3D(const ObjectType& type = OBJECT_3D_PLACEHOLDER, U32 flag = OBJECT_FLAG_NONE);
    Object3D(const std::string& name, const ObjectType& type = OBJECT_3D_PLACEHOLDER, U32 flag = OBJECT_FLAG_NONE);

    virtual ~Object3D();

    virtual VertexBuffer*   const getGeometryVB()   const;
    inline  ObjectType            getObjectType()   const {return _geometryType;}
    inline  U32                   getFlagMask()     const {return _geometryFlagMask;}
    inline  RenderInstance* const renderInstance()  const {return _renderInstance;}

    virtual void  postLoad(SceneGraphNode* const sgn);
    virtual bool  onDraw(SceneGraphNode* const sgn, const RenderStage& currentStage);
    virtual bool  updateAnimations(SceneGraphNode* const sgn) { return false; }

    inline void   setGeometryPartitionId(size_t idx) { _geometryPartitionId = (U32)idx; }

    inline const vectorImpl<vec3<U32> >& getTriangles() const { return _geometryTriangles; }
    inline void reserveTriangleCount(U32 size)                { _geometryTriangles.reserve(size); }
    inline void addTriangle(const vec3<U32>& tri)             { _geometryTriangles.push_back(tri); }

    //Create a list of triangles from the vertices + indices lists based on primitive type
    bool computeTriangleList(bool force = false);

protected:
    virtual	void  render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState);

    virtual void computeNormals();
    virtual void computeTangents();

protected:
    bool		    _update;
    U32             _geometryFlagMask;
    U32             _geometryPartitionId;
    ObjectType      _geometryType;
    ///The actual render instance needed by the rendering API
    RenderInstance* _renderInstance;
    /// 3 indices, pointing to position values, that form a triangle in the mesh.
    /// used, for example, for cooking collision meshes
    vectorImpl<vec3<U32> > _geometryTriangles;	
};

#endif