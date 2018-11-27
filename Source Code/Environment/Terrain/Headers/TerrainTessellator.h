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

#include "Core/Math/Headers/MathVectors.h"

//reference: https://victorbush.com/2015/01/tessellated-terrain/
namespace Divide {

class VertexBuffer;
class TerrainLoader;

namespace Attorney {
    class TerrainTessellatorLoader;
};

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

    // Tessellation scale
    // negative x edge (0), positive x edge (1), negative z edge (2), positive z edge (3)
    vec4<F32> tscale = { 0.0f, 0.0f, 0.0f , 0.0f }; 

    TessellatedTerrainNode *p  = nullptr;  // Parent
    TessellatedTerrainNode* c[4] = { nullptr, nullptr, nullptr, nullptr }; // Children

    TessellatedTerrainNode *n = nullptr; // Neighbor to north
    TessellatedTerrainNode *s = nullptr; // Neighbor to south
    TessellatedTerrainNode *e = nullptr; // Neighbor to east
    TessellatedTerrainNode *w = nullptr; // Neighbor to west
}; //struct TessellatedTerrainNode

struct TessellatedNodeData {
    vec4<F32> _positionAndTileScale = { 0.0f, 0.0f, 0.0f, 0.0f };
    vec4<F32> _tScale = {0.0f, 0.0f, 0.0f, 0.0f};
};

class TerrainTessellator {
    friend class Attorney::TerrainTessellatorLoader;
public:
    typedef vector<TessellatedTerrainNode> TreeVector;
    typedef vector<TessellatedNodeData> RenderDataVector;

public:
    // Reserves memory for the terrain quadtree and initializes the data structure.
    TerrainTessellator();
    // Frees memory for the terrain quadtree.
    ~TerrainTessellator();

    // Builds a terrain quadtree based on specified parameters and current camera position.
    void createTree(const vec3<F32>& camPos, const vec3<F32>& origin, const vec2<U16>& terrainDimensions);

    // Prepare data to draw the terrain. Returns the final render depth
    U16 updateRenderData();

    // Search for a node in the tree.
    // x, z == the point we are searching for (trying to find the node with an origin closest to that point)
    // n = the current node we are testing
    TessellatedTerrainNode* find(TessellatedTerrainNode& n, F32 x, F32 z);

    const RenderDataVector& renderData() const;

    const vec3<F32>& getEye() const;
    const vec3<F32>& getOrigin() const;
    U16 getRenderDepth() const;

protected:
    // Resets the terrain quadtree.
    void clearTree();
    
    // Determines whether a node should be subdivided based on its distance to the camera.
    // Returns true if the node should be subdivided.
    bool checkDivide(TessellatedTerrainNode& node);

    // Returns true if node is sub-divided. False otherwise.
    bool divideNode(TessellatedTerrainNode& node);

    //Allocates a new node in the terrain quadtree with the specified parameters.
    TessellatedTerrainNode* createNode(TessellatedTerrainNode& parent, TessellatedTerrainNodeType type, F32 x, F32 y, F32 z, F32 width, F32 height);

    // Calculate the tessellation scale factor for a node depending on the neighboring patches.
    void calcTessScale(TessellatedTerrainNode& node);

    // Pushes a node (patch) to the GPU to be drawn.
    void renderNode(TessellatedTerrainNode& node, U16 crtDepth);

    // Traverses the terrain quadtree to draw nodes with no children.
    void renderRecursive(TessellatedTerrainNode& node, U16& renderDepth);

protected:
    static void initTessellationPatch(VertexBuffer* vb);

private:
    I32 _numNodes;
    U16 _renderDepth;
    vec3<F32> _cameraEyeCache;
    vec3<F32> _originiCache;
    TreeVector _tree;
    RenderDataVector _renderData;

}; //TerrainTessellator

namespace Attorney {

class TerrainTessellatorLoader {
  private:

  static void initTessellationPatch(VertexBuffer* vb) {
      TerrainTessellator::initTessellationPatch(vb);
  }

  friend class Divide::TerrainLoader;
};//class TerrainTessellatorLoader

}; //namespace Attorney

}; //namespace Divide
#endif
