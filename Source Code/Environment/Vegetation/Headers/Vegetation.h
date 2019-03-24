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

#ifndef VEGETATION_H_
#define VEGETATION_H_

#include "Utility/Headers/ImageTools.h"
#include "Graphs/Headers/SceneNode.h"
#include "Platform/Threading/Headers/Task.h"
#include "Platform/Video/Headers/RenderAPIWrapper.h"
#include "Platform/Video/Headers/RenderStagePass.h"

namespace Divide {

class SceneState;
class VertexBuffer;
class RenderTarget;
class ShaderBuffer;
class TerrainChunk;
class SceneGraphNode;
class PlatformContext;
class RenderStateBlock;
class GenericVertexData;
enum class RenderStage : U8;

FWD_DECLARE_MANAGED_CLASS(Mesh);
FWD_DECLARE_MANAGED_CLASS(Texture);
FWD_DECLARE_MANAGED_CLASS(Terrain);
FWD_DECLARE_MANAGED_CLASS(ShaderProgram);

struct VegetationDetails {
    U16 billboardCount;
    stringImpl name;
    std::shared_ptr<ImageTools::ImageData> grassMap;
    std::shared_ptr<ImageTools::ImageData> treeMap;
    std::weak_ptr<Terrain> parentTerrain;
    vectorEASTL<stringImpl> treeMeshes;
    vec4<F32> grassScales, treeScales;
    std::array<vec3<F32>, 4> treeRotations;
    Material_ptr vegetationMaterialPtr;
};

struct VegetationData {
    vec4<F32> _positionAndScale;
    vec4<F32> _orientationQuat;
    //x - width extent, y = height extent, z = array index, w - lod
    vec4<F32> _data = { 1.0f, 1.0f, 1.0f, 0.0f };
};
//RenderDoc: mat4 transform; vec4 posAndIndex; vec4 extentAndRender;

/// Generates grass on the terrain.
/// Grass VB's + all resources are stored locally in the class.
class Vegetation : public SceneNode {
   public:
    explicit Vegetation(GFXDevice& context, TerrainChunk& parentChunk, const VegetationDetails& details);
    ~Vegetation();


    inline void toggleRendering(bool state) noexcept { _render = state; }

    void buildDrawCommands(SceneGraphNode& sgn,
                           RenderStagePass renderStagePass,
                           RenderPackage& pkgInOut) override;

    //ToDo: Multiple terrains will NOT support this! To fix! -Ionut
    static void precomputeStaticData(GFXDevice& gfxDevice, U32 chunkSize, U32 maxChunkCount, U32& maxGrassInstances, U32& maxTreeInstances);


  protected:
    void postLoad(SceneGraphNode& sgn)  override;

    void sceneUpdate(const U64 deltaTimeUS,
                     SceneGraphNode& sgn,
                     SceneState& sceneState) override;

    void onRefreshNodeData(SceneGraphNode& sgn,
                           RenderStagePass renderStagePass,
                           const Camera& camera,
                           GFX::CommandBuffer& bufferInOut) override;
    bool onRender(SceneGraphNode& sgn,
                  const Camera& camera,
                  RenderStagePass renderStagePass,
                  bool refreshData) override;

    bool getDrawState(const SceneGraphNode& sgn, RenderStagePass renderStage, U8 LoD) const override;

   private:
    void uploadVegetationData();
    void computeVegetationTransforms(const Task& parentTask, bool treeData);

   private:
    GFXDevice& _context;
    TerrainChunk& _terrainChunk;
    // variables
    bool _render;  ///< Toggle vegetation rendering On/Off
    bool _success;
    std::weak_ptr<Terrain> _terrain;
    U16 _billboardCount;  ///< Vegetation cumulated density
    F32 _windX = 0.0f, _windZ = 0.0f, _windS = 0.0f, _time = 0.0f;
    U64 _stateRefreshIntervalUS = 0ULL;
    U64 _stateRefreshIntervalBufferUS = 0ULL;
    vec4<F32> _grassScales, _treeScales;
    std::array<vec3<F32>, 4> _treeRotations;
    vec4<F32> _grassExtents;
    vec4<F32> _treeExtents;
    vectorEASTL<stringImpl> _treeMeshNames;
    std::shared_ptr<ImageTools::ImageData> _grassMap;  ///< Dispersion map for grass placement
    std::shared_ptr<ImageTools::ImageData> _treeMap;  ///< Dispersion map for tree placement
    ShaderProgram_ptr _cullShaderGrass;
    ShaderProgram_ptr _cullShaderTrees;
    bool _shadowMapped;
    U32 _instanceCountGrass;
    U32 _instanceCountTrees;

    Pipeline* _cullPipelineGrass;
    Pipeline* _cullPipelineTrees;
    vectorEASTL<VegetationData> _tempGrassData;
    vectorEASTL<VegetationData> _tempTreeData;

    static std::atomic_uint s_bufferUsage;
    static VertexBuffer* s_buffer;
    static ShaderBuffer* s_treeData;
    static ShaderBuffer* s_grassData;
    static vector<Mesh_ptr> s_treeMeshes;
    static std::unordered_set<vec2<F32>> s_treePositions;
    static std::unordered_set<vec2<F32>> s_grassPositions;

    static size_t s_maxChunks;
    static size_t s_maxTreeInstancesPerChunk;
    static size_t s_maxGrassInstancesPerChunk;
    static bool s_buffersBound;
    static std::array<bool, to_base(RenderStage::COUNT)> s_stageRefreshed;

    static Material_ptr s_treeMaterial;
};

TYPEDEF_SMART_POINTERS_FOR_TYPE(Vegetation);

};  // namespace Divide

#endif