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
    constexpr bool g_disableLoadFromCache = true;
    static U8 g_billboardsPlaneCount = 4;
    static F32 g_grassDistance = 100.0f;
};

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


void Vegetation::uploadGrassData() {
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


        const U16 indices[] = { 0, 1, 2, 0, 2, 3, 
                                2, 1, 0, 3, 2, 0 };

        const vec2<F32> texcoords[] = {
            vec2<F32>(0.0f, 0.0f),
            vec2<F32>(0.0f, 1.0f),
            vec2<F32>(1.0f, 1.0f),
            vec2<F32>(1.0f, 0.0f)
        };

        s_buffer = _context.newVB();
        s_buffer->useLargeIndices(false);
        s_buffer->setVertexCount(g_billboardsPlaneCount * 4);
        for (U8 i = 0; i < g_billboardsPlaneCount * 4; ++i) {
            s_buffer->modifyPositionValue(i, vertices[i]);
            s_buffer->modifyTexCoordValue(i, texcoords[i % 4].s, texcoords[i % 4].t);
            s_buffer->modifyNormalValue(i , vec3<F32>(vertices[i].x, 0.0f, vertices[i].y));
        }

        for (U8 i = 0; i < g_billboardsPlaneCount; ++i) {
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
    s_bufferUsage.fetch_add(1);

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
    static const Task updateTask;

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
void Vegetation::onRefreshNodeData(SceneGraphNode& sgn,
                                   GFX::CommandBuffer& bufferInOut){
    if (_render) {
        constexpr U32 GROUP_SIZE_AABB = 64;

        PipelineDescriptor pipeDesc;
        pipeDesc._shaderProgramHandle = _cullShader->getID();

        Texture_ptr depthTex = _context.renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachment(RTAttachmentType::Depth, 0).texture();

        GFX::BindDescriptorSetsCommand descriptorSetCmd;
        descriptorSetCmd._set = _context.newDescriptorSet();
        descriptorSetCmd._set->_textureData.addTexture(depthTex->getData(), to_U8(ShaderProgram::TextureUsage::UNIT0));
        descriptorSetCmd._set->_shaderBuffers.emplace_back(ShaderBufferLocation::GRASS_DATA, _grassData, vec2<U32>(0, _grassData->getPrimitiveCount()));
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        GFX::BindPipelineCommand pipelineCmd;
        pipelineCmd._pipeline = _context.newPipeline(pipeDesc);
        GFX::EnqueueCommand(bufferInOut, pipelineCmd);

        GFX::SendPushConstantsCommand pushConstantsCommand;
        
        g_grassDistance = _context.parent().sceneManager().getActiveScene().renderState().grassVisibility();
        pushConstantsCommand._constants.set("dvd_visibilityDistance", GFX::PushConstantType::FLOAT, g_grassDistance);
        pushConstantsCommand._constants.set("cullType", GFX::PushConstantType::UINT, to_base(CullType::INSTANCE_CLOUD_REDUCTION));
        pushConstantsCommand._constants.set("instanceCount", GFX::PushConstantType::UINT, _instanceCountGrass);
        GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

        GFX::DispatchComputeCommand computeCmd;
        computeCmd._computeGroupSize.set((_instanceCountGrass + GROUP_SIZE_AABB - 1) / GROUP_SIZE_AABB, 1, 1);
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
        sgn.get<RenderingComponent>()->registerShaderBuffer(ShaderBufferLocation::GRASS_DATA,
                                                            vec2<U32>(0, _grassData->getPrimitiveCount()),
                                                            *_grassData);

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

        STUBBED("We really need blue noise or casey muratori's special circle/hex for a nice distribution. Use Poisson disk sampling and optimise from there? -Ionut");
        /*
        ref: https://github.com/corporateshark/poisson-disk-generator/blob/master/Poisson.cpp
        ref: http://yutingye.info/OtherProjects_files/report.pdf
        ref: http://mollyrocket.com/casey/stream_0016.html
        THIS:

        generate the first random point p0
        insert p0 into active list
        place the index of p0 (zero) to the background grid cell
        while active list is not empty do
            choose a random point p from active list
            for attempt = 1:maxAttemp
                get new sample p? around p between r and 2r
                for each non-empty neighbor cell around p?
                    if p? is closer than r
                        break
                    end if
                end for
                if p? is far from all neighbors
                    insert p? to active list
                    place the index of p? to background grid cell
                    break
                end if
            end for
            if maxAttemp exceed
                remove p from active list
            end if
        end while
        return

        OR THIS:
        real32 PointRadius = 0.025f;
        v2 Ac = { -2, -2 }; // Center of circle A
        real32 ArBase = 1.0f; // Starting radius of circle A
        v2 Bc = { -2, 2 }; // Center of circle B
        real32 BrBase = 1.0f; // Starting radius of circle B
        real32 dR = 2.5f*PointRadius; // Distance between concentric rings
        for (int32x RadiusStepA = 0; RadiusStepA < 128; ++RadiusStepA) {
            real32 Ar = ArBase + dR*(real32)RadiusStepA;
            for (int32x RadiusStepB = 0; RadiusStepB < 128; ++RadiusStepB) {
                real32 Br = BrBase + dR*(real32)RadiusStepB;
                real32 UseAr = Ar + ((RadiusStepB % 3) ? 0.0f : 0.3f*dR);
                real32 UseBr = Br + ((RadiusStepA % 3) ? 0.0f : 0.3f*dR);

                // Intersect circle Ac,UseAr and Bc,UseBr
                // Add the resulting points if they are within the pattern bounds
                // (the bounds were [-1,1] on both axes for all prior screenshots)
            }
        }
        */

        F32 densityFactor = 1.0f / _grassDensity;
        assert(_tempData.empty());
        _tempData.reserve(static_cast<size_t>(currentCount * chunkSize.x * chunkSize.y));

        for (F32 x = 0; x < chunkSize.x - densityFactor * 1.5f; x += Random(densityFactor * 0.25f, densityFactor * 1.5f)) {
            F32 width = x + chunkPos.x;
            for (F32 y = 0; y < chunkSize.y - densityFactor * 1.5f; y += Random(densityFactor * 0.25f, densityFactor * 1.5f)) {
                if (parentTask._stopRequested) {
                    goto end;
                }
                    
                F32 height = (y + chunkPos.y);

                F32 x_fac = width  / mapWidth;
                F32 y_fac = height / mapHeight;

                Terrain::Vert vert = terrain.getVert(x_fac, y_fac);
                if (vert._position.y < waterLevel) {
                    continue;
                }
                if (vert._normal.y < 0.7f) {
                    continue;
                }

                UColour colour = _map->getColour((U16)width, (U16)height);
                U8 index = bestIndex(colour);
                {
                    GrassData entry = {};
                    entry._data.set(1.0f, 1.0f, to_F32(index), 1.0f);
                    entry._transform.setScale(vec3<F32>(((colour[index] + 1) / 256.0f)));
                    mat3<F32> rotationMatrix = GetMatrix(Quaternion<F32>(WORLD_Y_AXIS, Random(360.0f)) * RotationFromVToU(WORLD_Y_AXIS, vert._normal));
                    entry._transform = mat4<F32>(rotationMatrix, false) * entry._transform;
                    entry._transform.setTranslation(vert._position);
                    _tempData.push_back(entry);
                }
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
