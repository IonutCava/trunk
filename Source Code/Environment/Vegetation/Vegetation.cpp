#include "stdafx.h"

#include "Headers/Vegetation.h"

#include "Managers/Headers/SceneManager.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Geometry/Material/Headers/Material.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainChunk.h"
#include "Environment/Terrain/Quadtree/Headers/Quadtree.h"
#include "Environment/Terrain/Quadtree/Headers/QuadtreeNode.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"
#include "Platform/Video/Buffers/VertexBuffer/GenericBuffer/Headers/GenericVertexData.h"

namespace Divide {

bool Vegetation::_staticDataUpdated = false;

Vegetation::Vegetation(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, U32 chunkID, const VegetationDetails& details)
    : SceneNode(parentCache, descriptorHash + chunkID, details.name + "_" + to_stringImpl(chunkID), SceneNodeType::TYPE_VEGETATION_GRASS),
      _context(context),
      _billboardCount(details.billboardCount),
      _grassDensity(details.grassDensity),
      _grassScale(details.grassScale),
      _treeScale(details.treeScale),
      _treeDensity(details.treeDensity),
      _grassBillboards(details.grassBillboards),
      _terrain(details.parentTerrain),
      _render(false),
      _success(false),
      _culledFinal(false),
      _shadowMapped(true),
      _terrainChunk(nullptr),
      _instanceCountGrass(0),
      _instanceCountTrees(0),
      _grassStateBlockHash(0),
      _stateRefreshIntervalBufferUS(0ULL),
      _stateRefreshIntervalUS(Time::SecondsToMicroseconds(1))  ///<Every second?
{
    _threadedLoadComplete = false;
    _stopLoadingRequest = false;
    _readBuffer = 1;
    _writeBuffer = 0;
    _parentLoD = 0;

    _map = details.map;
    _grassShaderName = details.grassShaderName;
    _grassGPUBuffer[0] = _context.newGVD(3);
    _grassGPUBuffer[1] = _context.newGVD(3);
    _treeGPUBuffer[0] = _context.newGVD(1);
    _treeGPUBuffer[1] = _context.newGVD(1);

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
    _cullShader = CreateResource<ShaderProgram>(parentCache, instanceCullShader);
}

Vegetation::~Vegetation()
{
    Console::printfn(Locale::get(_ID("UNLOAD_VEGETATION_BEGIN")), name().c_str());
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

void Vegetation::initialize(TerrainChunk* const terrainChunk) {
    assert(terrainChunk);
    assert(_map->data() != nullptr);

    _terrainChunk = terrainChunk;

    RenderStateBlock transparentRenderState;
    transparentRenderState.setCullMode(CullMode::CW);
    _grassStateBlockHash = transparentRenderState.getHash();

    ResourceDescriptor vegetationMaterial("vegetationMaterial" + name());
    Material_ptr vegMaterial = CreateResource<Material>(_parentCache, vegetationMaterial);

    //vegMaterial->setShaderLoadThreaded(false);
    vegMaterial->setDiffuse(DefaultColours::WHITE);
    vegMaterial->setSpecular(FColour(0.1f, 0.1f, 0.1f, 1.0f));
    vegMaterial->setShininess(5.0f);
    vegMaterial->setShadingMode(Material::ShadingMode::BLINN_PHONG);
    vegMaterial->setShaderDefines("SKIP_TEXTURES");
    vegMaterial->setShaderProgram(_grassShaderName, RenderStage::DISPLAY, true);
    vegMaterial->setShaderProgram(_grassShaderName + ".PrePass", RenderPassType::DEPTH_PASS, true);
    vegMaterial->setShaderProgram(_grassShaderName + ".Shadow", RenderStage::SHADOW, true);
    vegMaterial->setTexture(ShaderProgram::TextureUsage::UNIT0, _grassBillboards);
    vegMaterial->dumpToFile(false);
    setMaterialTpl(vegMaterial);

    CreateTask(_context.parent().platformContext(),
                [this](const Task& parentTask) { generateGrass(parentTask); },
                [this]() {uploadGrassData();}
    ).startTask(Task::TaskPriority::LOW);

    setState(ResourceState::RES_LOADED);
}

namespace {
enum class BufferUsage : U8 {
    UnculledPositionBuffer = 0,
    CulledPositionBuffer = 1,
    UnculledSizeBuffer = 2,
    CulledSizeBuffer = 3,
    CulledInstanceBuffer = 4,
    COUNT
};

const U32 posLocation = 10;
const U32 scaleLocation = 11;
const U32 instLocation = 12;
const U32 instanceDiv = 1;
};

void Vegetation::uploadGrassData() {
    if (_grassPositions.empty()) {
        _threadedLoadComplete = true;
        return;
    }

    static const vec2<F32> pos000(cosf(Angle::to_RADIANS(0.000f)),
                                  sinf(Angle::to_RADIANS(0.000f)));
    static const vec2<F32> pos120(cosf(Angle::to_RADIANS(120.0f)),
                                  sinf(Angle::to_RADIANS(120.0f)));
    static const vec2<F32> pos240(cosf(Angle::to_RADIANS(240.0f)),
                                  sinf(Angle::to_RADIANS(240.0f)));

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



    if (_grassBlades.empty()) {
        U32 indexOffset = 0;
        for (U8 j = 0; j < 3; ++j) {
            indexOffset = (j * 4);
            for (U8 l = 0; l < 12; ++l) {
                _grassBlades.push_back(vertices[indices[l] + indexOffset]);
                _texCoord.push_back(texcoords[(indices[l] + indexOffset) % 4]);
            }
        }
    }

    if (_rotationMatrices.empty()) {
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
            _rotationMatrices.push_back(temp);
        }
    }

    for (U8 i = 0; i < 2; ++i) {
        GenericVertexData* buffer = _grassGPUBuffer[i];

        buffer->create(to_U8(BufferUsage::COUNT), 3);
        // position culled will be generated using transform feedback using
        // shader output 'posLocation'
        // writing to buffer "CulledPositionBuffer"
        buffer->setFeedbackBuffer(to_base(BufferUsage::CulledPositionBuffer), 0);
        buffer->setFeedbackBuffer(to_base(BufferUsage::CulledSizeBuffer), 1);
        buffer->setFeedbackBuffer(to_base(BufferUsage::CulledInstanceBuffer), 2);

        buffer->setBuffer(to_base(BufferUsage::UnculledPositionBuffer), _instanceCountGrass,
                          sizeof(vec4<F32>), true, &_grassPositions[0], BufferUpdateFrequency::OCASSIONAL);
        buffer->setBuffer(to_base(BufferUsage::UnculledSizeBuffer), _instanceCountGrass,
                          sizeof(F32), true, &_grassScales[0], BufferUpdateFrequency::OCASSIONAL);
        buffer->setBuffer(to_base(BufferUsage::CulledPositionBuffer), _instanceCountGrass * 3,
                          sizeof(vec4<F32>), true, NULL, BufferUpdateFrequency::OCASSIONAL);
        buffer->setBuffer(to_base(BufferUsage::CulledSizeBuffer), _instanceCountGrass * 3,
                          sizeof(F32), true, NULL, BufferUpdateFrequency::OCASSIONAL);
        buffer->setBuffer(to_base(BufferUsage::CulledInstanceBuffer), _instanceCountGrass * 3,
                          sizeof(I32), true, NULL, BufferUpdateFrequency::OCASSIONAL);

        buffer->attribDescriptor(posLocation)
            .set(to_base(BufferUsage::CulledPositionBuffer),
                 4,
                 GFXDataFormat::FLOAT_32,
                 false,
                 0,
                 instanceDiv);

        buffer->attribDescriptor(scaleLocation)
            .set(to_base(BufferUsage::CulledSizeBuffer),
                 1,
                 GFXDataFormat::FLOAT_32,
                 false,
                 0,
                 instanceDiv);

        buffer->attribDescriptor(instLocation)
            .set(to_base(BufferUsage::CulledInstanceBuffer), 
                 1,
                 GFXDataFormat::SIGNED_INT,
                 false,
                 0,
                 instanceDiv);
        buffer->fdbkAttribDescriptor(posLocation)
            .set(to_base(BufferUsage::UnculledPositionBuffer),
                 4,
                 GFXDataFormat::FLOAT_32,
                 false,
                 0,
                 instanceDiv);
        buffer->fdbkAttribDescriptor(scaleLocation)
            .set(to_base(BufferUsage::UnculledSizeBuffer),
                 1,
                 GFXDataFormat::FLOAT_32,
                 false,
                 0,
                 instanceDiv);

        buffer->toggleDoubleBufferedQueries(false);
    }

    _grassPositions.clear();
    _grassScales.clear();

    _render = _threadedLoadComplete = true;
}

void Vegetation::sceneUpdate(const U64 deltaTimeUS,
                             SceneGraphNode& sgn,
                             SceneState& sceneState) {
    static const Task updateTask;

    const SceneRenderState& renderState = sceneState.renderState();
    if (_threadedLoadComplete && !_success) {
        generateTrees(updateTask);
        Camera::addUpdateListener([this, &renderState](const Camera& cam) {
            gpuCull(renderState, cam);
        });
        _success = true;
    }

    if (_render && _success) {
        // Query shadow state every "_stateRefreshInterval" microseconds
        if (_stateRefreshIntervalBufferUS >= _stateRefreshIntervalUS) {
            if (!_staticDataUpdated) {
                _windX = sceneState.windDirX();
                _windZ = sceneState.windDirZ();
                _windS = sceneState.windSpeed();
                _stateRefreshIntervalBufferUS -= _stateRefreshIntervalUS;
                _staticDataUpdated = true;
            }
        }
        _stateRefreshIntervalBufferUS += deltaTimeUS;

        _writeBuffer = (_writeBuffer + 1) % 2;
        _readBuffer = (_writeBuffer + 1) % 2;
        _culledFinal = false;
    }

    SceneNode::sceneUpdate(deltaTimeUS, sgn, sceneState);
}

U32 Vegetation::getQueryID() {
    switch (_context.getRenderStage().stage()) {
        case RenderStage::SHADOW:
            return 0;
        case RenderStage::REFLECTION:
            return 1;
        case RenderStage::REFRACTION:
            return 2;
        default:
            return 3;
    };
}

void Vegetation::gpuCull(const SceneRenderState& sceneRenderState, const Camera& cam) {
    U32 queryID = getQueryID();

    bool draw = false;
    switch (queryID) {
        case 0 /*SHADOW*/: {
            draw = _context.getRenderStage() != _context.getPrevRenderStage();
            _culledFinal = false;
        } break;
        case 1 /*REFLECTION*/: {
            draw = true;
            _culledFinal = false;
        } break;
        default: {
            draw = !_culledFinal;
            _culledFinal = true;
        } break;
    }
    _parentLoD = _terrainChunk->getLoD(cam.getEye());
    if (draw && _threadedLoadComplete && _parentLoD == 0) {
        GenericVertexData* buffer = _grassGPUBuffer[_writeBuffer];
        //_cullShader->SetSubroutine(VERTEX,
        //_instanceRoutineIdx[HI_Z_CULL]);

        buffer->bindFeedbackBufferRange(to_base(BufferUsage::CulledPositionBuffer),
                                        _instanceCountGrass * queryID,
                                        _instanceCountGrass);
        buffer->bindFeedbackBufferRange(to_base(BufferUsage::CulledSizeBuffer),
                                        _instanceCountGrass * queryID,
                                        _instanceCountGrass);
        buffer->bindFeedbackBufferRange(to_base(BufferUsage::CulledInstanceBuffer),
                                        _instanceCountGrass * queryID,
                                        _instanceCountGrass);

        PipelineDescriptor pipeDesc;
        pipeDesc._shaderProgramHandle = _cullShader->getID();

        _cullDrawCommand.cmd().primCount = _instanceCountGrass;
        _cullDrawCommand.enableOption(GenericDrawCommand::RenderOptions::RENDER_NO_RASTERIZE);
        _cullDrawCommand.drawToBuffer(to_U8(queryID));
        _cullDrawCommand.sourceBuffer(buffer);
        buffer->incQueryQueue();

        GFX::ScopedCommandBuffer sBuffer = GFX::allocateScopedCommandBuffer();
        GFX::CommandBuffer& cmdBuffer = sBuffer();

        Texture_ptr depthTex = _context.renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachment(RTAttachmentType::Depth, 0).texture();

        GFX::BindDescriptorSetsCommand descriptorSetCmd;
        descriptorSetCmd._set._textureData.addTexture(depthTex->getData(), to_U8(ShaderProgram::TextureUsage::UNIT0));
        GFX::BindDescriptorSets(cmdBuffer, descriptorSetCmd);

        GFX::BindPipelineCommand pipelineCmd;
        pipelineCmd._pipeline = &_context.newPipeline(pipeDesc);
        GFX::BindPipeline(cmdBuffer, pipelineCmd);

        GFX::SendPushConstantsCommand pushConstantsCommand;
        pushConstantsCommand._constants.set("ObjectExtent", GFX::PushConstantType::VEC3, vec3<F32>(1.0f, 1.0f, 1.0f));
        pushConstantsCommand._constants.set("dvd_visibilityDistance", GFX::PushConstantType::FLOAT, sceneRenderState.grassVisibility());
        pushConstantsCommand._constants.set("cullType", GFX::PushConstantType::UINT, /*queryID*/ to_base(CullType::INSTANCE_CLOUD_REDUCTION));
        GFX::SendPushConstants(cmdBuffer, pushConstantsCommand);

        GFX::DrawCommand drawCmd;
        drawCmd._drawCommands.push_back(_cullDrawCommand);
        GFX::AddDrawCommands(cmdBuffer, drawCmd);

        _context.flushAndClearCommandBuffer(cmdBuffer);

        //_cullDrawCommand.setInstanceCount(_instanceCountTrees);
        //_cullDrawCommand.sourceBuffer(_treeGPUBuffer);
    }
}

void Vegetation::buildDrawCommands(SceneGraphNode& sgn,
                                   const RenderStagePass& renderStagePass,
                                   RenderPackage& pkgInOut) {

    GenericDrawCommand cmd;
    cmd.primitiveType(PrimitiveType::TRIANGLE_STRIP);
    cmd.cmd().firstIndex = 0;
    cmd.cmd().indexCount = 12 * 3;
    cmd.LoD(1);
    GFX::DrawCommand drawCommand;
    drawCommand._drawCommands.push_back(cmd);
    pkgInOut.addDrawCommand(drawCommand);

    const Pipeline* pipeline = pkgInOut.pipeline(0);
    PipelineDescriptor pipeDesc = pipeline->descriptor();
    pipeDesc._stateHash = _grassStateBlockHash;
    pkgInOut.pipeline(0, _context.newPipeline(pipeDesc));

    PushConstants constants = pkgInOut.pushConstants(0);
    constants.set("grassScale", GFX::PushConstantType::FLOAT, 1.0f);
    constants.set("positionOffsets", GFX::PushConstantType::VEC3, _grassBlades);
    constants.set("texCoordOffsets", GFX::PushConstantType::VEC2, _texCoord);
    constants.set("rotationMatrices", GFX::PushConstantType::MAT3, _rotationMatrices, true);
    constants.set("lod_metric", GFX::PushConstantType::FLOAT, 100.0f);
    pkgInOut.pushConstants(0, constants);

    SceneNode::buildDrawCommands(sgn, renderStagePass, pkgInOut);
}

bool Vegetation::onRender(SceneGraphNode& sgn,
                          const SceneRenderState& sceneRenderState,
                          const RenderStagePass& renderStagePass) {
    RenderingComponent* renderComp = sgn.get<RenderingComponent>();
    RenderPackage& pkg = renderComp->getDrawPackage(renderStagePass);

    ACKNOWLEDGE_UNUSED(sceneRenderState);
    GenericVertexData* buffer = _grassGPUBuffer[_readBuffer];
    U32 queryID = getQueryID();
    gpuCull(sceneRenderState, *sceneRenderState.parentScene().playerCamera());

    buffer->attribDescriptor(posLocation).strideInBytes(sizeof(vec4<F32>) * _instanceCountGrass * queryID);
    buffer->attribDescriptor(scaleLocation).strideInBytes(sizeof(F32) * _instanceCountGrass * queryID);
    buffer->attribDescriptor(instLocation).strideInBytes(sizeof(I32) * _instanceCountGrass * queryID);

    GenericDrawCommand cmd = pkg.drawCommand(0, 0);
    cmd.cmd().primCount = buffer->getFeedbackPrimitiveCount(to_U8(queryID));
    cmd.sourceBuffer(buffer);
    pkg.drawCommand(0, 0, cmd);
    
    _staticDataUpdated = false;
    return !(!_render || !_success || !_threadedLoadComplete ||
             _parentLoD > 0 ||
             (renderStagePass == _context.getPrevRenderStage() &&
              renderStagePass.stage() == RenderStage::SHADOW));
}

void Vegetation::generateTrees(const Task& parentTask) {
}

void Vegetation::generateGrass(const Task& parentTask) {
    
    //const vec2<F32>& chunkPos = _terrainChunk->getOffsetAndSize().xy();
    const vec2<F32>& chunkSize = _terrainChunk->getOffsetAndSize().zw();
    const F32 waterLevel = _terrainChunk->waterHeight(); //< ToDo: make this dynamic! (cull underwater points later on?)
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
    /*
    _grassPositions.reserve(grassElements);
    F32 densityFactor = 1.0f / _grassDensity;
#pragma omp parallel for
    for (I32 index = 0; index < currentCount; ++index) {
        densityFactor += 0.1f;
        for (F32 width = 0; width < chunkSize.x - densityFactor;
             width += densityFactor) {
            for (F32 height = 0; height < chunkSize.y - densityFactor;
                 height += densityFactor) {
                if (_stopLoadingRequest || stopRequested) {
                    continue;
                }
                F32 x = width + Random(densityFactor) + chunkPos.x;
                F32 y = height + Random(densityFactor) + chunkPos.y;
                CLAMP<F32>(x, 0.0f, to_F32(mapWidth) - 1.0f);
                CLAMP<F32>(y, 0.0f, to_F32(mapHeight) - 1.0f);
                F32 x_fac = x / _map->dimensions().width;
                F32 y_fac = y / _map->dimensions().height;

                I32 map_colour = _map->getColour((U16)x, (U16)y)[index];
                if (map_colour < 150) {
                    continue;
                }
                const vec3<F32>& P = _terrain->getPosition(x_fac, y_fac);
                if (P.y < waterLevel) {
                    continue;
                }
                const vec3<F32>& N = _terrain->getNormal(x_fac, y_fac);
                if (N.y < 0.7f) {
                    continue;
                }
#pragma omp critical
                {
                    position.set(P);
                    //mat4<F32> matRot1;
                    //matRot1.scale(vec3<F32>(grassScale));
                    //mat4<F32> matRot2;
                    //matRot2.fromYRotation(Random(360.0f));
                    //_grassMatricesTemp.push_back(matRot1 * matRot2 *
                    //rotationFromVToU(WORLD_Y_AXIS, N).getMatrix());

                    _grassPositions.push_back(vec4<F32>(P, to_F32(index)));
                    _grassScales.push_back(((map_colour + 1) / 256.0f));
                    _instanceCountGrass++;
                }
            }
        }
    }
    */
    Console::printfn(Locale::get(_ID("CREATE_GRASS_END")));
}
};
