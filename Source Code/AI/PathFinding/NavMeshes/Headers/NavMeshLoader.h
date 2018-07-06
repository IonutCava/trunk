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

// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
// Changes, Additions and Refactoring : Copyright (c) 2010-2011 Lethal Concept,
// LLC
// Changes, Additions and Refactoring Author: Simon Wittenberg (MD)
// The above license is fully inherited.

#ifndef _NAVIGATION_MESH_LOADER_H_
#define _NAVIGATION_MESH_LOADER_H_

#include "NavMeshConfig.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"
#include "Graphs/Headers/SceneNode.h"

namespace Divide {
namespace AI {
namespace Navigation {

// This struct contains the vertices and triangles in recast coords
class NavModelData {
   public:
    NavModelData()
        : _vertices(0),
          _vertexCount(0),
          _vertexCapacity(0),
          _normals(0),
          _triangleCount(0),
          _triangleCapacity(0),
          _triangles(0),
          _valid(false) {}

    void clear(bool del = true) {
        _valid = false;

        _vertexCount = _triangleCount = 0;
        _vertexCapacity = _vertexCount = 0;
        _triangleCapacity = _triangleCount = 0;

        if (del) {
            MemoryManager::DELETE_ARRAY(_vertices);
        } else {
            _vertices = 0;
        }
        if (del) {
            MemoryManager::DELETE_ARRAY(_triangles);
        } else {
            _triangles = 0;
        }
        if (del) {
            MemoryManager::DELETE_ARRAY(_normals);
        } else {
            _normals = 0;
        }
        _triangleAreaType.clear();
        _navMeshName = "";
    }

    inline bool isValid() const { return _valid; }
    inline void isValid(bool state) { _valid = state; }

    inline void setName(const stringImpl& name) { _navMeshName = name; }
    inline const stringImpl& getName() const { return _navMeshName; }

    inline const F32* getVerts() const { return _vertices; }
    inline const F32* getNormals() const { return _normals; }
    inline const I32* getTris() const { return _triangles; }
    inline U32 getVertCount() const { return _vertexCount; }
    inline U32 getTriCount() const { return _triangleCount; }

    inline vectorImpl<SamplePolyAreas>& getAreaTypes() {
        return _triangleAreaType;
    }

    F32* _vertices;
    F32* _normals;
    I32* _triangles;
    U32 _vertexCapacity;
    U32 _vertexCount;
    U32 _triangleCount;
    U32 _triangleCapacity;

   private:
    bool _valid;
    stringImpl _navMeshName;
    vectorImpl<SamplePolyAreas> _triangleAreaType;
};

namespace NavigationMeshLoader {
enum class MeshDetailLevel : U32 { 
    MAXIMUM = 0,
    BOUNDINGBOX = 1
};

/// Load the input geometry from file (Wavefront OBJ format) and save it in
/// 'outData'
bool loadMeshFile(NavModelData& outData, const char* fileName);
/// Save the navigation input geometry in Wavefront OBJ format
bool saveMeshFile(const NavModelData& inData, const char* filename);
/// Merge the data from two navigation geometry sources
NavModelData mergeModels(NavModelData& a, NavModelData& b,
                         bool delOriginals = false);
/// Parsing method that calls itself recursively untill all geometry has been
/// parsed
bool parse(const BoundingBox& box, NavModelData& outData, SceneGraphNode_wptr sgn);

void addVertex(NavModelData* modelData, const vec3<F32>& vertex);

void addTriangle(NavModelData* modelData, const vec3<U32>& triangleIndices,
                 U32 triangleIndexOffset = 0,
                 const SamplePolyAreas& areaType = SamplePolyAreas::SAMPLE_POLYAREA_GROUND);

char* parseRow(char* buf, char* bufEnd, char* row, I32 len);

I32 parseFace(char* row, I32* data, I32 n, I32 vcnt);
};
};  // namespace Navigation
};  // namespace AI
};  // namespace Divide

#endif