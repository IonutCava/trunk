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
        OBJECT_FLAG_NONE = 0,
        OBJECT_FLAG_SKINNED,
        OBJECT_FLAG_PLACEHOLDER
    };

    Object3D(const ObjectType& type = OBJECT_3D_PLACEHOLDER, const PrimitiveType& vbType = TRIANGLES,const ObjectFlag& flag = OBJECT_FLAG_NONE);
    Object3D(const std::string& name,const ObjectType& type = OBJECT_3D_PLACEHOLDER,const PrimitiveType& vbType = TRIANGLES,const ObjectFlag& flag = OBJECT_FLAG_NONE);

    virtual ~Object3D();

    inline  VertexBuffer*   const getGeometryVB()   const {assert(_geometry != nullptr); return _geometry;}
    inline  ObjectType            getType()         const {return _geometryType;}
    inline  ObjectFlag            getFlag()         const {return _geometryFlag;}
    inline  RenderInstance* const renderInstance()  const {return _renderInstance;}

    virtual void  postLoad(SceneGraphNode* const sgn);
    virtual bool  onDraw(SceneGraphNode* const sgn, const RenderStage& currentStage);
    virtual bool  updateAnimations(SceneGraphNode* const sgn) { return false; }
    //virtual void  optimizeForDepth(bool state = true,bool force = false) {if(_geometry) _geometry->optimizeForDepth(state,force);}

protected:
    virtual	void  render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState);

    virtual void computeNormals();
    virtual void computeTangents();

protected:
    bool		    _update;
    ObjectType      _geometryType;
    ObjectFlag      _geometryFlag;
    VertexBuffer*   _geometry;
    ///The actual render instance needed by the rendering API
    RenderInstance* _renderInstance;
};

#endif