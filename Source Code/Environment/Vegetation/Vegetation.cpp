#include "stdafx.h"

#include "Headers/Vegetation.h"

#include "Core/Headers/Kernel.h"
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
    static U8 g_billboardsPlaneCount = 1; /*3*/
    static F32 g_grassDistance = 100.0f;
    thread_local vectorEASTL<GrassData> g_tempData;
};

VertexBuffer*      Vegetation::s_buffer = nullptr;

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
      _grassStateBlockHash(0),
      _stateRefreshIntervalBufferUS(0ULL),
      _stateRefreshIntervalUS(Time::SecondsToMicroseconds(1))  ///<Every second?
{
    _threadedLoadComplete = false;
    _stopLoadingRequest = false;

    _map = details.map;

    ResourceDescriptor instanceCullShader("instanceCullGrass");
    instanceCullShader.setThreadedLoading(true);
    _cullShader = CreateResource<ShaderProgram>(context.parent().resourceCache(), instanceCullShader);

    assert(_map->data() != nullptr);

    RenderStateBlock transparentRenderState;
    transparentRenderState.setCullMode(CullMode::CW);
    _grassStateBlockHash = transparentRenderState.getHash();
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
    _stopLoadingRequest = true;
    U32 timer = 0;
    while (!_threadedLoadComplete) {
        // wait for the loading thread to finish first;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        timer += 10;
        if (timer > 3000) {
            break;
        }
    }
    assert(_threadedLoadComplete);
    Console::printfn(Locale::get(_ID("UNLOAD_VEGETATION_END")));
}


void Vegetation::uploadGrassData() {
    // Make sure this is ONLY CALLED FROM THE MAIN LOADING THREAD. All instances should call this in a serialized fashion
    if (s_buffer == nullptr) {
        const vec2<F32> pos000(cosf(Angle::to_RADIANS(0.000f)), sinf(Angle::to_RADIANS(0.000f)));
        const vec2<F32> pos120(cosf(Angle::to_RADIANS(120.0f)), sinf(Angle::to_RADIANS(120.0f)));
        const vec2<F32> pos240(cosf(Angle::to_RADIANS(240.0f)), sinf(Angle::to_RADIANS(240.0f)));

        const vec3<F32> vertices[] = {vec3<F32>(-1, 1, 0),
                                      vec3<F32>(1, 1, 0),
                                      vec3<F32>(-1, -1, 0),
                                      vec3<F32>(1, -1, 0)
                                    /*vec3<F32>(-pos000.x, 1.0f, -pos000.y),
                                      vec3<F32>( pos000.x, 1.0f,  pos000.y),
                                      vec3<F32>(-pos000.x, 0.0f, -pos000.y),
                                      vec3<F32>( pos000.x, 0.0f,  pos000.y),

                                      vec3<F32>(-pos120.x, 0.0f, -pos120.y),
                                      vec3<F32>(-pos120.x, 1.0f, -pos120.y),
                                      vec3<F32>( pos120.x, 1.0f,  pos120.y),
                                      vec3<F32>( pos120.x, 0.0f,  pos120.y),
            
                                      vec3<F32>(-pos240.x, 0.0f, -pos240.y),
                                      vec3<F32>(-pos240.x, 1.0f, -pos240.y),
                                      vec3<F32>( pos240.x, 1.0f,  pos240.y),
                                      vec3<F32>( pos240.x, 0.0f,  pos240.y)*/};

        const U16 indices[] = { 2, 0, 1, 1, 2, 3, 1, 0, 2, 2, 1, 3 };
        
        const vec2<F32> texcoords[] = {vec2<F32>(0.0f, 1.0f), // 0
                                       vec2<F32>(1.0f, 1.0f), // 1
                                       vec2<F32>(0.0f, 0.0f), // 2
                                       vec2<F32>(1.0f, 0.0f)}; //3

        s_buffer = _context.newVB();
        s_buffer->useLargeIndices(false);
        s_buffer->setVertexCount(g_billboardsPlaneCount * 4);
        for (U8 i = 0; i < g_billboardsPlaneCount * 4; ++i) {
            s_buffer->modifyPositionValue(i, vertices[i]);
            s_buffer->modifyTexCoordValue(i, texcoords[i % 4]);
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

        s_buffer->create(true);
        s_buffer->keepData(false);
    }

    _instanceCountGrass = to_U32(g_tempData.size());
    if (_instanceCountGrass > 0) {
        ShaderBufferDescriptor bufferDescriptor = {};
        bufferDescriptor._elementCount = _instanceCountGrass;
        bufferDescriptor._elementSize = sizeof(GrassData);
        bufferDescriptor._ringBufferLength = 1;
        bufferDescriptor._updateFrequency = BufferUpdateFrequency::ONCE;
        bufferDescriptor._initialData = (bufferPtr)g_tempData.data();
        bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::UNBOUND_STORAGE);
        bufferDescriptor._name = Util::StringFormat("Grass_data_chunk_%d", _terrainChunk.ID());
    
        _grassData = _context.newSB(bufferDescriptor);
        _render = true;
        g_tempData.clear();
    }
    _threadedLoadComplete = true;
    setState(ResourceState::RES_LOADED);
}

void Vegetation::sceneUpdate(const U64 deltaTimeUS,
                             SceneGraphNode& sgn,
                             SceneState& sceneState) {
    static const Task updateTask;

    if (_threadedLoadComplete && !_success) {
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
    if (_threadedLoadComplete) {
        constexpr U32 GROUP_SIZE_AABB = 64;

        PipelineDescriptor pipeDesc;
        pipeDesc._shaderProgramHandle = _cullShader->getID();

        Texture_ptr depthTex = _context.renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachment(RTAttachmentType::Depth, 0).texture();

        GFX::BindDescriptorSetsCommand descriptorSetCmd;
        descriptorSetCmd._set = _context.newDescriptorSet();
        descriptorSetCmd._set->_textureData.addTexture(depthTex->getData(), to_U8(ShaderProgram::TextureUsage::UNIT0));
        descriptorSetCmd._set->_shaderBuffers.emplace_back(ShaderBufferLocation::GRASS_DATA, _grassData);
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        GFX::BindPipelineCommand pipelineCmd;
        pipelineCmd._pipeline = _context.newPipeline(pipeDesc);
        GFX::EnqueueCommand(bufferInOut, pipelineCmd);

        GFX::SendPushConstantsCommand pushConstantsCommand;
        
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
        cmd._primitiveType = PrimitiveType::TRIANGLE_STRIP;
        cmd._cmd.indexCount = s_buffer->getIndexCount();
        cmd._sourceBuffer = s_buffer;
        cmd._cmd.primCount = _instanceCountGrass;

        GFX::DrawCommand drawCommand;
        drawCommand._drawCommands.push_back(cmd);
        pkgInOut.addDrawCommand(drawCommand);

        const Pipeline* pipeline = pkgInOut.pipeline(0);
        PipelineDescriptor pipeDesc = pipeline->descriptor();
        pipeDesc._stateHash = _grassStateBlockHash;
        pkgInOut.pipeline(0, *_context.newPipeline(pipeDesc));
    }

    SceneNode::buildDrawCommands(sgn, renderStagePass, pkgInOut);
}

bool Vegetation::onRender(SceneGraphNode& sgn,
                          const SceneRenderState& sceneRenderState,
                          RenderStagePass renderStagePass) {
    if (_render && _success && _threadedLoadComplete) {
        RenderPackage& pkg = sgn.get<RenderingComponent>()->getDrawPackage(renderStagePass);

        GenericDrawCommand cmd = pkg.drawCommand(0, 0);
        disableOption(cmd, CmdRenderOptions::RENDER_INDIRECT);
        pkg.drawCommand(0, 0, cmd);

        g_grassDistance = sceneRenderState.grassVisibility();

        return true;
    }

    return false;
}

void Vegetation::computeGrassTransforms(const Task& parentTask) {
    
    const vec2<F32>& chunkPos = _terrainChunk.getOffsetAndSize().xy();
    const vec2<F32>& chunkSize = _terrainChunk.getOffsetAndSize().zw();
    const F32 waterLevel = 0.0f;// ToDo: make this dynamic! (cull underwater points later on?)
    const I32 currentCount = std::min((I32)_billboardCount, 4);
    const U16 mapWidth = _map->dimensions().width;
    const U16 mapHeight = _map->dimensions().height;
    const U32 grassElements = to_U32(_grassDensity * chunkSize.x * chunkSize.y);

    Console::printfn(Locale::get(_ID("CREATE_GRASS_BEGIN")), grassElements);

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
    g_tempData.resize(0);
    g_tempData.reserve(static_cast<size_t>(currentCount * chunkSize.x * chunkSize.y));
    const Terrain& terrain = _terrainChunk.parent();

#pragma omp parallel for
    for (I32 index = 0; index < currentCount; ++index) {
        densityFactor += 0.1f;
        GrassData entry = {};
        for (F32 width = chunkPos.x; width < chunkSize.x + chunkPos.x - densityFactor; width += Random(0.01f, densityFactor)) {
            if (parentTask._stopRequested) {
                break;
            }

            for (F32 height = chunkPos.y; height < chunkSize.y + chunkPos.y - densityFactor; height += Random(0.01f, densityFactor)) {
                if (_stopLoadingRequest || parentTask._stopRequested) {
                    break;
                }

                F32 x_fac = width / mapWidth;
                F32 y_fac = height / mapHeight;

                I32 map_colour = _map->getColour((U16)width, (U16)height)[index];
                if (map_colour < 150) {
                    continue;
                }
                Terrain::Vert vert = terrain.getVert(x_fac, y_fac);
                if (vert._position.y < waterLevel) {
                    continue;
                }
                if (vert._normal.y < 0.7f) {
                    continue;
                }

                mat4<F32> transform;
                mat3<F32> rotationMatrix = GetMatrix(RotationFromVToU(WORLD_Y_AXIS, vert._normal) * Quaternion<F32>(WORLD_Y_AXIS, Random(360.0f)));
                transform.setScale(vec3<F32>(((map_colour + 1) / 256.0f)));
                transform *= mat4<F32>(rotationMatrix, false);
                transform.setTranslation(vert._position);

#pragma omp critical
                {
                    entry._data.z = to_F32(index);
                    entry._transform.set(transform);
                    g_tempData.push_back(entry);
                }
            }
        }
    }

    Console::printfn(Locale::get(_ID("CREATE_GRASS_END")));
}
};
