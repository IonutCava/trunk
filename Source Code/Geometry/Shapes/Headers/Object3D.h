/*
   Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _OBJECT_3D_H_
#define _OBJECT_3D_H_

#include "Graphs/Headers/SceneGraphNode.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

namespace Divide {

class BoundingBox;
enum class RigidBodyShape : U8;

class Object3D : public SceneNode {
   public:
    enum class ObjectType : U8 {
        SPHERE_3D = 0,
        BOX_3D = 1,
        QUAD_3D = 2,
        PATCH_3D = 3,
        MESH = 4,
        SUBMESH = 5,
        TERRAIN = 6,
        DECAL = 7,
        COUNT
    };

    enum class ObjectFlag : U8 {
        OBJECT_FLAG_SKINNED = toBit(1),
        OBJECT_FLAG_NO_VB = toBit(2)
    };


    explicit Object3D(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, ObjectType type, U32 flagMask = 0);
    explicit Object3D(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, ObjectType type, ObjectFlag flag);
    explicit Object3D(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, const stringImpl& resourceName, const stringImpl& resourceLocation, ObjectType type, ObjectFlag flag);
    explicit Object3D(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, const stringImpl& resourceName, const stringImpl& resourceLocation, ObjectType type, U32 flagMask);

    virtual ~Object3D();

    virtual VertexBuffer* const getGeometryVB() const;
    inline void setGeometryVBDirty() { _geometryDirty = true; }

    inline ObjectType getObjectType() const { return _geometryType; }

    inline void setObjectFlag(ObjectFlag flag) {
        SetBit(_geometryFlagMask, to_U32(flag));
    }
    inline void clearObjectFlag(ObjectFlag flag) {
        ClearBit(_geometryFlagMask, to_U32(flag));
    }

    inline bool getObjectFlag(ObjectFlag flag) {
        return BitCompare(_geometryFlagMask, to_U32(flag));
    }

    inline U32 getObjectFlagMask() const {
        return _geometryFlagMask;
    }

    virtual void postLoad(SceneGraphNode& sgn);

    virtual bool onRender(SceneGraphNode& sgn,
                          const SceneRenderState& sceneRenderState,
                          RenderStagePass renderStagePass) override;
                        
    virtual void onAnimationChange(SceneGraphNode& sgn, I32 newIndex) { 
        ACKNOWLEDGE_UNUSED(sgn);
        ACKNOWLEDGE_UNUSED(newIndex);
    }
    /// Use playAnimations() to toggle animation playback for the current object
    /// (and all subobjects) on or off
    inline void playAnimations(const bool state) { _playAnimations = state; }
    inline bool playAnimations() const { return _playAnimations; }

    inline void setGeometryPartitionID(size_t ID) {
        _geometryPartitionID = to_U16(ID);
    }

    // Procedural geometry deformation support?
    inline vector<vec3<U32> >& getTriangles() {
        return _geometryTriangles;
    }

    inline const vector<vec3<U32> >& getTriangles() const {
        return _geometryTriangles;
    }

    inline void reserveTriangleCount(U32 size) {
        _geometryTriangles.reserve(size);
    }

    inline void addTriangle(const vec3<U32>& triangle) {
        _geometryTriangles.push_back(triangle);
    }

    inline void addTriangles(const vector<vec3<U32>>& triangles) {
        reserveTriangleCount(to_U32(triangles.size() + _geometryTriangles.size()));
        _geometryTriangles.insert(std::end(_geometryTriangles),
                                  std::cbegin(triangles),
                                  std::cend(triangles));
    }

    // Create a list of triangles from the vertices + indices lists based on primitive type
    bool computeTriangleList(bool force = false);

    static vector<SceneGraphNode*> filterByType(const vector<SceneGraphNode*>& nodes, ObjectType filter);

   protected:
    void rebuild();
    virtual void rebuildVB();

    bool isPrimitive();
    /// Use a custom vertex buffer for this object (e.g., a submesh uses the mesh's vb)
    /// Please manually delete the old VB if available before replacing!
    virtual void setGeometryVB(VertexBuffer* const vb);

    virtual void buildDrawCommands(SceneGraphNode& sgn,
                                   RenderStagePass renderStagePass,
                                   RenderPackage& pkgInOut) override;
   protected:
    GFXDevice& _context;
    bool _playAnimations;
    U32 _geometryFlagMask;
    U16 _geometryPartitionID;
    ObjectType _geometryType;
    RigidBodyShape _rigidBodyShape;
    /// 3 indices, pointing to position values, that form a triangle in the mesh.
    /// used, for example, for cooking collision meshes
    vector<vec3<U32> > _geometryTriangles;

  private:
     bool _geometryDirty;
     /// A custom, override vertex buffer
     VertexBuffer* _buffer;
};

TYPEDEF_SMART_POINTERS_FOR_TYPE(Object3D);

};  // namespace Divide
#endif