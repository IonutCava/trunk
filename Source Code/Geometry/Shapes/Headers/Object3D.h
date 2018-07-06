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

#ifndef _OBJECT_3D_H_
#define _OBJECT_3D_H_

#include "Graphs/Headers/SceneGraphNode.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

namespace Divide {

class BoundingBox;
class Object3D : public SceneNode {
   public:
    enum class ObjectType : U32 {
        SPHERE_3D = 0,
        BOX_3D = 1,
        QUAD_3D = 2,
        TEXT_3D = 3,
        MESH = 4,
        SUBMESH = 5,
        TERRAIN = 6,
        FLYWEIGHT = 7,
        COUNT
    };

    enum class ObjectFlag : U32 {
        OBJECT_FLAG_NONE = toBit(1),
        OBJECT_FLAG_SKINNED = toBit(2),
        OBJECT_FLAG_NO_VB = toBit(3),
        COUNT
    };

    explicit Object3D(ObjectType type, ObjectFlag flag);

    explicit Object3D(ObjectType type, U32 flagMask);

    explicit Object3D(const stringImpl& name, ObjectType type, ObjectFlag flag);

    explicit Object3D(const stringImpl& name, ObjectType type, U32 flagMask);

    virtual ~Object3D();

    virtual VertexBuffer* const getGeometryVB() const;
    inline ObjectType getObjectType() const { return _geometryType; }
    inline U32 getFlagMask() const { return _geometryFlagMask; }

    virtual void postLoad(SceneGraphNode& sgn);

    virtual bool onDraw(SceneGraphNode& sgn,
                        RenderStage currentStage);

    virtual bool onDraw(RenderStage currentStage);

    inline bool isSkinned() const {
        return BitCompare(getFlagMask(),
                          to_uint(ObjectFlag::OBJECT_FLAG_SKINNED));
    }

    virtual bool updateAnimations(SceneGraphNode& sgn) { return false; }
    /// Use playAnimations() to toggle animation playback for the current object
    /// (and all subobjects) on or off
    inline void playAnimations(const bool state) { _playAnimations = state; }
    inline bool playAnimations() const { return _playAnimations; }

    inline void setGeometryPartitionID(size_t ID) {
        _geometryPartitionID = to_ushort(ID);
    }

    inline const vectorImpl<vec3<U32> >& getTriangles() const {
        return _geometryTriangles;
    }
    inline void reserveTriangleCount(U32 size) {
        _geometryTriangles.reserve(size);
    }
    inline void addTriangle(const vec3<U32>& tri) {
        _geometryTriangles.push_back(tri);
    }

    // Create a list of triangles from the vertices + indices lists based on
    // primitive type
    bool computeTriangleList(bool force = false);

   protected:
    bool isPrimitive();

    virtual void computeNormals();
    virtual void computeTangents();
    /// Use a custom vertex buffer for this object (e.g., a submesh uses the
    /// mesh's vb)
    /// Please manually delete the old VB if available before replacing!
    virtual void setGeometryVB(VertexBuffer* const vb);

    virtual bool getDrawCommands(SceneGraphNode& sgn,
                                 RenderStage renderStage,
                                 const SceneRenderState& sceneRenderState,
                                 vectorImpl<GenericDrawCommand>& drawCommandsOut) override;
   protected:
    bool _update;
    bool _playAnimations;
    U32 _geometryFlagMask;
    U16 _geometryPartitionID;
    ObjectType _geometryType;
    /// 3 indices, pointing to position values, that form a triangle in the
    /// mesh.
    /// used, for example, for cooking collision meshes
    vectorImpl<vec3<U32> > _geometryTriangles;
    /// A custom, override vertex buffer
    VertexBuffer* _buffer;
};

};  // namespace Divide
#endif