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

#ifndef _TERRAIN_TESSELLATOR_H_
#define _TERRAIN_TESSELLATOR_H_

#include "Rendering/Camera/Headers/Frustum.h"

//reference: https://victorbush.com/2015/01/tessellated-terrain/
namespace Divide {

class VertexBuffer;
class TerrainLoader;

enum class TessellatedTerrainNodeType : U8 {
    ROOT = 0,
    CHILD_1,
    CHILD_2,
    CHILD_3,
    CHILD_4
};

struct TessellatedTerrainNode {
    vec3<F32> origin = { 0.0f, 0.0f, 0.0f };
    vec2<F32> dim = { 0.0f, 0.0f };
    TessellatedTerrainNodeType type = TessellatedTerrainNodeType::ROOT;

    TessellatedTerrainNode *p  = nullptr;  // Parent
    TessellatedTerrainNode* c[4] = { nullptr, nullptr, nullptr, nullptr }; // Children

    TessellatedTerrainNode *n = nullptr; // Neighbor to north
    TessellatedTerrainNode *s = nullptr; // Neighbor to south
    TessellatedTerrainNode *e = nullptr; // Neighbor to east
    TessellatedTerrainNode *w = nullptr; // Neighbor to west
}; //struct TessellatedTerrainNode

struct TessellatedNodeData {
    vec4<F32> _positionAndTileScale = { 0.0f, 0.0f, 0.0f, 0.0f };
    vec4<F32> _tScale = {1.0f, 1.0f, 1.0f, 1.0f};
};

class TerrainTessellator {
public:
    struct Configuration {
        // The size of a patch in meters at which point to stop subdividing a terrain patch once it's width is less than the cutoff
        F32 _cutoffDistance = 100.0f;
        bool _useCameraDistance = true;
        bool _useFrustumCulling = true;
    };

    using TreeVector = vectorEASTL<TessellatedTerrainNode>;
    using RenderDataVector = std::vector<TessellatedNodeData>;

    static constexpr U32 MAX_TESS_NODES = 2048;

public:
    // Reserves memory for the terrain quadtree and initializes the data structure.
    TerrainTessellator();
    // Frees memory for the terrain quadtree.
    ~TerrainTessellator();

    // Builds a terrain quadtree based on specified parameters and current camera position.
    void createTree(const vec3<F32>& camPos, const vec3<F32>& origin, const vec2<U16>& terrainDimensions, const F32 patchSizeInMetres);

    // Prepare data to draw the terrain. Returns the final render depth
    bufferPtr updateAndGetRenderData(const Frustum& frust, U16& renderDepth, U8 LoD);

    // Search for a node in the tree.
    // x, z == the point we are searching for (trying to find the node with an origin closest to that point)
    // n = the current node we are testing
    TessellatedTerrainNode* find(TessellatedTerrainNode* const n, F32 x, F32 z, bool& found);

    const RenderDataVector& renderData() const;

    const vec3<F32>& getOrigin() const;
    const Frustum& getFrustum() const;
    U16 getRenderDepth() const;
    U16 getPrevRenderDepth() const;

    inline void overrideConfig(const Configuration& config) { _config = config; }
protected:
   
    // Determines whether a node should be subdivided based on its distance to the camera.
    // Returns true if the node should be subdivided.
    bool checkDivide(TessellatedTerrainNode* node);

    // Returns true if node is sub-divided. False otherwise.
    bool divideNode(TessellatedTerrainNode* node);

    //Allocates a new node in the terrain quadtree with the specified parameters.
    TessellatedTerrainNode* createNode(TessellatedTerrainNode* parent, TessellatedTerrainNodeType type, F32 x, F32 y, F32 z, F32 width, F32 height);

    // Calculate the tessellation scale factor for a node depending on the neighboring patches.
    vec4<F32> calcTessScale(TessellatedTerrainNode* node);

    // Traverses the terrain quadtree to draw nodes with no children.
    void renderRecursive(TessellatedTerrainNode* node, U16& renderDepth, U8 LoD);

    bool inDivideCheck(TessellatedTerrainNode* node) const;

private:
    Frustum _frustumCache;
    TreeVector _tree;
    RenderDataVector _renderData;
    vec3<F32> _cameraEyeCache;
    vec3<F32> _originCache;
    Configuration _config = {};
    I32 _numNodes = 0;
    U16 _renderDepth = 0;
    U16 _prevRenderDepth = 0;
    I8  _lastFrustumPlaneCache = -1;
}; //TerrainTessellator

}; //namespace Divide
#endif
