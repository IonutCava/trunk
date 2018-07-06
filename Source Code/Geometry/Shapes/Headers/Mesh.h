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

#ifndef _MESH_H_
#define _MESH_H_

/**
DIVIDE-Engine: 21.10.2010 (Ionut Cava)

Mesh class. This class wraps all of the renderable geometry drawn by the engine.
The only exceptions are: Terrain (including TerrainChunk) and Vegetation.

Meshes are composed of at least 1 submesh that contains vertex data, texture info and so on.
A mesh has a name, position, rotation, scale and a Boolean value that enables or disables rendering
across the network and one that disables rendering altogether;

Note: all transformations applied to the mesh affect every submesh that compose the mesh.
*/

#include "Object3D.h"
#include <assimp/anim.h>

class SubMesh;
class Mesh : public Object3D {
public:

    Mesh(ObjectFlag flag = OBJECT_FLAG_NONE);

    virtual ~Mesh();

    bool computeBoundingBox(SceneGraphNode* const sgn);

    virtual void postLoad(SceneGraphNode* const sgn);

    virtual void addSubMesh(SubMesh* const subMesh);

    /// Use playAnimations() to toggle animation playback for the current mesh (and all submeshes) on or off
    inline void playAnimations(const bool state)       { _playAnimations = state; }
    inline bool playAnimations()                 const { return _playAnimations; }

protected:
    /// Called from SceneGraph "sceneUpdate"
    virtual void sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState);
    virtual void render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState, const RenderStage& currentRenderStage) { }

protected:

    typedef Unordered_map<std::string, SceneGraphNode*> childrenNodes;
    typedef Unordered_map<U32, SubMesh*> subMeshRefMap;

    bool _visibleToNetwork;
    bool _playAnimations;
    bool _playAnimationsCurrent;

    vectorImpl<std::string > _subMeshes;
    subMeshRefMap            _subMeshRefMap;
    BoundingBox              _maxBoundingBox;
};

#endif