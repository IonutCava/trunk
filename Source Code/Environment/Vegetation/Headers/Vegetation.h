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

#ifndef VEGETATION_H_
#define VEGETATION_H_

#include "Utility/Headers/ImageTools.h"
#include "Graphs/Headers/SceneNode.h"
#include "Platform/Threading/Headers/Task.h"
#include "Platform/Video/Headers/RenderAPIWrapper.h"

namespace Divide {

class Terrain;
class Texture;
class Transform;
class SceneState;
class Framebuffer;
class ShaderBuffer;
class TerrainChunk;
class ShaderProgram;
class SceneGraphNode;
class RenderStateBlock;
class GenericVertexData;
enum class RenderStage : U32;

struct VegetationDetails {
    U16 billboardCount;
    F32 grassDensity;
    F32 grassScale;
    F32 treeDensity;
    F32 treeScale;
    stringImpl map;
    stringImpl name;
    stringImpl grassShaderName;
    Terrain* parentTerrain;
    Texture* grassBillboards;
};

/// Generates grass and trees on the terrain.
/// Grass VB's + all resources are stored locally in the class.
/// Trees are added to the SceneGraph and handled by the scene.
class Vegetation : public SceneNode {
   public:
    Vegetation(const VegetationDetails& details);
    ~Vegetation();

    void initialize(TerrainChunk* const terrainChunk);

    inline void toggleRendering(bool state) { _render = state; }

    bool getDrawCommands(SceneGraphNode& sgn,
                         RenderStage renderStage,
                         const SceneRenderState& sceneRenderState,
                         vectorImpl<GenericDrawCommand>& drawCommandsOut) override;

    inline bool isInView(const SceneRenderState& sceneRenderState,
                         SceneGraphNode& sgn,
                         Frustum::FrustCollision& collisionType,
                         const bool distanceCheck = true) override {
        collisionType = Frustum::FrustCollision::FRUSTUM_IN;
        return true;
    }

   protected:
    void sceneUpdate(const U64 deltaTime,
                     SceneGraphNode& sgn,
                     SceneState& sceneState) override;

    bool onDraw(SceneGraphNode& sgn, RenderStage renderStage)  override;

    void gpuCull();

   private:
    void uploadGrassData();
    void generateTrees();
    void generateGrass();

    U32 getQueryID();

   private:
    enum class CullType : U32 {
        PASS_THROUGH = 0,
        INSTANCE_CLOUD_REDUCTION = 1,
        HI_Z_CULL = 2,
        COUNT
    };
    // variables
    bool _render;  ///< Toggle vegetation rendering On/Off
    bool _success;
    std::atomic_bool _threadedLoadComplete;
    std::atomic_bool _stopLoadingRequest;
    Terrain* _terrain;
    TerrainChunk* _terrainChunk;
    F32 _grassDensity, _treeDensity;
    U16 _billboardCount;  ///< Vegetation cumulated density
    F32 _grassSize, _grassScale, _treeScale;
    F32 _windX, _windZ, _windS, _time;
    U64 _stateRefreshInterval;
    U64 _stateRefreshIntervalBuffer;
    ImageTools::ImageData _map;  ///< Dispersion map for vegetation placement
    Texture* _grassBillboards;
    ShaderProgram* _cullShader;
    stringImpl _grassShaderName;
    bool _shadowMapped;
    U32  _grassStateBlockHash;
    bool _culledFinal;
    U32 _readBuffer;
    U32 _writeBuffer;
    U32 _instanceCountGrass;
    U32 _instanceCountTrees;
    std::array<U32, to_const_uint(CullType::COUNT)> _instanceRoutineIdx;
    vectorImpl<F32> _grassScales;
    vectorImpl<vec4<F32> > _grassPositions;
    GenericVertexData* _grassGPUBuffer[2];
    GenericVertexData* _treeGPUBuffer[2];
    Task_ptr _generateVegetation;
    ShaderBuffer* _grassMatrices;
    static bool _staticDataUpdated;
    GenericDrawCommand _cullDrawCommand;
};

};  // namespace Divide

#endif