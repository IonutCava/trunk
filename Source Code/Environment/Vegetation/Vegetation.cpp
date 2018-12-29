#include "stdafx.h"

#include "Headers/Vegetation.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"
#include "Managers/Headers/SceneManager.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderPackage.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Geometry/Material/Headers/Material.h"
#include "Platform/Headers/PlatformRuntime.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainChunk.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"
#include "Platform/Video/Buffers/VertexBuffer/GenericBuffer/Headers/GenericVertexData.h"

#include "ECS/Components/Headers/RenderingComponent.h"

namespace Divide {

namespace {
    constexpr U32 WORK_GROUP_SIZE = 64;
    constexpr bool g_disableLoadFromCache = false;
    constexpr I16 g_maxRadiusSteps = 254;

};

vectorFast<vec2<F32>> Vegetation::s_grassPositions;
VertexBuffer* Vegetation::s_buffer = nullptr;
std::atomic_uint Vegetation::s_bufferUsage = 0;

Vegetation::Vegetation(GFXDevice& context, 
                       TerrainChunk& parentChunk,
                       const VegetationDetails& details)
    : SceneNode(context.parent().resourceCache(), parentChunk.parent().getDescriptorHash() + parentChunk.ID(), details.name + "_" + to_stringImpl(parentChunk.ID()), SceneNodeType::TYPE_VEGETATION),
      _context(context),
      _terrainChunk(parentChunk),
      _billboardCount(details.billboardCount),
      _grassDensity(details.grassDensity),
      _grassScale(details.grassScale),
      _terrain(details.parentTerrain),
      _render(false),
      _success(false),
      _shadowMapped(true),
      _grassData(nullptr),
      _instanceCountGrass(0),
      _stateRefreshIntervalBufferUS(0ULL),
      _stateRefreshIntervalUS(Time::SecondsToMicroseconds(1))  ///<Every second?
{
    _map = details.map;

    ResourceDescriptor instanceCullShader("instanceCullGrass");
    instanceCullShader.setThreadedLoading(true);

    ShaderProgramDescriptor shaderDescriptor = {};
    shaderDescriptor._defines.push_back(std::make_pair(Util::StringFormat("WORK_GROUP_SIZE %d", WORK_GROUP_SIZE), true));
    instanceCullShader.setPropertyDescriptor(shaderDescriptor);

    _cullShader = CreateResource<ShaderProgram>(context.parent().resourceCache(), instanceCullShader);

    assert(_map->data() != nullptr);

    setMaterialTpl(details.vegetationMaterialPtr);

    _boundingBox.set(parentChunk.bounds());

    CreateTask(_context.parent().platformContext(),
        [this](const Task& parentTask) {
            computeGrassTransforms(parentTask);
        }
        ).startTask(TaskPriority::REALTIME, [this]() {
            uploadGrassData();
        }
    );
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
    }

    Console::printfn(Locale::get(_ID("UNLOAD_VEGETATION_END")));
}

void Vegetation::precomputeStaticData(PlatformContext& context, U32 chunkSize) {
    // Make sure this is ONLY CALLED FROM THE MAIN LOADING THREAD. All instances should call this in a serialized fashion
    if (s_buffer == nullptr) {
        const vec2<F32> pos000(cosf(Angle::to_RADIANS(0.000f)), sinf(Angle::to_RADIANS(0.000f)));
        const vec2<F32> pos120(cosf(Angle::to_RADIANS(120.0f)), sinf(Angle::to_RADIANS(120.0f)));
        const vec2<F32> pos240(cosf(Angle::to_RADIANS(240.0f)), sinf(Angle::to_RADIANS(240.0f)));

        const vec3<F32> vertices[] = {
            vec3<F32>(-pos000.x, 0.0f, -pos000.y),  vec3<F32>(-pos000.x, 1.0f, -pos000.y),  vec3<F32>(pos000.x, 1.0f, pos000.y), vec3<F32>(pos000.x, 0.0f, pos000.y),
            vec3<F32>(-pos120.x, 0.0f, -pos120.y),	vec3<F32>(-pos120.x, 1.0f, -pos120.y),	vec3<F32>(pos120.x, 1.0f, pos120.y), vec3<F32>(pos120.x, 0.0f, pos120.y),
            vec3<F32>(-pos240.x, 0.0f, -pos240.y),	vec3<F32>(-pos240.x, 1.0f, -pos240.y),	vec3<F32>(pos240.x, 1.0f, pos240.y), vec3<F32>(pos240.x, 0.0f, pos240.y)
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

        s_buffer = context.gfx().newVB();
        s_buffer->useLargeIndices(false);
        s_buffer->setVertexCount(billboardsPlaneCount * 4);
        for (U8 i = 0; i < billboardsPlaneCount * 4; ++i) {
            s_buffer->modifyPositionValue(i, vertices[i]);
            s_buffer->modifyTexCoordValue(i, texcoords[i % 4].s, texcoords[i % 4].t);
            s_buffer->modifyNormalValue(i, vec3<F32>(vertices[i].x, 0.0f, vertices[i].y));
        }

        for (U8 i = 0; i < billboardsPlaneCount; ++i) {
            if (i > 0) {
                s_buffer->addRestartIndex();
            }
            for (U8 j = 0; j < 12; ++j) {
                s_buffer->addIndex(indices[j] + (i * 4));
            }

        }

        s_buffer->computeTangents();
        s_buffer->create(true);
        s_buffer->keepData(false);
    }

    /*if (!g_disableLoadFromCache) {
        return;
    }*/

    //ref: http://mollyrocket.com/casey/stream_0016.html
    F32 PointRadius = 1.0f;
    F32 ArBase = 1.0f; // Starting radius of circle A
    F32 BrBase = 1.0f; // Starting radius of circle B
    F32 dR = 2.5f*PointRadius; // Distance between concentric rings

    s_grassPositions.reserve(static_cast<size_t>(chunkSize * chunkSize));

    F32 posOffset = to_F32(chunkSize * 2);

    vec2<F32> intersections[2];
    Util::Circle circleA, circleB;
    circleA.center[0] = circleB.center[0] = -posOffset;
    circleA.center[1] = -posOffset;
    circleB.center[1] = posOffset;

#   pragma omp parallel for
    for (I16 RadiusStepA = 0; RadiusStepA < g_maxRadiusSteps; ++RadiusStepA) {
        F32 Ar = ArBase + dR * (F32)RadiusStepA;
        for (I16 RadiusStepB = 0; RadiusStepB < g_maxRadiusSteps; ++RadiusStepB) {
            F32 Br = BrBase + dR * (F32)RadiusStepB;
            circleA.radius = Ar + ((RadiusStepB % 3) ? 0.0f : 0.3f*dR);
            circleB.radius = Br + ((RadiusStepA % 3) ? 0.0f : 0.3f*dR);
            // Intersect circle Ac,UseAr and Bc,UseBr
            if (Util::IntersectCircles(circleA, circleB, intersections)) {
                // Add the resulting points if they are within the pattern bounds
                for (U8 i = 0; i < 2; ++i) {
                    if (IS_IN_RANGE_EXCLUSIVE(intersections[i].x, chunkSize * -1.0f, chunkSize * 1.0f) &&
                        IS_IN_RANGE_EXCLUSIVE(intersections[i].y, chunkSize * -1.0f, chunkSize * 1.0f))
                    {
#                       pragma omp critical
                        s_grassPositions.push_back(intersections[i]);
                    }
                }
            }
        }
    }
}

void Vegetation::uploadGrassData() {
    assert(s_buffer != nullptr);

    s_bufferUsage.fetch_add(1);

    U32 dif = _tempData.size() % WORK_GROUP_SIZE;
    if (dif > 0) {
        _tempData.insert(eastl::end(_tempData), dif, GrassData{});
    }

    _instanceCountGrass = to_U32(_tempData.size());
    if (_instanceCountGrass > 0) {
        ShaderBufferDescriptor bufferDescriptor = {};
        bufferDescriptor._elementCount = _instanceCountGrass;
        bufferDescriptor._elementSize = sizeof(GrassData);
        bufferDescriptor._updateFrequency = BufferUpdateFrequency::ONCE;
        bufferDescriptor._initialData = (bufferPtr)_tempData.data();
        bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::UNBOUND_STORAGE);
        bufferDescriptor._name = Util::StringFormat("Grass_data_chunk_%d", _terrainChunk.ID());
    
        _grassData = _context.newSB(bufferDescriptor);
        _render = true;
        _tempData.clear();
    }

    setState(ResourceState::RES_LOADED);
}

void Vegetation::sceneUpdate(const U64 deltaTimeUS,
                             SceneGraphNode& sgn,
                             SceneState& sceneState) {
    if (!_success && getState() == ResourceState::RES_LOADED) {
        _success = true;
    }

    if (_render && _success) {
        // Query shadow state every "_stateRefreshInterval" microseconds
        if (_stateRefreshIntervalBufferUS >= _stateRefreshIntervalUS) {
            _windX = sceneState.windDirX();
            _windZ = sceneState.windDirZ();
            _windS = sceneState.windSpeed();
            _stateRefreshIntervalBufferUS -= _stateRefreshIntervalUS;
        }
        _stateRefreshIntervalBufferUS += deltaTimeUS;
    }

    SceneNode::sceneUpdate(deltaTimeUS, sgn, sceneState);
}

bool Vegetation::getDrawState(const SceneGraphNode& sgn, RenderStagePass renderStage) const {
    if (_grassData != nullptr && _terrainChunk.isInView()) {
        return SceneNode::getDrawState(sgn, renderStage);
    }

    return false;
}

void Vegetation::onRefreshNodeData(SceneGraphNode& sgn,
                                   GFX::CommandBuffer& bufferInOut){
    if (_render) {
        // This will always lag one frame
        PipelineDescriptor pipeDesc;
        pipeDesc._shaderProgramHandle = _cullShader->getID();

        Texture_ptr depthTex = _context.renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachment(RTAttachmentType::Depth, 0).texture();

        GFX::BindDescriptorSetsCommand descriptorSetCmd;
        descriptorSetCmd._set._textureData.addTexture(depthTex->getData(), to_U8(ShaderProgram::TextureUsage::UNIT0));
        descriptorSetCmd._set._shaderBuffers.emplace_back(ShaderBufferLocation::GRASS_DATA, _grassData);
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        GFX::BindPipelineCommand pipelineCmd;
        pipelineCmd._pipeline = _context.newPipeline(pipeDesc);
        GFX::EnqueueCommand(bufferInOut, pipelineCmd);

        GFX::SendPushConstantsCommand pushConstantsCommand;
        
        float grassDistance = _context.parent().sceneManager().getActiveScene().renderState().grassVisibility();
        pushConstantsCommand._constants.set("dvd_visibilityDistance", GFX::PushConstantType::FLOAT, grassDistance);
        GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

        GFX::DispatchComputeCommand computeCmd;
        computeCmd._computeGroupSize.set(_instanceCountGrass / WORK_GROUP_SIZE, 1, 1);
        GFX::EnqueueCommand(bufferInOut, computeCmd);

        GFX::MemoryBarrierCommand memCmd;
        memCmd._barrierMask = to_base(MemoryBarrierType::SHADER_BUFFER);
        GFX::EnqueueCommand(bufferInOut, memCmd);
    }

    SceneNode::onRefreshNodeData(sgn, bufferInOut);
}

void Vegetation::buildDrawCommands(SceneGraphNode& sgn,
                                   RenderStagePass renderStagePass,
                                   RenderPackage& pkgInOut) {
    if (_render) {
        DescriptorSet set = pkgInOut.descriptorSet(0);
        set.addShaderBuffer({ ShaderBufferLocation::GRASS_DATA, _grassData});
        pkgInOut.descriptorSet(0, set);

        GenericDrawCommand cmd;
        cmd._primitiveType = PrimitiveType::TRIANGLES;
        cmd._cmd.indexCount = s_buffer->getIndexCount();
        cmd._sourceBuffer = s_buffer;
        cmd._cmd.primCount = _instanceCountGrass;

        GFX::DrawCommand drawCommand;
        drawCommand._drawCommands.push_back(cmd);
        pkgInOut.addDrawCommand(drawCommand);
    }

    SceneNode::buildDrawCommands(sgn, renderStagePass, pkgInOut);
}

namespace {
    FORCE_INLINE U8 bestIndex(const UColour& in) {
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
        if (point.x > chunkSize.x * -1.0f && point.x < chunkSize.y * 1.0f && 
            point.y > chunkSize.y * -1.0f && point.y < chunkSize.y * 1.0f) 
        {
            // [-chunkSize * 0.5f, chunkSize * 0.5f] to [0, chunkSize]
            point = (point + chunkSize) * 0.5f;
            point += chunkPos;
            return true;
        }

        return false;
    }
};

void Vegetation::computeGrassTransforms(const Task& parentTask) {
    const Terrain& terrain = _terrainChunk.parent();

    stringImpl cacheFileName = terrain.resourceName() + "_" + resourceName() + ".cache";

    const vec2<F32>& chunkSize = _terrainChunk.getOffsetAndSize().zw();
    const U32 grassElements = to_U32(_grassDensity * chunkSize.x * chunkSize.y);
    Console::printfn(Locale::get(_ID("CREATE_GRASS_BEGIN")), grassElements);

    ByteBuffer chunkCache;
    if (!g_disableLoadFromCache && chunkCache.loadFromFile(Paths::g_cacheLocation + Paths::g_terrainCacheLocation, cacheFileName)) {
        _tempData.resize(chunkCache.read<size_t>());
        chunkCache.read(reinterpret_cast<Byte*>(_tempData.data()), sizeof(GrassData) * _tempData.size());
    } else {

        const vec2<F32>& chunkPos = _terrainChunk.getOffsetAndSize().xy();
        const F32 waterLevel = 0.0f;// ToDo: make this dynamic! (cull underwater points later on?)
        const I32 currentCount = std::min((I32)_billboardCount, 4);
        const U16 mapWidth = _map->dimensions().width;
        const U16 mapHeight = _map->dimensions().height;

        _tempData.reserve(s_grassPositions.size());
        for (vec2<F32> pos : s_grassPositions) {
            if (!ScaleAndCheckBounds(chunkPos, chunkSize, pos)) {
                continue;
            }

            if (parentTask._stopRequested) {
                goto end;
            }

            F32 x_fac = pos.x / mapWidth;
            F32 y_fac = pos.y / mapHeight;

            Terrain::Vert vert = terrain.getVert(x_fac, y_fac, true);

            // terrain slope should be taken into account
            if (std::acos(Dot(vert._normal, WORLD_Y_AXIS)) > 45.0f) {
                continue;
            }

            const F32 heightExtent = 1.0f;
            const F32 heightHalfExtent = heightExtent * 0.5f;
            UColour colour = _map->getColour((U16)pos.x, (U16)pos.y);
            U8 index = bestIndex(colour);
            
            F32 scale = (colour[index] + 1) / 256.0f;
            //vert._position.y = (((0.0f*heightExtent) + vert._position.y) - ((0.0f*scale) + vert._position.y)) + vert._position.y;
            {
                GrassData entry = {};
                entry._data.set(1.0f, heightExtent, to_F32(index), 1.0f);
                entry._transform.setScale(vec3<F32>(scale));
                mat3<F32> rotationMatrix = GetMatrix(Quaternion<F32>(WORLD_Y_AXIS, Random(360.0f)) * RotationFromVToU(vert._normal, WORLD_Y_AXIS));
                entry._transform = mat4<F32>(rotationMatrix, false) * entry._transform;
                entry._transform.setTranslation(vert._position);
                _tempData.push_back(entry);
            }
        }


        chunkCache << _tempData.size();
        chunkCache.append(_tempData.data(), _tempData.size());
        chunkCache.dumpToFile(Paths::g_cacheLocation + Paths::g_terrainCacheLocation, cacheFileName);
    }
    
end:
    Console::printfn(Locale::get(_ID("CREATE_GRASS_END")));
}
};
