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
    U16 billboardCount = 0;
    Str128 name = "";
    stringImpl billboardTextureArray = "";
    std::shared_ptr<ImageTools::ImageData> grassMap;
    std::shared_ptr<ImageTools::ImageData> treeMap;
    eastl::weak_ptr<Terrain> parentTerrain;
    vectorEASTL<stringImpl> treeMeshes;
    vec4<F32> grassScales, treeScales;
    std::array<vec3<F32>, 4> treeRotations;
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

    void buildDrawCommands(SceneGraphNode& sgn,
                           RenderStagePass renderStagePass,
                           RenderPackage& pkgInOut) final;

    void getStats(U32& maxGrassInstances, U32& maxTreeInstances) const;

    //ToDo: Multiple terrains will NOT support this! To fix! -Ionut
    static void destroyStaticData();
    static void precomputeStaticData(GFXDevice& gfxDevice, U32 chunkSize, U32 maxChunkCount);
    static void createAndUploadGPUData(GFXDevice& gfxDevice, const Terrain_ptr& terrain, const VegetationDetails& vegDetails);

  protected:
    static void createVegetationMaterial(GFXDevice& gfxDevice, const Terrain_ptr& terrain, const VegetationDetails& vegDetails);

    void postLoad(SceneGraphNode& sgn)  final;

    void sceneUpdate(const U64 deltaTimeUS,
                     SceneGraphNode& sgn,
                     SceneState& sceneState) final;

    void onRefreshNodeData(SceneGraphNode& sgn,
                           RenderStagePass renderStagePass,
                           const Camera& camera,
                           bool quick,
                           GFX::CommandBuffer& bufferInOut) final;
   private:
    void uploadVegetationData(SceneGraphNode& sgn);
    void computeVegetationTransforms(const Task& parentTask, bool treeData);

    const char* getResourceTypeName() const noexcept final { return "Vegetation"; }

   private:
    GFXDevice& _context;
    TerrainChunk& _terrainChunk;
    // variables
    eastl::weak_ptr<Terrain> _terrain;
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

    SceneGraphNode* _treeParentNode;
    PushConstants _cullPushConstants;

    bool _shadowMapped;
    U32 _instanceCountGrass;
    U32 _instanceCountTrees;
    F32 _grassDistance;
    F32 _treeDistance;

    Task* _buildTask = nullptr;

    Pipeline* _cullPipelineGrass;
    Pipeline* _cullPipelineTrees;
    vectorEASTL<VegetationData> _tempGrassData;
    vectorEASTL<VegetationData> _tempTreeData;

    static U32 s_maxGrassInstances;
    static U32 s_maxTreeInstances;
    static ShaderProgram_ptr s_cullShaderGrass;
    static ShaderProgram_ptr s_cullShaderTrees;
    static std::atomic_uint s_bufferUsage;
    static VertexBuffer* s_buffer;
    static ShaderBuffer* s_treeData;
    static ShaderBuffer* s_grassData;
    static vector<Mesh_ptr> s_treeMeshes;

    static std::unordered_set<vec2<F32>> s_treePositions;
    static std::unordered_set<vec2<F32>> s_grassPositions;

    static U32 s_maxChunks;
    static bool s_buffersBound;

    static Material_ptr s_treeMaterial;
    static Material_ptr s_vegetationMaterial;
};

TYPEDEF_SMART_POINTERS_FOR_TYPE(Vegetation);

};  // namespace Divide

#endif