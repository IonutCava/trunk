#include "stdafx.h"

#include "Headers/Vegetation.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Managers/Headers/SceneManager.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderPackage.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Material/Headers/Material.h"
#include "Platform/Headers/PlatformRuntime.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainChunk.h"
#include "Environment/Terrain/Quadtree/Headers/QuadtreeNode.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"
#include "Platform/Video/Buffers/VertexBuffer/GenericBuffer/Headers/GenericVertexData.h"

#include "ECS/Components/Headers/BoundsComponent.h"
#include "ECS/Components/Headers/RenderingComponent.h"
#include "ECS/Components/Headers/TransformComponent.h"

namespace Divide {

namespace {
    constexpr U32 WORK_GROUP_SIZE = 64;
    constexpr I16 g_maxRadiusSteps = 512;

    SharedMutex g_treeMeshLock;
};

bool Vegetation::s_buffersBound = false;
Material_ptr Vegetation::s_treeMaterial = nullptr;
Material_ptr Vegetation::s_vegetationMaterial = nullptr;

eastl::unordered_set<vec2<F32>> Vegetation::s_treePositions;
eastl::unordered_set<vec2<F32>> Vegetation::s_grassPositions;
ShaderBuffer* Vegetation::s_treeData = nullptr;
ShaderBuffer* Vegetation::s_grassData = nullptr;
VertexBuffer* Vegetation::s_buffer = nullptr;
ShaderProgram_ptr Vegetation::s_cullShaderGrass = nullptr;
ShaderProgram_ptr Vegetation::s_cullShaderTrees = nullptr;
vectorEASTL<Mesh_ptr> Vegetation::s_treeMeshes;
std::atomic_uint Vegetation::s_bufferUsage = 0;
U32 Vegetation::s_maxChunks = 0u;

//Per-chunk
U32 Vegetation::s_maxGrassInstances = 0u;
U32 Vegetation::s_maxTreeInstances = 0u;

Vegetation::Vegetation(GFXDevice& context, 
                       TerrainChunk& parentChunk,
                       const VegetationDetails& details)
    : SceneNode(context.parent().resourceCache(), 
                parentChunk.parent().descriptorHash() + parentChunk.ID(),
                details.name,
                details.name + "_" + to_stringImpl(parentChunk.ID()),
                "",
                SceneNodeType::TYPE_VEGETATION,
                to_base(ComponentType::TRANSFORM) | to_base(ComponentType::BOUNDS) | to_base(ComponentType::RENDERING)),

      _context(context),
      _terrainChunk(parentChunk),
      _billboardCount(details.billboardCount),
      _terrain(details.parentTerrain),
      _grassScales(details.grassScales),
      _treeScales(details.treeScales),
      _treeRotations(details.treeRotations),
      _treeMap(details.treeMap),
      _grassMap(details.grassMap)
{
    _treeMeshNames.insert(eastl::cend(_treeMeshNames), eastl::cbegin(details.treeMeshes), eastl::cend(details.treeMeshes));

    assert(_grassMap->data() != nullptr && _treeMap->data() != nullptr);

    setBounds(parentChunk.bounds());

    renderState().addToDrawExclusionMask(RenderStage::COUNT, RenderPassType::MAIN_PASS, -1);
    renderState().addToDrawExclusionMask(RenderStage::REFLECTION, RenderPassType::COUNT, -1);
    renderState().addToDrawExclusionMask(RenderStage::REFRACTION, RenderPassType::COUNT, -1);
    renderState().addToDrawExclusionMask(RenderStage::SHADOW, RenderPassType::COUNT, to_base(LightType::POINT));
    renderState().addToDrawExclusionMask(RenderStage::SHADOW, RenderPassType::COUNT, to_base(LightType::SPOT));
    renderState().addToDrawExclusionMask(RenderStage::SHADOW, RenderPassType::COUNT, to_U8(LightType::DIRECTIONAL), 1u);
    renderState().addToDrawExclusionMask(RenderStage::SHADOW, RenderPassType::COUNT, to_U8(LightType::DIRECTIONAL), 2u);

    renderState().minLodLevel(2u);
    renderState().drawState(false);

    setState(ResourceState::RES_LOADING);
    _buildTask = CreateTask(_context.context(),
        [this](const Task& parentTask) {
            s_bufferUsage.fetch_add(1);
            computeVegetationTransforms(parentTask, false);
            computeVegetationTransforms(parentTask, true);
            _instanceCountGrass = to_U32(_tempGrassData.size());
            _instanceCountTrees = to_U32(_tempTreeData.size());
        });

    Start(*_buildTask, TaskPriority::DONT_CARE);
}

Vegetation::~Vegetation()
{
    Console::printfn(Locale::get(_ID("UNLOAD_VEGETATION_BEGIN")), resourceName().c_str());
    U32 timer = 0;
    while (getState() == ResourceState::RES_LOADING) {
        // wait for the loading thread to finish first;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        timer += 10;
        if (timer > 3000) {
            break;
        }
    }
    assert(getState() != ResourceState::RES_LOADING);
    if (s_bufferUsage.fetch_sub(1) == 1) {
        destroyStaticData();
    }

    Console::printfn(Locale::get(_ID("UNLOAD_VEGETATION_END")));
}

void Vegetation::destroyStaticData() {
    {
        UniqueLock<SharedMutex> w_lock(g_treeMeshLock);
        s_treeMeshes.clear();
    }
    s_treeMaterial.reset();
    s_vegetationMaterial.reset();
    s_cullShaderGrass.reset();
    s_cullShaderTrees.reset();
}

void Vegetation::precomputeStaticData(GFXDevice& gfxDevice, U32 chunkSize, U32 maxChunkCount) {
    // Make sure this is ONLY CALLED FROM THE MAIN LOADING THREAD. All instances should call this in a serialized fashion
    if (s_buffer == nullptr) {
        constexpr bool useDoubleSided = true;

        const vec2<F32> pos000(cosf(Angle::to_RADIANS(0.000f)), sinf(Angle::to_RADIANS(0.000f)));
        const vec2<F32> pos120(cosf(Angle::to_RADIANS(120.0f)), sinf(Angle::to_RADIANS(120.0f)));
        const vec2<F32> pos240(cosf(Angle::to_RADIANS(240.0f)), sinf(Angle::to_RADIANS(240.0f)));

        const vec3<F32> vertices[] = {
            vec3<F32>(-pos000.x, 0.0f, -pos000.y),  vec3<F32>(-pos000.x, 1.0f, -pos000.y),  vec3<F32>(pos000.x, 1.0f, pos000.y), vec3<F32>(pos000.x, 0.0f, pos000.y),
            vec3<F32>(-pos120.x, 0.0f, -pos120.y),  vec3<F32>(-pos120.x, 1.0f, -pos120.y),  vec3<F32>(pos120.x, 1.0f, pos120.y), vec3<F32>(pos120.x, 0.0f, pos120.y),
            vec3<F32>(-pos240.x, 0.0f, -pos240.y),  vec3<F32>(-pos240.x, 1.0f, -pos240.y),  vec3<F32>(pos240.x, 1.0f, pos240.y), vec3<F32>(pos240.x, 0.0f, pos240.y)
        };

        const size_t billboardsPlaneCount = sizeof(vertices) / (sizeof(vec3<F32>) * 4);

        const U16 indices[] = { 0, 1, 2, 0, 2, 3,
                                2, 1, 0, 3, 2, 0 };

        const vec2<F32> texcoords[] = {
            vec2<F32>(0.0f, 0.0f),
            vec2<F32>(0.0f, 1.0f),
            vec2<F32>(1.0f, 1.0f),
            vec2<F32>(1.0f, 0.0f)
        };

        s_buffer = gfxDevice.newVB();
        s_buffer->useLargeIndices(false);
        s_buffer->setVertexCount(billboardsPlaneCount * 4);
        for (U8 i = 0; i < billboardsPlaneCount * 4; ++i) {
            s_buffer->modifyPositionValue(i, vertices[i]);
            s_buffer->modifyTexCoordValue(i, texcoords[i % 4].s, texcoords[i % 4].t);
        }

        for (U8 i = 0; i < billboardsPlaneCount; ++i) {
            if (i > 0) {
                s_buffer->addRestartIndex();
            }
            for (U8 j = 0; j < (useDoubleSided ? 12 : 6); ++j) {
                s_buffer->addIndex(indices[j] + (i * 4));
            }

        }

        s_buffer->computeNormals();
        s_buffer->computeTangents();
        s_buffer->create(true);
        s_buffer->keepData(false);
    }

    //ref: http://mollyrocket.com/casey/stream_0016.html
    F32 PointRadius = 0.95f;
    const F32 ArBase = 1.0f; // Starting radius of circle A
    const F32 BrBase = 1.0f; // Starting radius of circle B
    F32 dR = 2.5f * PointRadius; // Distance between concentric rings

    s_grassPositions.reserve(to_size(chunkSize) * chunkSize);
    s_treePositions.reserve(to_size(chunkSize) * chunkSize);

    const F32 posOffset = to_F32(chunkSize * 2);

    vec2<F32> intersections[2];
    Util::Circle circleA, circleB;
    circleA.center[0] = circleB.center[0] = -posOffset;
    circleA.center[1] = -posOffset;
    circleB.center[1] = posOffset;

    for (I16 RadiusStepA = 0; RadiusStepA < g_maxRadiusSteps; ++RadiusStepA) {
        const F32 Ar = ArBase + dR * (F32)RadiusStepA;
        for (I16 RadiusStepB = 0; RadiusStepB < g_maxRadiusSteps; ++RadiusStepB) {
            const F32 Br = BrBase + dR * (F32)RadiusStepB;
            circleA.radius = Ar + ((RadiusStepB % 3) ? 0.0f : 0.3f * dR);
            circleB.radius = Br + ((RadiusStepA % 3) ? 0.0f : 0.3f * dR);
            // Intersect circle Ac,UseAr and Bc,UseBr
            if (Util::IntersectCircles(circleA, circleB, intersections)) {
                // Add the resulting points if they are within the pattern bounds
                for (U8 i = 0; i < 2; ++i) {
                    if (IS_IN_RANGE_EXCLUSIVE(intersections[i].x, -to_F32(chunkSize), to_F32(chunkSize)) &&
                        IS_IN_RANGE_EXCLUSIVE(intersections[i].y, -to_F32(chunkSize), to_F32(chunkSize)))
                    {
                        s_grassPositions.insert(intersections[i]);
                    }
                }
            }
        }
    }

    PointRadius = 5.0f;
    dR = 2.5f * PointRadius; // Distance between concentric rings

    for (I16 RadiusStepA = 0; RadiusStepA < g_maxRadiusSteps; ++RadiusStepA) {
        const F32 Ar = ArBase + dR * (F32)RadiusStepA;
        for (I16 RadiusStepB = 0; RadiusStepB < g_maxRadiusSteps; ++RadiusStepB) {
            const F32 Br = BrBase + dR * (F32)RadiusStepB;
            circleA.radius = Ar + ((RadiusStepB % 3) ? 0.0f : 0.3f * dR);
            circleB.radius = Br + ((RadiusStepA % 3) ? 0.0f : 0.3f * dR);
            // Intersect circle Ac,UseAr and Bc,UseBr
            if (Util::IntersectCircles(circleA, circleB, intersections)) {
                // Add the resulting points if they are within the pattern bounds
                for (U8 i = 0; i < 2; ++i) {
                    if (IS_IN_RANGE_EXCLUSIVE(intersections[i].x, -to_F32(chunkSize), to_F32(chunkSize)) &&
                        IS_IN_RANGE_EXCLUSIVE(intersections[i].y, -to_F32(chunkSize), to_F32(chunkSize)))
                    {
                        s_treePositions.insert(intersections[i]);
                    }
                }
            }
        }
    }

    s_maxChunks = maxChunkCount;
}

void Vegetation::createVegetationMaterial(GFXDevice& gfxDevice, const Terrain_ptr& terrain, const VegetationDetails& vegDetails) {
    if (vegDetails.billboardCount == 0) {
        return;
    }

    assert(s_maxGrassInstances != 0u && "Vegetation error: call \"precomputeStaticData\" first!");

    std::atomic_uint loadTasks = 0u;
    Material::ShaderData treeShaderData = {};
    treeShaderData._depthShaderVertSource = "tree";
    treeShaderData._depthShaderVertVariant = "";
    treeShaderData._colourShaderVertSource = "tree";
    treeShaderData._colourShaderVertVariant = "";

    ResourceDescriptor matDesc("Tree_material");
    s_treeMaterial = CreateResource<Material>(gfxDevice.parent().resourceCache(), matDesc);
    s_treeMaterial->setShadingMode(ShadingMode::BLINN_PHONG);
    s_treeMaterial->baseShaderData(treeShaderData);
    s_treeMaterial->addShaderDefine(ShaderType::VERTEX, "USE_CULL_DISTANCE", true);
    s_treeMaterial->addShaderDefine(ShaderType::COUNT, "OVERRIDE_DATA_IDX", true);
    s_treeMaterial->addShaderDefine(ShaderType::COUNT, Util::StringFormat("MAX_TREE_INSTANCES %d", s_maxTreeInstances).c_str(), true);

    SamplerDescriptor grassSampler = {};
    grassSampler.wrapU(TextureWrap::CLAMP_TO_EDGE);
    grassSampler.wrapV(TextureWrap::CLAMP_TO_EDGE);
    grassSampler.wrapW(TextureWrap::CLAMP_TO_EDGE);
    grassSampler.anisotropyLevel(8);

    TextureDescriptor grassTexDescriptor(TextureType::TEXTURE_2D_ARRAY);
    grassTexDescriptor.layerCount(vegDetails.billboardCount);
    grassTexDescriptor.samplerDescriptor(grassSampler);
    grassTexDescriptor.srgb(true);
    grassTexDescriptor.autoMipMaps(true);

    ResourceDescriptor textureDetailMaps("Vegetation Billboards");
    textureDetailMaps.assetLocation(Paths::g_assetsLocation + terrain->descriptor()->getVariable("vegetationTextureLocation"));
    textureDetailMaps.assetName(vegDetails.billboardTextureArray);
    textureDetailMaps.propertyDescriptor(grassTexDescriptor);
    textureDetailMaps.waitForReady(false);
    Texture_ptr grassBillboardArray = CreateResource<Texture>(terrain->parentResourceCache(), textureDetailMaps, loadTasks);

    ResourceDescriptor vegetationMaterial("grassMaterial");
    Material_ptr vegMaterial = CreateResource<Material>(terrain->parentResourceCache(), vegetationMaterial);
    vegMaterial->setShadingMode(ShadingMode::BLINN_PHONG);
    vegMaterial->getColourData().baseColour(DefaultColours::WHITE);
    vegMaterial->getColourData().specular(FColour3(0.1f, 0.1f, 0.1f));
    vegMaterial->getColourData().shininess(5.0f);
    vegMaterial->setDoubleSided(false);
    vegMaterial->setStatic(false);

    Material::ApplyDefaultStateBlocks(*vegMaterial);

    ShaderModuleDescriptor vertModule = {};
    vertModule._batchSameFile = false;
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "grass.glsl";

    vertModule._defines.emplace_back("USE_CULL_DISTANCE", true);
    vertModule._defines.emplace_back(Util::StringFormat("MAX_GRASS_INSTANCES %d", s_maxGrassInstances).c_str(), true);
    vertModule._defines.emplace_back("OVERRIDE_DATA_IDX", true);
    vertModule._defines.emplace_back("NODE_DYNAMIC", true);

    ShaderModuleDescriptor fragModule = {};
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._sourceFile = "grass.glsl";
    fragModule._defines.emplace_back("SKIP_TEXTURES", true);
    fragModule._defines.emplace_back(Util::StringFormat("MAX_GRASS_INSTANCES %d", s_maxGrassInstances).c_str(), true);
    fragModule._defines.emplace_back("USE_DOUBLE_SIDED", true);
    fragModule._defines.emplace_back("OVERRIDE_DATA_IDX", true);
    fragModule._defines.emplace_back("NODE_DYNAMIC", true);
    if (!gfxDevice.context().config().rendering.shadowMapping.enabled) {
        fragModule._defines.emplace_back("DISABLE_SHADOW_MAPPING", true);
    }
    fragModule._variant = "Colour";

    ShaderProgramDescriptor shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor grassColourShader("GrassColour");
    grassColourShader.propertyDescriptor(shaderDescriptor);
    grassColourShader.waitForReady(true);
    ShaderProgram_ptr grassColour = CreateResource<ShaderProgram>(terrain->parentResourceCache(), grassColourShader, loadTasks);

    ShaderProgramDescriptor shaderOitDescriptor = shaderDescriptor;
    shaderOitDescriptor._modules.back()._defines.emplace_back("OIT_PASS", true);
    //shaderOitDescriptor._modules.back()._defines.emplace_back("USE_SSAO", true);
    shaderOitDescriptor._modules.back()._variant = "Colour.OIT";
    
    ResourceDescriptor grassColourOITShader("grassColourOIT");
    grassColourOITShader.propertyDescriptor(shaderOitDescriptor);
    grassColourOITShader.waitForReady(false);
    ShaderProgram_ptr grassColourOIT = CreateResource<ShaderProgram>(terrain->parentResourceCache(), grassColourOITShader, loadTasks);

    fragModule._variant = "PrePass";
    shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor grassPrePassShader("grassPrePass");
    grassPrePassShader.propertyDescriptor(shaderDescriptor);
    grassPrePassShader.waitForReady(false);
    ShaderProgram_ptr grassPrePass = CreateResource<ShaderProgram>(terrain->parentResourceCache(), grassPrePassShader, loadTasks);

    fragModule._variant = "Shadow";
    shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor grassShadowShader("grassShadow");
    grassShadowShader.propertyDescriptor(shaderDescriptor);
    grassShadowShader.waitForReady(false);
    ShaderProgram_ptr grassShadow = CreateResource<ShaderProgram>(terrain->parentResourceCache(), grassShadowShader, loadTasks);

    fragModule._variant = "Shadow.VSM";
    shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);
    ShaderProgram_ptr grassShadowVSM = CreateResource<ShaderProgram>(terrain->parentResourceCache(), grassShadowShader, loadTasks);

    ShaderModuleDescriptor compModule = {};
    compModule._moduleType = ShaderType::COMPUTE;
    compModule._sourceFile = "instanceCullVegetation.glsl";
    compModule._defines.emplace_back(Util::StringFormat("WORK_GROUP_SIZE %d", WORK_GROUP_SIZE), true);
    compModule._defines.emplace_back(Util::StringFormat("MAX_TREE_INSTANCES %d", s_maxTreeInstances).c_str(), true);
    compModule._defines.emplace_back(Util::StringFormat("MAX_GRASS_INSTANCES %d", s_maxGrassInstances).c_str(), true);
    switch (GetHiZMethod()) {
        case HiZMethod::ARM:
            compModule._defines.emplace_back("USE_ARM", true);
            break;
        case HiZMethod::NVIDIA:
            compModule._defines.emplace_back("USE_NVIDIA", true);
            break;
        default:
        case HiZMethod::RASTER_GRID:
            compModule._defines.emplace_back("USE_RASTERGRID", true);
            break;
    };

    ShaderProgramDescriptor shaderCompDescriptor = {};
    shaderCompDescriptor._modules.push_back(compModule);

    ResourceDescriptor instanceCullShaderGrass("instanceCullVegetation_Grass");
    instanceCullShaderGrass.waitForReady(false);
    instanceCullShaderGrass.propertyDescriptor(shaderCompDescriptor);
    s_cullShaderGrass = CreateResource<ShaderProgram>(terrain->parentResourceCache(), instanceCullShaderGrass, loadTasks);

    compModule._defines.emplace_back("CULL_TREES", true);
    shaderCompDescriptor = {};
    shaderCompDescriptor._modules.push_back(compModule);

    ResourceDescriptor instanceCullShaderTrees("instanceCullVegetation_Trees");
    instanceCullShaderTrees.waitForReady(false);
    instanceCullShaderTrees.propertyDescriptor(shaderCompDescriptor);
    s_cullShaderTrees = CreateResource<ShaderProgram>(terrain->parentResourceCache(), instanceCullShaderTrees, loadTasks);

    WAIT_FOR_CONDITION(loadTasks.load() == 0u);

    vegMaterial->setShaderProgram(grassColour, RenderStage::COUNT, RenderPassType::COUNT, 0u);
    vegMaterial->setShaderProgram(grassColourOIT, RenderStage::COUNT, RenderPassType::OIT_PASS, 0u);
    vegMaterial->setShaderProgram(grassPrePass, RenderStage::COUNT, RenderPassType::PRE_PASS, 0u);
    vegMaterial->setShaderProgram(grassShadow, RenderStage::SHADOW, RenderPassType::COUNT, to_U8(LightType::POINT));
    vegMaterial->setShaderProgram(grassShadow, RenderStage::SHADOW, RenderPassType::COUNT, to_U8(LightType::SPOT));
    vegMaterial->setShaderProgram(grassShadowVSM, RenderStage::SHADOW, RenderPassType::MAIN_PASS, to_U8(LightType::DIRECTIONAL));

    vegMaterial->setTexture(TextureUsage::UNIT0, grassBillboardArray);
    s_vegetationMaterial = vegMaterial;
}

void Vegetation::createAndUploadGPUData(GFXDevice& gfxDevice, const Terrain_ptr& terrain, const VegetationDetails& vegDetails) {
    assert(s_grassData == nullptr);
    for (TerrainChunk* chunk : terrain->terrainChunks()) {
        chunk->initializeVegetation(vegDetails);
    }
    terrain->getVegetationStats(s_maxGrassInstances, s_maxTreeInstances);

    s_maxGrassInstances += s_maxGrassInstances % WORK_GROUP_SIZE;
    s_maxTreeInstances += s_maxTreeInstances % WORK_GROUP_SIZE;

    if (s_maxTreeInstances > 0 || s_maxGrassInstances > 0) {
        ShaderBufferDescriptor bufferDescriptor = {};
        bufferDescriptor._usage = ShaderBuffer::Usage::UNBOUND_BUFFER;
        bufferDescriptor._elementSize = sizeof(VegetationData);
        bufferDescriptor._updateFrequency = BufferUpdateFrequency::OCASSIONAL;
        bufferDescriptor._updateUsage = BufferUpdateUsage::CPU_W_GPU_R;
        bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::NO_SYNC);

        if (s_maxTreeInstances > 0) {
            bufferDescriptor._elementCount = to_U32(s_maxTreeInstances * s_maxChunks);
            bufferDescriptor._name = "Tree_data";
            s_treeData = gfxDevice.newSB(bufferDescriptor);
        }
        if (s_maxGrassInstances > 0) {
            bufferDescriptor._elementCount = to_U32(s_maxGrassInstances * s_maxChunks);
            bufferDescriptor._name = "Grass_data";
            s_grassData = gfxDevice.newSB(bufferDescriptor);
        }

        createVegetationMaterial(gfxDevice, terrain, vegDetails);
    }
}

void Vegetation::uploadVegetationData(SceneGraphNode& sgn) {
    OPTICK_EVENT();

    assert(s_buffer != nullptr);
    Wait(*_buildTask);

    bool hasVegetation = false;
    if (_instanceCountGrass > 0u) {
        hasVegetation = true;
        if (_terrainChunk.ID() < s_maxChunks) {
            s_grassData->writeData(_terrainChunk.ID() * s_maxGrassInstances, _instanceCountGrass, (bufferPtr)_tempGrassData.data());
            renderState().drawState(true);
        } else {
            Console::errorfn("Vegetation::uploadGrassData: insufficient buffer space for grass data");
        }
        _tempGrassData.clear();
    }

    if (_instanceCountTrees > 0u) {
        hasVegetation = true;

        if (_terrainChunk.ID() < s_maxChunks) {
            s_treeData->writeData(_terrainChunk.ID() * s_maxTreeInstances, _instanceCountTrees, (bufferPtr)_tempTreeData.data());
            renderState().drawState(true);
        } else {
            Console::errorfn("Vegetation::uploadGrassData: insufficient buffer space for tree data");
        }
        _tempTreeData.clear();
    }

    if (hasVegetation) {
        sgn.get<RenderingComponent>()->setMaterialTpl(s_vegetationMaterial);
        sgn.get<RenderingComponent>()->lockLoD(0u);
        WAIT_FOR_CONDITION(s_cullShaderGrass->getState() == ResourceState::RES_LOADED &&
                           s_cullShaderTrees->getState() == ResourceState::RES_LOADED);

        PipelineDescriptor pipeDesc;
        pipeDesc._shaderProgramHandle = s_cullShaderGrass->getGUID();
        _cullPipelineGrass = _context.newPipeline(pipeDesc);
        pipeDesc._shaderProgramHandle = s_cullShaderTrees->getGUID();
        _cullPipelineTrees = _context.newPipeline(pipeDesc);
    }

    const U32 ID = _terrainChunk.ID();
    const U32 meshID = to_U32(ID % _treeMeshNames.size());

    if (_instanceCountTrees > 0 && !_treeMeshNames.empty()) {
        {
            UniqueLock<SharedMutex> w_lock(g_treeMeshLock);
            if (s_treeMeshes.empty()) {
                for (const stringImpl& meshName : _treeMeshNames) {
                    if (eastl::find_if(eastl::cbegin(s_treeMeshes), eastl::cend(s_treeMeshes),
                        [&meshName](const Mesh_ptr& ptr) {
                        return Util::CompareIgnoreCase(ptr->assetName(), meshName);
                    }) == eastl::cend(s_treeMeshes))
                    {
                        ResourceDescriptor model("Tree");
                        model.assetLocation(Paths::g_assetsLocation + "models");
                        model.flag(true);
                        model.threaded(false); //< we need the extents asap!
                        model.assetName(meshName);
                        Mesh_ptr meshPtr = CreateResource<Mesh>(_context.parent().resourceCache(), model);
                        meshPtr->setMaterialTpl(s_treeMaterial);
                        // CSM last split should probably avoid rendering trees since it would cover most of the scene :/
                        meshPtr->renderState().addToDrawExclusionMask(RenderStage::SHADOW, RenderPassType::MAIN_PASS, to_U8(LightType::DIRECTIONAL), 2u);
                        for (const SubMesh_ptr& subMesh : meshPtr->subMeshList()) {
                            subMesh->renderState().addToDrawExclusionMask(RenderStage::SHADOW, RenderPassType::MAIN_PASS, to_U8(LightType::DIRECTIONAL), 2u);
                        }
                        s_treeMeshes.push_back(meshPtr);
                    }
                }
            }
        }

        Mesh_ptr crtMesh = nullptr;
        {
            SharedLock<SharedMutex> r_lock(g_treeMeshLock);
            crtMesh = s_treeMeshes.front();
            const stringImpl& meshName = _treeMeshNames[meshID];
            for (const Mesh_ptr& mesh : s_treeMeshes) {
                if (Util::CompareIgnoreCase(mesh->assetName(), meshName)) {
                    crtMesh = mesh;
                    break;
                }
            }
        }
        constexpr U32 normalMask = to_base(ComponentType::TRANSFORM) |
                                   to_base(ComponentType::BOUNDS) |
                                   to_base(ComponentType::NETWORKING) |
                                   to_base(ComponentType::RENDERING);

        SceneGraphNodeDescriptor nodeDescriptor = {};
        nodeDescriptor._componentMask = normalMask;
        nodeDescriptor._usageContext = NodeUsageContext::NODE_STATIC;
        nodeDescriptor._serialize = false;
        nodeDescriptor._instanceCount = _instanceCountTrees;
        nodeDescriptor._node = crtMesh;
        nodeDescriptor._name = Util::StringFormat("Trees_chunk_%d", ID);
        _treeParentNode = sgn.addChildNode(nodeDescriptor);
        _treeParentNode->setFlag(SceneGraphNode::Flags::VISIBILITY_LOCKED);

        TransformComponent* tComp = _treeParentNode->get<TransformComponent>();
        const vec4<F32>& offset = _terrainChunk.getOffsetAndSize();
        tComp->setPositionX(offset.x + offset.z * 0.5f);
        tComp->setPositionZ(offset.y + offset.w * 0.5f);
        tComp->setScale(_treeScales[meshID]);

        _treeParentNode->forEachChild([ID](SceneGraphNode* child, I32 /*childIdx*/) {
            RenderingComponent* rComp = child->get<RenderingComponent>();
            // negative value to disable occlusion culling
            rComp->cullFlagValue(-1.0f * ID);
            return true;
        });

        const vec3<F32>& extents = _treeParentNode->get<BoundsComponent>()->updateAndGetBoundingBox().getExtent();
        _treeExtents.set(extents, 0);
        _cullPushConstants.set(_ID("treeExtents"), GFX::PushConstantType::VEC4, _treeExtents);
    }

    _cullPushConstants.set(_ID("dvd_terrainChunkOffset"), GFX::PushConstantType::UINT, ID);
    _cullPushConstants.set(_ID("grassExtents"), GFX::PushConstantType::VEC4, _grassExtents);

    setState(ResourceState::RES_LOADED);
}

void Vegetation::getStats(U32& maxGrassInstances, U32& maxTreeInstances) const {
    Wait(*_buildTask);

    maxGrassInstances = _instanceCountGrass;
    maxTreeInstances = _instanceCountTrees;
}

void Vegetation::sceneUpdate(const U64 deltaTimeUS,
                             SceneGraphNode& sgn,
                             SceneState& sceneState) {
    OPTICK_EVENT();

    if (!renderState().drawState()) {
        uploadVegetationData(sgn);
        // positive value to keep occlusion culling happening
        sgn.get<RenderingComponent>()->cullFlagValue(1.0f * _terrainChunk.ID());
    }

    if (!s_buffersBound) {
        if (s_treeData != nullptr) {
            s_treeData->bind(ShaderBufferLocation::TREE_DATA);
        }
        if (s_grassData != nullptr) {
            s_grassData->bind(ShaderBufferLocation::GRASS_DATA);
        }
        s_treePositions.clear();
        s_grassPositions.clear();
        s_buffersBound = true;
    }

    if (renderState().drawState()) {
        assert(getState() == ResourceState::RES_LOADED);
        // Query shadow state every "_stateRefreshInterval" microseconds
        if (_stateRefreshIntervalBufferUS >= _stateRefreshIntervalUS) {
            _windX = sceneState.windDirX();
            _windZ = sceneState.windDirZ();
            _windS = sceneState.windSpeed();
            _stateRefreshIntervalBufferUS -= _stateRefreshIntervalUS;
        }
        _stateRefreshIntervalBufferUS += deltaTimeUS;

        const SceneRenderState& renderState = sceneState.renderState();

        const F32 sceneRenderRange = renderState.generalVisibility();
        const F32 sceneGrassDistance = std::min(renderState.grassVisibility(), sceneRenderRange);
        const F32 sceneTreeDistance = std::min(renderState.treeVisibility(), sceneRenderRange);
        if (sceneGrassDistance != _grassDistance) {
            _grassDistance = sceneGrassDistance;
            _cullPushConstants.set(_ID("dvd_grassVisibilityDistance"), GFX::PushConstantType::FLOAT, _grassDistance);
            sgn.get<RenderingComponent>()->setRenderRange(-_grassDistance, _grassDistance);
        }
        if (sceneTreeDistance != _treeDistance) {
            _treeDistance = sceneTreeDistance;
            _cullPushConstants.set(_ID("dvd_treeVisibilityDistance"), GFX::PushConstantType::FLOAT, _treeDistance);
            if (_treeParentNode != nullptr) {
                _treeParentNode->forEachChild([sceneTreeDistance](SceneGraphNode* child, I32 /*childIdx*/) {
                    RenderingComponent* rComp = child->get<RenderingComponent>();
                    // negative value to disable occlusion culling
                    rComp->setRenderRange(-sceneTreeDistance, sceneTreeDistance);
                    return true;
                });
            }
        }
    }

    SceneNode::sceneUpdate(deltaTimeUS, sgn, sceneState);
}


void Vegetation::buildDrawCommands(SceneGraphNode& sgn,
                                   const RenderStagePass& renderStagePass,
                                   const Camera& crtCamera,
                                   RenderPackage& pkgInOut) {

    GenericDrawCommand cmd = {};
    cmd._primitiveType = PrimitiveType::TRIANGLE_STRIP;
    cmd._cmd.indexCount = to_U32(s_buffer->getIndexCount());
    cmd._sourceBuffer = s_buffer->handle();
    cmd._cmd.primCount = _instanceCountGrass;

    pkgInOut.add(GFX::DrawCommand{ cmd });

    SceneNode::buildDrawCommands(sgn, renderStagePass, crtCamera, pkgInOut);
}

void Vegetation::onRefreshNodeData(const SceneGraphNode& sgn, const RenderStagePass& renderStagePass, const Camera& crtCamera, bool refreshData, GFX::CommandBuffer& bufferInOut) {
    // Culling lags one full frame
    if (renderStagePass._stage == RenderStage::DISPLAY && refreshData && _instanceCountGrass > 0 || _instanceCountTrees > 0)
    {
        // This will always lag one frame
        const Texture_ptr& depthTex = _context.renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::HI_Z)).getAttachment(RTAttachmentType::Depth, 0).texture();

        GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
        descriptorSetCmd._set._textureData.setTexture(depthTex->data(), to_U8(TextureUsage::UNIT0));
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        GFX::SendPushConstantsCommand cullConstants(_cullPushConstants);
        cullConstants._constants.countHint(5 + _cullPushConstants.data().size());
        cullConstants._constants.set(_ID("viewportDimensions"), GFX::PushConstantType::VEC2, vec2<F32>(depthTex->width(), depthTex->height()));
        cullConstants._constants.set(_ID("projectionMatrix"), GFX::PushConstantType::MAT4, crtCamera.getProjectionMatrix());
        cullConstants._constants.set(_ID("viewMatrix"), GFX::PushConstantType::MAT4, mat4<F32>::Multiply(crtCamera.getViewMatrix(), crtCamera.getViewMatrix()));
        cullConstants._constants.set(_ID("viewProjectionMatrix"), GFX::PushConstantType::MAT4, mat4<F32>::Multiply(crtCamera.getViewMatrix(), crtCamera.getProjectionMatrix()));
        cullConstants._constants.set(_ID("cameraPosition"), GFX::PushConstantType::VEC3, crtCamera.getEye());
        cullConstants._constants.set(_ID("dvd_nearPlaneDistance"), GFX::PushConstantType::FLOAT, crtCamera.getZPlanes().x);
        GFX::DispatchComputeCommand computeCmd = {};

        if (_instanceCountGrass > 0) {
            computeCmd._computeGroupSize.set(std::max(_instanceCountGrass, _instanceCountGrass / WORK_GROUP_SIZE), 1, 1);

            //Cull grass
            GFX::EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ _cullPipelineGrass });
            GFX::EnqueueCommand(bufferInOut, cullConstants);
            GFX::EnqueueCommand(bufferInOut, computeCmd);
        }

        if (_instanceCountTrees > 0) {
            computeCmd._computeGroupSize.set(std::max(_instanceCountTrees, _instanceCountTrees / WORK_GROUP_SIZE), 1, 1);

            // Cull trees
            GFX::EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ _cullPipelineTrees });
            GFX::EnqueueCommand(bufferInOut, cullConstants);
            GFX::EnqueueCommand(bufferInOut, computeCmd);
        }

        GFX::MemoryBarrierCommand memCmd = {};
        memCmd._barrierMask = to_base(MemoryBarrierType::SHADER_STORAGE) | to_base(MemoryBarrierType::BUFFER_UPDATE);
        GFX::EnqueueCommand(bufferInOut, memCmd);
    }

    return SceneNode::onRefreshNodeData(sgn, renderStagePass, crtCamera, refreshData, bufferInOut);
}

namespace {
    FORCE_INLINE U8 bestIndex(const UColour4& in) {
        U8 maxValue = 0;
        U8 bestIndex = 0;
        for (U8 i = 0; i < 4; ++i) {
            if (in[i] > maxValue) {
                maxValue = in[i];
                bestIndex = i;
            }
        }

        return bestIndex;
    }

    FORCE_INLINE bool ScaleAndCheckBounds(const vec2<F32>& chunkPos, const vec2<F32>& chunkSize, vec2<F32>& point) {
        if (point.x > -chunkSize.x && point.x < chunkSize.x && 
            point.y > -chunkSize.y && point.y < chunkSize.y) 
        {
            // [-chunkSize * 0.5f, chunkSize * 0.5f] to [0, chunkSize]
            point = (point + chunkSize) * 0.5f;
            point += chunkPos;
            return true;
        }

        return false;
    }
};

void Vegetation::computeVegetationTransforms(const Task& parentTask, bool treeData) {
    // Grass disabled
    if (!treeData && !_context.context().config().debug.enableGrassInstances) {
        return;
    }
    // Trees disabled
    if (treeData && !_context.context().config().debug.enableTreeInstances) {
        return;
    }

    const Terrain& terrain = _terrainChunk.parent();
    const U32 ID = _terrainChunk.ID();

    const stringImpl cacheFileName = Util::StringFormat("%s_%s_%s_%d.cache", terrain.resourceName().c_str(), resourceName().c_str(), (treeData ? "trees" : "grass"), ID);
    Console::printfn(Locale::get(treeData ? _ID("CREATE_TREE_START") : _ID("CREATE_GRASS_BEGIN")), ID);

    vectorEASTL<VegetationData>& container = treeData ? _tempTreeData : _tempGrassData;

    ByteBuffer chunkCache;
    if (_context.context().config().debug.useVegetationCache && chunkCache.loadFromFile((Paths::g_cacheLocation + Paths::g_terrainCacheLocation).c_str(), cacheFileName.c_str())) {
        container.resize(chunkCache.read<size_t>());
        chunkCache.read(reinterpret_cast<Byte*>(container.data()), sizeof(VegetationData) * container.size());
    } else {
        const U32 meshID = to_U32(ID % _treeMeshNames.size());

        const vec2<F32>& chunkSize = _terrainChunk.getOffsetAndSize().zw();
        const vec2<F32>& chunkPos = _terrainChunk.getOffsetAndSize().xy();
        const F32 waterLevel = 0.0f;// ToDo: make this dynamic! (cull underwater points later on?)
        auto map = treeData ? _treeMap : _grassMap;
        const U16 mapWidth = map->dimensions().width;
        const U16 mapHeight = map->dimensions().height;
        const auto& positions = treeData ? s_treePositions : s_grassPositions;
        const auto& scales = treeData ? _treeScales : _grassScales;
        const F32 slopeLimit = treeData ? 10.0f : 35.0f;

        for (vec2<F32> pos : positions) {
            if (!ScaleAndCheckBounds(chunkPos, chunkSize, pos)) {
                continue;
            }

            if (parentTask._stopRequested) {
                goto end;
            }

            const vec2<F32> mapCoord(pos.x + mapWidth * 0.5f, pos.y + mapHeight * 0.5f);

            const F32 x_fac = mapCoord.x / mapWidth;
            const F32 y_fac = mapCoord.y / mapHeight;

            const Terrain::Vert vert = terrain.getVert(x_fac, y_fac, true);

            // terrain slope should be taken into account
            if (Angle::to_DEGREES(std::acos(Dot(vert._normal, WORLD_Y_AXIS))) > slopeLimit) {
                continue;
            }

            assert(vert._position != VECTOR3_ZERO);

            const UColour4 colour = map->getColour(to_I32(mapCoord.x), to_I32(mapCoord.y));
            const U8 index = bestIndex(colour);
            const F32 colourVal = colour[index];
            if (colourVal <= EPSILON_F32) {
                continue;
            }

            const F32 xmlScale = scales[treeData ? meshID : index];
            // Don't go under 75% of the scale specified in the data files
            const F32 minXmlScale = (xmlScale * 7.5f) / 10.0f;
            // Don't go over 75% of the cale specified in the data files
            const F32 maxXmlScale = (xmlScale * 1.25f);

            const F32 scale = CLAMPED(((colour[index] + 1) / 256.0f) * xmlScale, minXmlScale, maxXmlScale);

            assert(scale > EPSILON_F32);

            //vert._position.y = (((0.0f*heightExtent) + vert._position.y) - ((0.0f*scale) + vert._position.y)) + vert._position.y;
            VegetationData entry = {};
            entry._positionAndScale.set(vert._position, scale);
            Quaternion<F32> modelRotation;
            if (treeData) {
                modelRotation.fromEuler(_treeRotations[meshID]);
            } else {
                modelRotation = RotationFromVToU(WORLD_Y_AXIS, vert._normal, WORLD_Z_AXIS);
            }

            entry._orientationQuat = (Quaternion<F32>(WORLD_Y_AXIS, Random(360.0f)) * modelRotation).asVec4();
            entry._data = {
                to_F32(index),
                to_F32(ID),
                1.0f,
                1.0f
            };

            container.push_back(entry);
        }

        container.shrink_to_fit();
        chunkCache << container.size();
        chunkCache.append(container.data(), container.size());
        chunkCache.dumpToFile((Paths::g_cacheLocation + Paths::g_terrainCacheLocation).c_str(), cacheFileName.c_str());
    }
    
end:
    Console::printfn(Locale::get(_ID("CREATE_GRASS_END")));
}

};
