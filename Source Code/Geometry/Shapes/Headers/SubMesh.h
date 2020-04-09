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
#ifndef _SUB_MESH_H_
#define _SUB_MESH_H_

#include "Object3D.h"

/*
DIVIDE-Engine: 21.10.2010 (Ionut Cava)

A SubMesh is a geometry wrapper used to build a mesh. Just like a mesh, it can
be rendered locally or across
the server or disabled from rendering alltogheter.

Objects created from this class have theyr position in relative space based on
the parent mesh position.
(Same for scale,rotation and so on).

The SubMesh is composed of a VB object that contains vertx,normal and textcoord
data, a vector of materials,
and a name.
*/

#include "Mesh.h"

namespace Divide {

namespace Attorney {
    class SubMeshMesh;
    class SubMeshMeshImporter;
};

class MeshImporter;
class SubMesh : public Object3D {
    friend class Attorney::SubMeshMesh;
    friend class Attorney::SubMeshMeshImporter;

   public:
    explicit SubMesh(GFXDevice& context,
                     ResourceCache* parentCache,
                     size_t descriptorHash,
                     const Str128& name,
                     ObjectFlag flag = ObjectFlag::OBJECT_FLAG_NO_VB);

    virtual ~SubMesh();

    inline U32 getID() noexcept { return _ID; }
    /// When loading a submesh, the ID is the node index from the imported scene
    /// scene->mMeshes[n] == (SubMesh with _ID == n)
    inline void setID(U32 ID) noexcept { _ID = ID; }
    inline Mesh* getParentMesh() noexcept { return _parentMesh; }

   protected:
    void setParentMesh(Mesh* const parentMesh);

    void buildDrawCommands(SceneGraphNode& sgn,
                           RenderStagePass renderStagePass,
                           RenderPackage& pkgInOut) override;

    virtual const char* getResourceTypeName() const override { return "SubMesh"; }

    void sceneUpdate(const U64 deltaTimeUS,
                     SceneGraphNode& sgn,
                     SceneState& sceneState) override;
    // SGN node + parent mesh
    size_t maxReferenceCount() const noexcept override { return 2; }
   protected:
    bool _visibleToNetwork;
    bool _render;
    U32 _ID;
    Mesh* _parentMesh;
};

TYPEDEF_SMART_POINTERS_FOR_TYPE(SubMesh);

namespace Attorney {
class SubMeshMesh {
   private:
    static void setParentMesh(SubMesh& submesh, Mesh* const parentMesh) {
        submesh.setParentMesh(parentMesh);
    }

    friend class Divide::Mesh;
};

class SubMeshMeshImporter {
   private:
    static void setGeometryLimits(SubMesh& submesh,
                                  const vec3<F32>& min,
                                  const vec3<F32>& max) noexcept {
        submesh._boundingBox.set(min, max);
    }

    friend class Divide::MeshImporter;
};
};  // namespace Attorney
};  // namespace Divide

#endif