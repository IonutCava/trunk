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

BETTER_ENUM(ObjectType, U8,
    SPHERE_3D,
    BOX_3D,
    QUAD_3D,
    PATCH_3D,
    MESH,
    SUBMESH,
    TERRAIN,
    DECAL,
    COUNT
);

class Object3D : public SceneNode {
   public:

    enum class ObjectFlag : U8 {
        OBJECT_FLAG_SKINNED = toBit(1),
        OBJECT_FLAG_NO_VB = toBit(2)
    };

    explicit Object3D(GFXDevice& context, ResourceCache* parentCache, size_t descriptorHash, const Str128& name, const Str128& resourceName, const stringImpl& resourceLocation, ObjectType type, U32 flagMask);

    virtual ~Object3D();

    virtual VertexBuffer* const getGeometryVB() const;
    inline void setGeometryVBDirty() noexcept { _geometryDirty = true; }

    inline ObjectType getObjectType() const noexcept { return _geometryType; }

    inline void setObjectFlag(ObjectFlag flag) noexcept {
        SetBit(_geometryFlagMask, to_U32(flag));
    }
    inline void clearObjectFlag(ObjectFlag flag) noexcept {
        ClearBit(_geometryFlagMask, to_U32(flag));
    }

    inline bool getObjectFlag(ObjectFlag flag) const noexcept {
        return BitCompare(_geometryFlagMask, to_U32(flag));
    }

    inline U32 getObjectFlagMask() const noexcept {
        return _geometryFlagMask;
    }

    void postLoad(SceneGraphNode& sgn) override;

    virtual bool onRender(SceneGraphNode& sgn,
                          RenderingComponent& rComp,
                          const Camera& camera,
                          RenderStagePass renderStagePass,
                          bool refreshData) override;
                        
    virtual void onAnimationChange(SceneGraphNode& sgn, I32 newIndex) { 
        ACKNOWLEDGE_UNUSED(sgn);
        ACKNOWLEDGE_UNUSED(newIndex);
    }
    /// Use playAnimations() to toggle animation playback for the current object
    /// (and all subobjects) on or off
    virtual void playAnimations(const SceneGraphNode& sgn, const bool state);

    inline void setGeometryPartitionID(U8 lodIndex, U16 ID) {
        if (lodIndex < _geometryPartitionIDs.size()) {
            _geometryPartitionIDs[lodIndex] = ID;
        }
    }

    // Procedural geometry deformation support?
    inline vectorEASTL<vec3<U32> >& getTriangles() noexcept {
        return _geometryTriangles;
    }

    inline const vectorEASTL<vec3<U32> >& getTriangles() const noexcept {
        return _geometryTriangles;
    }

    inline void reserveTriangleCount(U32 size) {
        _geometryTriangles.reserve(size);
    }

    inline void addTriangle(const vec3<U32>& triangle) {
        _geometryTriangles.push_back(triangle);
    }

    inline void addTriangles(const vectorEASTL<vec3<U32>>& triangles) {
        reserveTriangleCount(to_U32(triangles.size() + _geometryTriangles.size()));
        _geometryTriangles.insert(eastl::cend(_geometryTriangles),
                                  eastl::cbegin(triangles),
                                  eastl::cend(triangles));
    }

    // Create a list of triangles from the vertices + indices lists based on primitive type
    bool computeTriangleList();

    static vectorSTD<SceneGraphNode*> filterByType(const vectorSTD<SceneGraphNode*>& nodes, ObjectType filter);
    static vectorEASTL<SceneGraphNode*> filterByType(const vectorEASTL<SceneGraphNode*>& nodes, ObjectType filter);

    virtual const char* getTypeName() const override;

    bool isPrimitive() const noexcept;

    void saveToXML(boost::property_tree::ptree& pt) const override;
    void loadFromXML(const boost::property_tree::ptree& pt)  override;

   protected:
    void rebuild();
    virtual void rebuildVB();

    /// Use a custom vertex buffer for this object (e.g., a submesh uses the mesh's vb)
    /// Please manually delete the old VB if available before replacing!
    virtual void setGeometryVB(VertexBuffer* const vb);

    virtual void buildDrawCommands(SceneGraphNode& sgn,
                                   RenderStagePass renderStagePass,
                                   RenderPackage& pkgInOut) override;

    virtual const char* getResourceTypeName() const noexcept override { return "Object3D"; }

   protected:
    GFXDevice& _context;
    std::array<U16, 4> _geometryPartitionIDs;
    U32 _geometryFlagMask = 0u;
    ObjectType _geometryType = ObjectType::COUNT;
    RigidBodyShape _rigidBodyShape = RigidBodyShape::SHAPE_COUNT;
    /// 3 indices, pointing to position values, that form a triangle in the mesh.
    /// used, for example, for cooking collision meshes
    vectorEASTL<vec3<U32> > _geometryTriangles;

  private:
     bool _geometryDirty = true;;
     /// A custom, override vertex buffer
     VertexBuffer* _buffer = nullptr;
};

TYPEDEF_SMART_POINTERS_FOR_TYPE(Object3D);

};  // namespace Divide
#endif