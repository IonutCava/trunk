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
    static F32 g_grassDistance = 100.0f;
    thread_local vectorEASTL<GrassData> g_tempData;
};

vector<mat3<F32> > Vegetation::s_rotationMatrices;
vector<vec3<F32> > Vegetation::s_grassBlades;
vector<vec2<F32> > Vegetation::s_texCoord;
GenericVertexData* Vegetation::s_grassGPUBuffer = nullptr;

Vegetation::Vegetation(GFXDevice& context, 
                       TerrainChunk& parentChunk,
                       const VegetationDetails& details)
    : SceneNode(context.parent().resourceCache(), parentChunk.parent().getDescriptorHash() + parentChunk.ID(), details.name + "_" + to_stringImpl(parentChunk.ID()), SceneNodeType::TYPE_VEGETATION_GRASS),
      _context(context),
      _terrainChunk(parentChunk),
      _billboardCount(details.billboardCount),
      _grassDensity(details.grassDensity),
      _grassScale(details.grassScale),
      _treeScale(details.treeScale),
      _treeDensity(details.treeDensity),
      _terrain(details.parentTerrain),
      _render(false),
      _success(false),
      _shadowMapped(true),
      _grassData(nullptr),
      _instanceCountGrass(0),
      _instanceCountTrees(0),
      _grassStateBlockHash(0),
      _stateRefreshIntervalBufferUS(0ULL),
      _stateRefreshIntervalUS(Time::SecondsToMicroseconds(1))  ///<Every second?
{
    _threadedLoadComplete = false;
    _stopLoadingRequest = false;

    _map = details.map;
    _cullDrawCommand = GenericDrawCommand(PrimitiveType::API_POINTS, 0, 1);

    _instanceRoutineIdx.fill(0);

    auto setShaderData = [this](Resource_wptr res) {
        ShaderProgram_ptr shader = std::dynamic_pointer_cast<ShaderProgram>(res.lock());
        _instanceRoutineIdx[to_base(CullType::PASS_THROUGH)] = shader->GetSubroutineIndex(ShaderType::VERTEX, "PassThrough");
        _instanceRoutineIdx[to_base(CullType::INSTANCE_CLOUD_REDUCTION)] = shader->GetSubroutineIndex(ShaderType::VERTEX, "InstanceCloudReduction");
        _instanceRoutineIdx[to_base(CullType::HI_Z_CULL)] = shader->GetSubroutineIndex(ShaderType::VERTEX, "HiZOcclusionCull");
    };

    ResourceDescriptor instanceCullShader("instanceCull");
    instanceCullShader.setThreadedLoading(true);
    instanceCullShader.setID(3);
    instanceCullShader.setOnLoadCallback(setShaderData);
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
    if (s_grassGPUBuffer == nullptr) {
        static const vec2<F32> pos000(cosf(Angle::to_RADIANS(0.000f)), sinf(Angle::to_RADIANS(0.000f)));
        static const vec2<F32> pos120(cosf(Angle::to_RADIANS(120.0f)), sinf(Angle::to_RADIANS(120.0f)));
        static const vec2<F32> pos240(cosf(Angle::to_RADIANS(240.0f)), sinf(Angle::to_RADIANS(240.0f)));

        static const vec3<F32> vertices[] = {vec3<F32>(-pos000.x, 0.0f, -pos000.y),
                                             vec3<F32>(-pos000.x, 1.0f, -pos000.y),
                                             vec3<F32>(pos000.x, 1.0f, pos000.y),
                                             vec3<F32>(pos000.x, 0.0f, pos000.y),

                                             vec3<F32>(-pos120.x, 0.0f, -pos120.y),
                                             vec3<F32>(-pos120.x, 1.0f, -pos120.y),
                                             vec3<F32>(pos120.x, 1.0f, pos120.y),
                                             vec3<F32>(pos120.x, 0.0f, pos120.y),

                                             vec3<F32>(-pos240.x, 0.0f, -pos240.y),
                                             vec3<F32>(-pos240.x, 1.0f, -pos240.y),
                                             vec3<F32>(pos240.x, 1.0f, pos240.y),
                                             vec3<F32>(pos240.x, 0.0f, pos240.y)};

        static const U32 indices[] = {2, 1, 0, 2, 0, 1, 2, 0, 3, 2, 3, 0};

        static const vec2<F32> texcoords[] = {vec2<F32>(0.0f, 0.99f),
                                              vec2<F32>(0.0f, 0.01f),
                                              vec2<F32>(1.0f, 0.01f),
                                              vec2<F32>(1.0f, 0.99f)};

        s_grassGPUBuffer = _context.newGVD(1);

        if (s_grassBlades.empty()) {
            U32 indexOffset = 0;
            for (U8 j = 0; j < 3; ++j) {
                indexOffset = (j * 4);
                for (U8 l = 0; l < 12; ++l) {
                    s_grassBlades.push_back(vertices[indices[l] + indexOffset]);
                    s_texCoord.push_back(texcoords[(indices[l] + indexOffset) % 4]);
                }
            }
        }

        if (s_rotationMatrices.empty()) {
            vector<F32> angles;
            angles.resize(18, 0.0f);
            for (U8 i = 0; i < 18; ++i) {
                F32 temp = Random(360.0f);
                while (std::find(std::begin(angles), std::end(angles), temp) !=
                       std::end(angles)) {
                    temp = Random(360.0f);
                }
                angles[i] = temp;
            }

            mat3<F32> temp;
            for (U8 i = 0; i < 18; ++i) {
                temp.identity();
                temp.fromYRotation(angles[i]);
                s_rotationMatrices.push_back(temp);
            }
        }

        s_grassGPUBuffer->create(1);
        s_grassGPUBuffer->setBuffer(0, _instanceCountGrass * 3, sizeof(I32), true, NULL, BufferUpdateFrequency::OCASSIONAL);
        s_grassGPUBuffer->attribDescriptor(12)
            .set(0, //buffer
                    1, //component per element
                    GFXDataFormat::SIGNED_INT,//element type 
                    false, //normalized
                    0,//stride in bytes
                    1); //instance div
    
    }

    U32 elementCount = to_U32(g_tempData.size());
    ShaderBufferDescriptor bufferDescriptor = {};
    bufferDescriptor._elementCount = elementCount;
    bufferDescriptor._elementSize = sizeof(GrassData);
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::ONCE;
    bufferDescriptor._initialData = (bufferPtr)g_tempData.data();
    bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::UNBOUND_STORAGE);
    bufferDescriptor._name = Util::StringFormat("Grass_data_chunk_%d", _terrainChunk.ID());
    if (elementCount > 0) {
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
        generateTrees(updateTask);
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
        //_cullShader->SetSubroutine(VERTEX, _instanceRoutineIdx[HI_Z_CULL]);

        PipelineDescriptor pipeDesc;
        pipeDesc._shaderProgramHandle = _cullShader->getID();

        _cullDrawCommand._cmd.primCount = _instanceCountGrass;
        _cullDrawCommand._bufferIndex = 0;
        _cullDrawCommand._sourceBuffer = s_grassGPUBuffer;
        
        enableOption(_cullDrawCommand, CmdRenderOptions::RENDER_NO_RASTERIZE);

        Texture_ptr depthTex = _context.renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachment(RTAttachmentType::Depth, 0).texture();

        GFX::BindDescriptorSetsCommand descriptorSetCmd;
        descriptorSetCmd._set = _context.newDescriptorSet();
        descriptorSetCmd._set->_textureData.addTexture(depthTex->getData(), to_U8(ShaderProgram::TextureUsage::UNIT0));
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        GFX::BindPipelineCommand pipelineCmd;
        pipelineCmd._pipeline = _context.newPipeline(pipeDesc);
        GFX::EnqueueCommand(bufferInOut, pipelineCmd);

        GFX::SendPushConstantsCommand pushConstantsCommand;
        pushConstantsCommand._constants.set("ObjectExtent", GFX::PushConstantType::VEC3, vec3<F32>(1.0f, 1.0f, 1.0f));
        pushConstantsCommand._constants.set("dvd_visibilityDistance", GFX::PushConstantType::FLOAT, g_grassDistance);
        pushConstantsCommand._constants.set("cullType", GFX::PushConstantType::UINT, /*queryID*/ to_base(CullType::INSTANCE_CLOUD_REDUCTION));
        GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

        GFX::DrawCommand drawCmd;
        drawCmd._drawCommands.push_back(_cullDrawCommand);
        GFX::EnqueueCommand(bufferInOut, drawCmd);
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
        cmd._cmd.firstIndex = 0;
        cmd._cmd.indexCount = 36;
        cmd._sourceBuffer = s_grassGPUBuffer;
        GFX::DrawCommand drawCommand;
        drawCommand._drawCommands.push_back(cmd);
        pkgInOut.addDrawCommand(drawCommand);

        const Pipeline* pipeline = pkgInOut.pipeline(0);
        PipelineDescriptor pipeDesc = pipeline->descriptor();
        pipeDesc._stateHash = _grassStateBlockHash;
        pkgInOut.pipeline(0, *_context.newPipeline(pipeDesc));

        PushConstants constants = pkgInOut.pushConstants(0);
        constants.set("positionOffsets", GFX::PushConstantType::VEC3, s_grassBlades);
        constants.set("texCoordOffsets", GFX::PushConstantType::VEC2, s_texCoord);
        constants.set("rotationMatrices", GFX::PushConstantType::MAT3, s_rotationMatrices, true);
        constants.set("dvd_visibilityDistance", GFX::PushConstantType::FLOAT, 100.0f);
        pkgInOut.pushConstants(0, constants);
    }

    SceneNode::buildDrawCommands(sgn, renderStagePass, pkgInOut);
}

bool Vegetation::onRender(SceneGraphNode& sgn,
                          const SceneRenderState& sceneRenderState,
                          RenderStagePass renderStagePass) {
    if (_render && _success && _threadedLoadComplete) {
        RenderingComponent* renderComp = sgn.get<RenderingComponent>();
        RenderPackage& pkg = renderComp->getDrawPackage(renderStagePass);

        GenericDrawCommand cmd = pkg.drawCommand(0, 0);
        cmd._cmd.primCount = _instanceCountGrass;
        cmd._sourceBuffer = s_grassGPUBuffer;
        cmd._bufferIndex = renderStagePass.index();
        pkg.drawCommand(0, 0, cmd);

        g_grassDistance = sceneRenderState.grassVisibility();

        return true;
    }

    return false;
}

void Vegetation::generateTrees(const Task& parentTask) {
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
        for attemp = 1:maxAttemp
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
        for (F32 width = 0; width < chunkSize.x - densityFactor; width += densityFactor) {
            if (parentTask._stopRequested) {
                break;
            }

            for (F32 height = 0; height < chunkSize.y - densityFactor; height += densityFactor) {
                if (_stopLoadingRequest || parentTask._stopRequested) {
                    break;
                }
                F32 x = width + Random(densityFactor) + chunkPos.x;
                F32 y = height + Random(densityFactor) + chunkPos.y;
                CLAMP<F32>(x, 0.0f, to_F32(mapWidth) - 1.0f);
                CLAMP<F32>(y, 0.0f, to_F32(mapHeight) - 1.0f);
                F32 x_fac = x / mapWidth;
                F32 y_fac = y / mapHeight;

                I32 map_colour = _map->getColour((U16)x, (U16)y)[index];
                if (map_colour < 150) {
                    continue;
                }
                const vec3<F32>& P = terrain.getPosition(x_fac, y_fac);
                if (P.y < waterLevel) {
                    continue;
                }
                const vec3<F32>& N = terrain.getNormal(x_fac, y_fac);
                if (N.y < 0.7f) {
                    continue;
                }

                entry._positionAndIndex.set(P, to_F32(index));
                entry._transform.identity();
                entry._transform.setScale(vec3<F32>(((map_colour + 1) / 256.0f)));

                mat3<F32> rotationMatrix = GetMatrix(RotationFromVToU(WORLD_Y_AXIS, N) * Quaternion<F32>(WORLD_Y_AXIS, Random(360.0f)));
                entry._transform *= mat4<F32>(rotationMatrix, true);

#pragma omp critical
                {
                    g_tempData.push_back(entry);
                    _instanceCountGrass++;
                }
            }
        }
    }

    Console::printfn(Locale::get(_ID("CREATE_GRASS_END")));
}
};
