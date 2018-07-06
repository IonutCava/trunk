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

#ifndef VEGETATION_H_
#define VEGETATION_H_

#include "Utility/Headers/ImageTools.h"
#include "Graphs/Headers/SceneNode.h"
#include "Hardware/Platform/Headers/Task.h"

class Terrain;
class Texture;
class Transform;
class SceneState;
class FrameBuffer;
class ShaderProgram;
class SceneGraphNode;
class RenderStateBlock;
class GenericVertexData;
enum RenderStage;
///Generates grass and trees on the terrain.
///Grass VB's + all resources are stored locally in the class.
///Trees are added to the SceneGraph and handled by the scene.
class Vegetation : public SceneNode {
public:
    Vegetation(const std::string& name, U16 billboardCount, D32 grassDensity, F32 grassScale, D32 treeDensity, F32 treeScale, 
               const std::string& map, Texture* grassBillboards,
               const std::string& grassShaderName);
    ~Vegetation();
    void postLoad(SceneGraphNode* const sgn) { SceneNode::postLoad(sgn); }
    void initialize(Terrain* const terrain,SceneGraphNode* const terrainSGN);
    inline void toggleRendering(bool state){_render = state;}
    ///parentTransform: the transform of the parent terrain node
    void render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState);
    inline bool isInView(const SceneRenderState& sceneRenderState, const BoundingBox& boundingBox, const BoundingSphere& sphere, const bool distanceCheck = true) { return true; }

protected:
    void sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState);
    bool onDraw(SceneGraphNode* const sgn, const RenderStage& renderStage);

private:
    bool uploadGrassData();
    void generateTrees();
    void generateGrass();
    void gpuCull();

private:
    //variables
    bool _render; ///< Toggle vegetation rendering On/Off
    bool _success;
    boost::atomic_bool _threadedLoadComplete, _stopLoadingRequest;
    SceneGraphNode* _terrainSGN;
    Terrain*        _terrain;
    D32 _grassDensity, _treeDensity;
    U16 _billboardCount;          ///< Vegetation cumulated density
    F32 _grassSize,_grassScale, _treeScale;
    F32 _windX, _windZ, _windS, _time;
    U64 _stateRefreshInterval;
    U64 _stateRefreshIntervalBuffer;
    ImageTools::ImageData _map;  ///< Dispersion map for vegetation placement
    Texture*              _grassBillboards;
    ShaderProgram*		  _grassShader[3];
    ShaderProgram*        _cullShader;
    std::string           _grassShaderName;
    bool _shadowMapped;
    RenderStateBlock*     _grassStateBlock;


    bool                   _culledFinal;
    U32                    _shadowQueryID;
    U32                    _drawQueryID;
    U32                    _billboardDivisor;
    U32                    _instanceCountGrass;
    U32                    _instanceCountTrees;
    vectorImpl<I32 >       _grassTextureIdx;
    vectorImpl<F32 >       _grassSizes;
    vectorImpl<vec3<F32> > _grassPositions;
    GenericVertexData*     _grassGPUBuffer;
    GenericVertexData*     _treeGPUBuffer;
    Task_ptr               _generateVegetation;
};

#endif