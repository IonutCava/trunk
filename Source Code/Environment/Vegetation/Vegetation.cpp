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

Vegetation::Vegetation(GFXDevice& context, ResourceCache& parentCache, const VegetationDetails& details)
    : SceneNode(parentCache, details.name, SceneNodeType::TYPE_VEGETATION_GRASS),
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
      _stateRefreshIntervalBuffer(0ULL),
      _stateRefreshInterval(Time::SecondsToMicroseconds(1))  ///<Every second?
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

        shader->Uniform("ObjectExtent", vec3<F32>(1.0f, 1.0f, 1.0f));
        _instanceRoutineIdx[to_const_uint(CullType::PASS_THROUGH)] = shader->GetSubroutineIndex(ShaderType::VERTEX, "PassThrough");
        _instanceRoutineIdx[to_const_uint(CullType::INSTANCE_CLOUD_REDUCTION)] = shader->GetSubroutineIndex(ShaderType::VERTEX, "InstanceCloudReduction");
        _instanceRoutineIdx[to_const_uint(CullType::HI_Z_CULL)] = shader->GetSubroutineIndex(ShaderType::VERTEX, "HiZOcclusionCull");
    };

    ResourceDescriptor instanceCullShader("instanceCull");
    instanceCullShader.setThreadedLoading(true);
    instanceCullShader.setID(3);
    instanceCullShader.setOnLoadCallback(setShaderData);
    _cullShader = CreateResource<ShaderProgram>(parentCache, instanceCullShader);
}

Vegetation::~Vegetation()
{
    Console::printfn(Locale::get(_ID("UNLOAD_VEGETATION_BEGIN")), getName().c_str());
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
    // transparent.setBlend(true);
    _grassStateBlockHash = transparentRenderState.getHash();

    ResourceDescriptor vegetationMaterial("vegetationMaterial" + getName());
    Material_ptr vegMaterial = CreateResource<Material>(_parentCache, vegetationMaterial);

    vegMaterial->setDiffuse(DefaultColours::WHITE());
    vegMaterial->setSpecular(vec4<F32>(0.1f, 0.1f, 0.1f, 1.0f));
    vegMaterial->setShininess(5.0f);
    vegMaterial->setShadingMode(Material::ShadingMode::BLINN_PHONG);
    vegMaterial->setShaderDefines("SKIP_TEXTURES");
    vegMaterial->setShaderProgram(_grassShaderName, RenderStage::DISPLAY, true);
    vegMaterial->setShaderProgram(_grassShaderName + ".Shadow", RenderStage::SHADOW, true);
    vegMaterial->setShaderProgram(_grassShaderName + ".PrePass",RenderPassType::DEPTH_PASS, true);
    vegMaterial->setTexture(ShaderProgram::TextureUsage::UNIT0, _grassBillboards);
    vegMaterial->setShaderLoadThreaded(false);
    vegMaterial->dumpToFile(false);
    setMaterialTpl(vegMaterial);

    CreateTask(DELEGATE_BIND(&Vegetation::generateGrass, this, std::placeholders::_1),
            DELEGATE_BIND(&Vegetation::uploadGrassData, this))
            .startTask(Task::TaskPriority::LOW);
    setState(ResourceState::RES_LOADED);
}

namespace {
enum class BufferUsage : U32 {
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

    static const vec2<F32> pos000(cosf(Angle::DegreesToRadians(0.000f)),
                                  sinf(Angle::DegreesToRadians(0.000f)));
    static const vec2<F32> pos120(cosf(Angle::DegreesToRadians(120.0f)),
                                  sinf(Angle::DegreesToRadians(120.0f)));
    static const vec2<F32> pos240(cosf(Angle::DegreesToRadians(240.0f)),
                                  sinf(Angle::DegreesToRadians(240.0f)));

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

    vectorImpl<vec3<F32> > grassBlades;
    vectorImpl<vec2<F32> > texCoord;

    U32 indexOffset = 0;
    for (U8 j = 0; j < 3; ++j) {
        indexOffset = (j * 4);
        for (U8 l = 0; l < 12; ++l) {
            grassBlades.push_back(vertices[indices[l] + indexOffset]);
            texCoord.push_back(texcoords[(indices[l] + indexOffset) % 4]);
        }
    }

    static vectorImpl<mat3<F32> > rotationMatrices;
    if (rotationMatrices.empty()) {
        vectorImpl<F32> angles;
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
            rotationMatrices.push_back(temp);
        }
    }

    const Material_ptr& mat = getMaterialTpl();
    for (U8 pass = 0; pass < to_const_ubyte(RenderPassType::COUNT); ++pass) {
        for (U32 i = 0; i < to_const_uint(RenderStage::COUNT); ++i) {
            const ShaderProgram_ptr& drawShader = mat->getShaderInfo(RenderStagePass(static_cast<RenderStage>(i), static_cast<RenderPassType>(pass))).getProgram();

            drawShader->Uniform("positionOffsets", grassBlades);
            drawShader->Uniform("texCoordOffsets", texCoord);
            drawShader->Uniform("rotationMatrices", rotationMatrices);
            drawShader->Uniform("lod_metric", 100.0f);
        }
    }

    for (U8 i = 0; i < 2; ++i) {
        GenericVertexData* buffer = _grassGPUBuffer[i];

        buffer->create(to_const_ubyte(BufferUsage::COUNT), 3);
        // position culled will be generated using transform feedback using
        // shader output 'posLocation'
        // writing to buffer "CulledPositionBuffer"
        buffer->setFeedbackBuffer(to_const_uint(BufferUsage::CulledPositionBuffer), 0);
        buffer->setFeedbackBuffer(to_const_uint(BufferUsage::CulledSizeBuffer), 1);
        buffer->setFeedbackBuffer(to_const_uint(BufferUsage::CulledInstanceBuffer), 2);

        buffer->setBuffer(to_const_uint(BufferUsage::UnculledPositionBuffer), _instanceCountGrass,
                          sizeof(vec4<F32>), true, &_grassPositions[0], false, false);
        buffer->setBuffer(to_const_uint(BufferUsage::UnculledSizeBuffer), _instanceCountGrass,
                          sizeof(F32), true, &_grassScales[0], false, false);
        buffer->setBuffer(to_const_uint(BufferUsage::CulledPositionBuffer), _instanceCountGrass * 3,
                          sizeof(vec4<F32>), true, NULL, true, false);
        buffer->setBuffer(to_const_uint(BufferUsage::CulledSizeBuffer), _instanceCountGrass * 3,
                          sizeof(F32), true, NULL, true, false);
        buffer->setBuffer(to_const_uint(BufferUsage::CulledInstanceBuffer), _instanceCountGrass * 3,
                          sizeof(I32), true, NULL, true, false);

        buffer->attribDescriptor(posLocation)
            .set(to_const_uint(BufferUsage::CulledPositionBuffer), instanceDiv, 4, false, 0, 
                             GFXDataFormat::FLOAT_32);
        buffer->attribDescriptor(scaleLocation)
            .set(to_const_uint(BufferUsage::CulledSizeBuffer), instanceDiv, 1, false, 0,
                 GFXDataFormat::FLOAT_32);
        buffer->attribDescriptor(instLocation)
            .set(to_const_uint(BufferUsage::CulledInstanceBuffer), instanceDiv, 1, false, 0,
                 GFXDataFormat::SIGNED_INT);
        buffer->fdbkAttribDescriptor(posLocation)
            .set(to_const_uint(BufferUsage::UnculledPositionBuffer), instanceDiv, 4, false, 0,
                 GFXDataFormat::FLOAT_32);
        buffer->fdbkAttribDescriptor(scaleLocation)
            .set(to_const_uint(BufferUsage::UnculledSizeBuffer), instanceDiv, 1, false, 0,
                 GFXDataFormat::FLOAT_32);

        buffer->toggleDoubleBufferedQueries(false);
    }

    _grassPositions.clear();
    _grassScales.clear();

    _render = _threadedLoadComplete = true;
}

void Vegetation::sceneUpdate(const U64 deltaTime,
                             SceneGraphNode& sgn,
                             SceneState& sceneState) {
    static const Task updateTask;

    const SceneRenderState& renderState = sceneState.renderState();
    if (_threadedLoadComplete && !_success) {
        generateTrees(updateTask);
        Camera::addUpdateListener([this, &renderState](const Camera& cam) {
            gpuCull(renderState);
        });
        _success = true;
    }

    if (_render && _success) {
        // Query shadow state every "_stateRefreshInterval" microseconds
        if (_stateRefreshIntervalBuffer >= _stateRefreshInterval) {
            if (!_staticDataUpdated) {
                _windX = sceneState.windDirX();
                _windZ = sceneState.windDirZ();
                _windS = sceneState.windSpeed();
                _stateRefreshIntervalBuffer -= _stateRefreshInterval;
                _cullShader->Uniform("dvd_visibilityDistance",
                                     sceneState.renderState().grassVisibility());
                _staticDataUpdated = true;
            }
        }
        _stateRefreshIntervalBuffer += deltaTime;

        _writeBuffer = (_writeBuffer + 1) % 2;
        _readBuffer = (_writeBuffer + 1) % 2;
        _culledFinal = false;
    }

    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}

U32 Vegetation::getQueryID() {
    switch (_context.getRenderStage()._stage) {
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

void Vegetation::gpuCull(const SceneRenderState& sceneRenderState) {
    U32 queryID = getQueryID();

    if (_context.is2DRendering()) {
        return;
    }

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
    _parentLoD = _terrainChunk->getLoD(sceneRenderState);
    if (draw && _threadedLoadComplete && _parentLoD == 0) {
        GenericVertexData* buffer = _grassGPUBuffer[_writeBuffer];
        //_cullShader->SetSubroutine(VERTEX,
        //_instanceRoutineIdx[HI_Z_CULL]);
        _cullShader->Uniform("cullType",
                             /*queryID*/ to_const_uint(CullType::INSTANCE_CLOUD_REDUCTION));

        _context.renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).bind(0, RTAttachment::Type::Depth, 0);
        buffer->bindFeedbackBufferRange(to_const_uint(BufferUsage::CulledPositionBuffer),
                                        _instanceCountGrass * queryID,
                                        _instanceCountGrass);
        buffer->bindFeedbackBufferRange(to_const_uint(BufferUsage::CulledSizeBuffer),
                                        _instanceCountGrass * queryID,
                                        _instanceCountGrass);
        buffer->bindFeedbackBufferRange(to_const_uint(BufferUsage::CulledInstanceBuffer),
                                        _instanceCountGrass * queryID,
                                        _instanceCountGrass);

        _cullDrawCommand.cmd().primCount = _instanceCountGrass;
        _cullDrawCommand.enableOption(GenericDrawCommand::RenderOptions::RENDER_NO_RASTERIZE);
        _cullDrawCommand.drawToBuffer(to_ubyte(queryID));
        _cullDrawCommand.shaderProgram(_cullShader);
        _cullDrawCommand.sourceBuffer(buffer);
        buffer->incQueryQueue();

        _context.draw(_cullDrawCommand);

        //_cullDrawCommand.setInstanceCount(_instanceCountTrees);
        //_cullDrawCommand.sourceBuffer(_treeGPUBuffer);
        // GFXDevice::instance().draw(_cullDrawCommand);
    }
}

void Vegetation::initialiseDrawCommands(SceneGraphNode& sgn,
                                        const RenderStagePass& renderStagePass,
                                        GenericDrawCommands& drawCommandsInOut) {

    GenericDrawCommand cmd;
    cmd.primitiveType(PrimitiveType::TRIANGLE_STRIP);
    cmd.cmd().firstIndex = 0;
    cmd.cmd().indexCount = 12 * 3;
    cmd.LoD(1);
    cmd.stateHash(_grassStateBlockHash);
    drawCommandsInOut.push_back(cmd);

    SceneNode::initialiseDrawCommands(sgn, renderStagePass, drawCommandsInOut);
}

void Vegetation::updateDrawCommands(SceneGraphNode& sgn,
                                    const RenderStagePass& renderStagePass,
                                    const SceneRenderState& sceneRenderState,
                                    GenericDrawCommands& drawCommandsOut) {

    GenericVertexData* buffer = _grassGPUBuffer[_readBuffer];
    U32 queryID = getQueryID();
    gpuCull(sceneRenderState);

    buffer->attribDescriptor(posLocation).offset(_instanceCountGrass * queryID);
    buffer->attribDescriptor(scaleLocation).offset(_instanceCountGrass * queryID);
    buffer->attribDescriptor(instLocation).offset(_instanceCountGrass * queryID);

    GenericDrawCommand& cmd = drawCommandsOut.front();
    cmd.cmd().primCount = buffer->getFeedbackPrimitiveCount(to_ubyte(queryID));
    cmd.sourceBuffer(buffer);
    cmd.shaderProgram()->Uniform("grassScale", /*_grassScale*/1.0f);

    SceneNode::updateDrawCommands(sgn, renderStagePass, sceneRenderState, drawCommandsOut);
}

bool Vegetation::onRender(const RenderStagePass& renderStagePass) {
    _staticDataUpdated = false;
    return !(!_render || !_success || !_threadedLoadComplete ||
             _parentLoD > 0 ||
             (renderStagePass == _context.getPrevRenderStage() &&
              renderStagePass._stage == RenderStage::SHADOW));
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
    const U32 grassElements = to_uint(_grassDensity * chunkSize.x * chunkSize.y);

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
                CLAMP<F32>(x, 0.0f, to_float(mapWidth) - 1.0f);
                CLAMP<F32>(y, 0.0f, to_float(mapHeight) - 1.0f);
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

                    _grassPositions.push_back(vec4<F32>(P, to_float(index)));
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
