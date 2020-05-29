#include "stdafx.h"

#include "Headers/Terrain.h"
#include "Headers/TerrainChunk.h"
#include "Headers/TerrainDescriptor.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"

#include "ECS/Components/Headers/TransformComponent.h"
#include "ECS/Components/Headers/RigidBodyComponent.h"
#include "ECS/Components/Headers/RenderingComponent.h"

#include "Geometry/Material/Headers/Material.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderPackage.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

namespace {
    constexpr U32 g_bufferFrameDelay = 3;

    void CreateTileQuadListIB(vectorEASTLFast<U32>& indices)
    {
        indices.resize(Terrain::QUAD_LIST_INDEX_COUNT, 0u);
        I32 index = 0;

        // The IB describes one tile of NxN patches.
        // Four vertices per quad, with VTX_PER_TILE_EDGE-1 quads per tile edge.
        for (I32 y = 0; y < Terrain::VTX_PER_TILE_EDGE - 1; ++y)
        {
            const I32 rowStart = y * Terrain::VTX_PER_TILE_EDGE;

            for (I32 x = 0; x < Terrain::VTX_PER_TILE_EDGE - 1; ++x) {
                indices[index++] = rowStart + x;
                indices[index++] = rowStart + x + Terrain::VTX_PER_TILE_EDGE;
                indices[index++] = rowStart + x + Terrain::VTX_PER_TILE_EDGE + 1;
                indices[index++] = rowStart + x + 1;
            }
        }
        assert(index == Terrain::QUAD_LIST_INDEX_COUNT);
    }
}

Terrain::Terrain(GFXDevice& context, ResourceCache* parentCache, size_t descriptorHash, const Str256& name)
    : Object3D(context, parentCache, descriptorHash, name, name, "", ObjectType::TERRAIN, 0u),
      _terrainQuadtree(context)
{
    _renderState.addToDrawExclusionMask(RenderStage::SHADOW, RenderPassType::COUNT, to_U8(LightType::SPOT));
    _renderState.addToDrawExclusionMask(RenderStage::SHADOW, RenderPassType::COUNT, to_U8(LightType::POINT));
}

void Terrain::postLoad(SceneGraphNode& sgn) {

    if (!_initialSetupDone) {
        _editorComponent.onChangedCbk([this](std::string_view field) {onEditorChange(field); });

        EditorComponentField tessTriangleWidthField = {};
        tessTriangleWidthField._name = "Tessellated Triangle Width";
        tessTriangleWidthField._data = &_descriptor->_tessellatedTriangleWidth;
        tessTriangleWidthField._type = EditorComponentFieldType::SLIDER_TYPE;
        tessTriangleWidthField._readOnly = false;
        tessTriangleWidthField._basicType = GFX::PushConstantType::UINT;
        tessTriangleWidthField._range = { 1.0f, 150.0f };
        tessTriangleWidthField._step = 1.0f;

        _editorComponent.registerField(std::move(tessTriangleWidthField));

        EditorComponentField parallaxHeightField = {};
        parallaxHeightField._name = "Parallax Height";
        parallaxHeightField._data = &_descriptor->_parallaxHeightScale;
        parallaxHeightField._type = EditorComponentFieldType::SLIDER_TYPE;
        parallaxHeightField._readOnly = false;
        parallaxHeightField._basicType = GFX::PushConstantType::FLOAT;
        parallaxHeightField._range = { 0.01f, 10.0f };

        _editorComponent.registerField(std::move(parallaxHeightField));

        EditorComponentField toggleBoundsField = {};
        toggleBoundsField._name = "Toggle Quadtree Bounds";
        toggleBoundsField._range = { toggleBoundsField._name.length() * 10.0f, 20.0f };//dimensions
        toggleBoundsField._type = EditorComponentFieldType::BUTTON;
        toggleBoundsField._readOnly = false; //disabled/enabled
        _editorComponent.registerField(std::move(toggleBoundsField));

        PlatformContext& pContext = sgn.context();
        SceneManager* sMgr = pContext.kernel().sceneManager();

        EditorComponentField grassVisibilityDistanceField = {};
        grassVisibilityDistanceField._name = "Grass visibility distance";
        grassVisibilityDistanceField._range = { 0.01f, 10000.0f };
        grassVisibilityDistanceField._serialise = false;
        grassVisibilityDistanceField._dataGetter = [sMgr](void* dataOut) {
            const SceneRenderState& rState = sMgr->getActiveScene().renderState();
            *static_cast<F32*>(dataOut) = rState.grassVisibility();
        };
        grassVisibilityDistanceField._dataSetter = [sMgr](const void* data) {
            SceneRenderState& rState = sMgr->getActiveScene().renderState();
            rState.grassVisibility(*static_cast<const F32*>(data)); 
        };
        grassVisibilityDistanceField._type = EditorComponentFieldType::PUSH_TYPE;
        grassVisibilityDistanceField._basicType = GFX::PushConstantType::FLOAT;
        grassVisibilityDistanceField._readOnly = false;
        _editorComponent.registerField(std::move(grassVisibilityDistanceField));

        EditorComponentField treeVisibilityDistanceField = {};
        treeVisibilityDistanceField._name = "Tree visibility distance";
        treeVisibilityDistanceField._range = { 0.01f, 10000.0f };
        treeVisibilityDistanceField._serialise = false;
        treeVisibilityDistanceField._dataGetter = [sMgr](void* dataOut) {
            const SceneRenderState& rState = sMgr->getActiveScene().renderState();
            *static_cast<F32*>(dataOut) = rState.treeVisibility();
        };
        treeVisibilityDistanceField._dataSetter = [sMgr](const void* data) {
            SceneRenderState& rState = sMgr->getActiveScene().renderState();
            rState.treeVisibility(*static_cast<const F32*>(data));
        };
        treeVisibilityDistanceField._type = EditorComponentFieldType::PUSH_TYPE;
        treeVisibilityDistanceField._basicType = GFX::PushConstantType::FLOAT;
        treeVisibilityDistanceField._readOnly = false;
        _editorComponent.registerField(std::move(treeVisibilityDistanceField));

        ShaderBufferDescriptor bufferDescriptor = {};
        bufferDescriptor._elementCount = descriptor()->maxNodesPerStage() * ((to_base(RenderStage::COUNT) - 1));
        bufferDescriptor._elementSize = sizeof(TessellatedNodeData);
        bufferDescriptor._ringBufferLength = g_bufferFrameDelay;
        bufferDescriptor._separateReadWrite = false;
        bufferDescriptor._usage = ShaderBuffer::Usage::UNBOUND_BUFFER;
        bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::ALLOW_THREADED_WRITES) |
                                  to_U32(ShaderBuffer::Flags::AUTO_RANGE_FLUSH);
        //Should be once per frame
        bufferDescriptor._updateFrequency = BufferUpdateFrequency::OFTEN;
        bufferDescriptor._updateUsage = BufferUpdateUsage::CPU_W_GPU_R;

        bufferDescriptor._name = "TERRAIN_RENDER_NODES";
        _shaderData = _context.newSB(bufferDescriptor);
        _initialSetupDone = true;
    }

    SceneGraphNodeDescriptor vegetationParentNode;
    vegetationParentNode._serialize = false;
    vegetationParentNode._name = "Vegetation";
    vegetationParentNode._usageContext = NodeUsageContext::NODE_STATIC;
    vegetationParentNode._componentMask = to_base(ComponentType::TRANSFORM) | to_base(ComponentType::BOUNDS);
    SceneGraphNode* vegParent = sgn.addChildNode(vegetationParentNode);
    assert(vegParent != nullptr);

    SceneGraphNodeDescriptor vegetationNodeDescriptor;
    vegetationNodeDescriptor._serialize = false;
    vegetationNodeDescriptor._usageContext = NodeUsageContext::NODE_STATIC;
    vegetationNodeDescriptor._componentMask = to_base(ComponentType::TRANSFORM) |
        to_base(ComponentType::BOUNDS) |
        to_base(ComponentType::RENDERING);

    for (TerrainChunk* chunk : _terrainChunks) {
        vegetationNodeDescriptor._node = Attorney::TerrainChunkTerrain::getVegetation(*chunk);
        vegetationNodeDescriptor._name = Util::StringFormat("Vegetation_chunk_%d", chunk->ID());
        vegParent->addChildNode(vegetationNodeDescriptor);
    }

    sgn.get<RigidBodyComponent>()->physicsGroup(PhysicsGroup::GROUP_STATIC);
    sgn.get<RenderingComponent>()->lockLoD(0u);

    SceneNode::postLoad(sgn);
}
U32 Terrain::getBufferOffset(const RenderStagePass& renderStagePass) const noexcept {
    // Shadows use the exact same settings as the forward pass
    if (renderStagePass._stage == RenderStage::SHADOW) {
        return getBufferOffset(RenderStagePass(RenderStage::DISPLAY, RenderPassType::PRE_PASS, 0u));
    } 
  
    return (to_U32(renderStagePass._stage) - 1u)  * descriptor()->maxNodesPerStage();
}

TerrainTessellator& Terrain::getTessellator(const RenderStagePass& renderStagePass) {
    if (renderStagePass._stage == RenderStage::SHADOW) {
        return getTessellator(RenderStagePass(RenderStage::DISPLAY, RenderPassType::PRE_PASS, 0u));
    } 

    const U8 s = to_U8(renderStagePass._stage) - 1;
    const U8 p = to_U8(renderStagePass._passType);
    const U32 i = RenderStagePass::indexForStage(renderStagePass);

    return _terrainTessellators[s][p][i];
}

U32& Terrain::getUpdateCounter(const RenderStagePass& renderStagePass) {
    if (renderStagePass._stage == RenderStage::SHADOW) {
        return getUpdateCounter(RenderStagePass(RenderStage::DISPLAY, RenderPassType::PRE_PASS, 0u));
    }

    const U8 s = to_U8(renderStagePass._stage) - 1;
    const U8 p = to_U8(renderStagePass._passType);
    const U32 i = RenderStagePass::indexForStage(renderStagePass);

    return _bufferUpdateCounter[s][p][i];
}


F32 Terrain::getTriangleWidth(const RenderStagePass& renderStagePass)  const noexcept {
    const F32 triangleWidth = to_F32(_descriptor->tessellatedTriangleWidth());
    // Lower detail for reflections and refraction
    if (renderStagePass._stage == RenderStage::REFLECTION || renderStagePass._stage == RenderStage::REFRACTION) {
        return triangleWidth * 1.5f;
    }

    return triangleWidth;
}

void Terrain::onEditorChange(std::string_view field) {
    ACKNOWLEDGE_UNUSED(field);

    if (field == "Toggle Quadtree Bounds") {
        toggleBoundingBoxes();
    } else {
        _editorDataDirtyState = EditorDataState::QUEUED;
    }
}

void Terrain::postBuild() {
    TerrainTessellator::Configuration config = {};
    config._minPatchSize = _descriptor->tessellationSettings().y;
    config._terrainDimensions = _descriptor->dimensions();

    for (U8 s = 0; s < to_U8(RenderStage::COUNT); ++s) {
        if (s == to_U8(RenderStage::SHADOW)) {
            continue;
        }

        UpdateCounterPerPassType& perPassCounter = _bufferUpdateCounter[s - 1];
        TessellatorsPerPassType& perPassTess = _terrainTessellators[s - 1];

        for (U8 p = 0; p < to_U8(RenderPassType::COUNT); ++p) {
            const U8 count = RenderStagePass::passCountForStage(
                RenderStagePass{
                    static_cast<RenderStage>(s),
                    static_cast<RenderPassType>(p)
                }
            );

            perPassCounter[p].resize(count, g_bufferFrameDelay);
            perPassTess[p].resize(count);
            for (auto& tess : perPassTess[p]) {
                tess.overrideConfig(config);
            }
        }
    }


    const U16 terrainWidth = _descriptor->dimensions().width;
    const U16 terrainHeight = _descriptor->dimensions().height;

    reserveTriangleCount((terrainWidth - 1) * (terrainHeight - 1) * 2);

    // Generate index buffer
    vectorEASTL<vec3<U32>>& triangles = getTriangles();
    triangles.resize(to_size(terrainHeight) * terrainWidth * 2u);

    // ToDo: Use parallel_for for this
    I32 vectorIndex = 0;
    for (U16 height = 0; height < (terrainHeight - 1); ++height) {
        for (U16 width = 0; width < (terrainWidth - 1); ++width) {
            const U16 vertexIndex = TER_COORD(width, height, terrainWidth);
            // Top triangle (T0)
            triangles[vectorIndex++].set(vertexIndex, vertexIndex + terrainWidth + 1, vertexIndex + 1);
            // Bottom triangle (T1)
            triangles[vectorIndex++].set(vertexIndex, vertexIndex + terrainWidth, vertexIndex + terrainWidth + 1);
        }
    }

    // Approximate bounding box
    const F32 halfWidth = terrainWidth * 0.5f;
    _boundingBox.setMin(-halfWidth, _descriptor->altitudeRange().min, -halfWidth);
    _boundingBox.setMax(halfWidth, _descriptor->altitudeRange().max, halfWidth);

    const U32 chunkSize = to_U32(_descriptor->tessellationSettings().x);

    _terrainQuadtree.build(_boundingBox, _descriptor->dimensions(), chunkSize, this);

    // The terrain's final bounding box is the QuadTree's root bounding box
    _boundingBox.set(_terrainQuadtree.computeBoundingBox());

    vectorEASTLFast<U32> indices;
    CreateTileQuadListIB(indices);

    VertexBuffer* vb = getGeometryVB();
    vb->resizeVertexCount(SQUARED(Terrain::VTX_PER_TILE_EDGE));
    vb->addIndices(indices, false);
    vb->keepData(false);
    vb->create(true);
}

void Terrain::toggleBoundingBoxes() {
    _terrainQuadtree.toggleBoundingBoxes();
    _drawCommandsDirty = true;
}

void Terrain::sceneUpdate(const U64 deltaTimeUS, SceneGraphNode& sgn, SceneState& sceneState) {
    OPTICK_EVENT();

    if (_drawDistanceChanged) {
        _drawDistanceChanged = false;
    }

    const F32 newDrawDistance = sceneState.renderState().generalVisibility();
    if (newDrawDistance != _drawDistance) {
        _drawDistance = newDrawDistance;
        _drawDistanceChanged = true;
    }

    if (_shaderDataDirty) {
        _shaderData->incQueue();
        _shaderDataDirty = false;
    }
    switch (_editorDataDirtyState) {
        case EditorDataState::QUEUED:
            _editorDataDirtyState = EditorDataState::CHANGED;
            break;
        case EditorDataState::PROCESSED:
            _editorDataDirtyState = EditorDataState::IDLE;
            break;
    };
    Object3D::sceneUpdate(deltaTimeUS, sgn, sceneState);
}

void Terrain::onRefreshNodeData(const SceneGraphNode& sgn,
                                const RenderStagePass& renderStagePass,
                                const Camera& crtCamera,
                                bool refreshData,
                                GFX::CommandBuffer& bufferInOut) {
    if (_drawCommandsDirty) {
        rebuildDrawCommands(true);
        _drawCommandsDirty = false;
    }

    if (_shaderDataDirty) {
        GFX::MemoryBarrierCommand memCmd = {};
        memCmd._barrierMask = to_U32(MemoryBarrierType::SHADER_STORAGE);
        GFX::EnqueueCommand(bufferInOut, memCmd);
    }

}

bool Terrain::prepareRender(SceneGraphNode& sgn,
                            RenderingComponent& rComp,
                            const RenderStagePass& renderStagePass,
                            const Camera& camera,
                            bool refreshData) {
    RenderPackage& pkg = rComp.getDrawPackage(renderStagePass);
    if (_editorDataDirtyState == EditorDataState::CHANGED || _editorDataDirtyState == EditorDataState::PROCESSED) {
        rComp.getMaterialInstance()->parallaxFactor(_descriptor->parallaxHeightScale());
        if (!pkg.empty()) {
            PushConstants constants = pkg.pushConstants(0);
            constants.set(_ID("tessTriangleWidth"), GFX::PushConstantType::FLOAT, getTriangleWidth(renderStagePass));
            pkg.pushConstants(0, constants);
            _editorDataDirtyState = EditorDataState::PROCESSED;
        }
    }

    TerrainTessellator& tessellator = getTessellator(renderStagePass);

    U16 depth = tessellator.getRenderDepth();
    if (refreshData && renderStagePass._stage != RenderStage::SHADOW) {
        const Frustum& frustum = camera.getFrustum();
        const vec3<F32>& crtPos = sgn.get<TransformComponent>()->getPosition();

        bool update = false;
        const bool cameraMoved = tessellator.getOrigin() != crtPos || tessellator.getFrustum() != frustum;

        U32& updateCounter = getUpdateCounter(renderStagePass);
        if (cameraMoved || _drawDistanceChanged) {
            updateCounter = g_bufferFrameDelay;
            update = true;
        } else  if (updateCounter > 0) {
            update = true;
            --updateCounter;
        }

        if (update) {
            const TerrainTessellator::RenderData& data = cameraMoved ? tessellator.createTree(frustum, camera.getEye(), crtPos, _drawDistance, depth)
                                                                     : tessellator.renderData();

            _shaderData->writeData(getBufferOffset(renderStagePass), depth, (bufferPtr)data.data());
            _shaderDataDirty = true;
        }
    }

    GenericDrawCommand cmd = pkg.drawCommand(0, 0);
    if (cmd._cmd.primCount != depth) {
        cmd._cmd.primCount = depth;
        pkg.drawCommand(0, 0, cmd);
    }

    return Object3D::prepareRender(sgn, rComp, renderStagePass, camera, refreshData);
}

void Terrain::buildDrawCommands(SceneGraphNode& sgn,
                                const RenderStagePass& renderStagePass,
                                const Camera& crtCamera,
                                RenderPackage& pkgInOut) {

    ShaderBufferBinding buffer = {};
    buffer._binding = ShaderBufferLocation::TERRAIN_DATA;
    buffer._buffer = _shaderData;
    buffer._elementRange = { getBufferOffset(renderStagePass), descriptor()->maxNodesPerStage() };
    pkgInOut.addShaderBuffer(0, buffer);

    GFX::SendPushConstantsCommand pushConstantsCommand = {};
    pushConstantsCommand._constants.set(_ID("tessTriangleWidth"), GFX::PushConstantType::FLOAT, getTriangleWidth(renderStagePass));

    GenericDrawCommand cmd = {};
    enableOption(cmd, CmdRenderOptions::RENDER_INDIRECT);
    cmd._bufferIndex = renderStagePass.baseIndex();
    cmd._primitiveType = PrimitiveType::PATCH;
    cmd._cmd.indexCount = to_U32(Terrain::QUAD_LIST_INDEX_COUNT);

    cmd._sourceBuffer = getGeometryVB()->handle();
    cmd._cmd.primCount = buffer._elementRange.max;

    pkgInOut.add(pushConstantsCommand);    
    pkgInOut.add(GFX::DrawCommand{ cmd });

    _terrainQuadtree.drawBBox(pkgInOut);

    Object3D::buildDrawCommands(sgn, renderStagePass, crtCamera, pkgInOut);
}

vec3<F32> Terrain::getPositionFromGlobal(F32 x, F32 z, bool smooth) const {
    x -= _boundingBox.getCenter().x;
    z -= _boundingBox.getCenter().z;
    F32 xClamp = (0.5f * _descriptor->dimensions().x) + x;
    F32 zClamp = (0.5f * _descriptor->dimensions().y) - z;
    xClamp /= _descriptor->dimensions().x;
    zClamp /= _descriptor->dimensions().y;
    zClamp = 1 - zClamp;
    vec3<F32> temp = getPosition(xClamp, zClamp, smooth);

    return temp;
}

Terrain::Vert Terrain::getVert(F32 x_clampf, F32 z_clampf, bool smooth) const {
    if (smooth) {
        return getSmoothVert(x_clampf, z_clampf);
    }

    return getVert(x_clampf, z_clampf);
}

Terrain::Vert Terrain::getSmoothVert(F32 x_clampf, F32 z_clampf) const {
    assert(!(x_clampf < .0f || z_clampf < .0f || x_clampf > 1.0f || z_clampf > 1.0f));

    const vec2<U16>& dim = _descriptor->dimensions();
    const vec3<F32>& bbMin = _boundingBox.getMin();
    const vec3<F32>& bbMax = _boundingBox.getMax();

    vec2<F32> posF(x_clampf * dim.x, z_clampf * dim.y);
    vec2<I32> posI(to_I32(posF.x), to_I32(posF.y));
    vec2<F32> posD(posF.x - posI.x, posF.y - posI.y);

    if (posI.x >= (I32)dim.x - 1) {
        posI.x = dim.x - 2;
    }

    if (posI.y >= (I32)dim.y - 1) {
        posI.y = dim.y - 2;
    }

    assert(posI.x >= 0 && posI.x < to_I32(dim.x) - 1 && posI.y >= 0 && posI.y < to_I32(dim.y) - 1);

    const VertexBuffer::Vertex& tempVert1 = _physicsVerts[TER_COORD(posI.x, posI.y, to_I32(dim.x))];
    const VertexBuffer::Vertex& tempVert2 = _physicsVerts[TER_COORD(posI.x + 1, posI.y, to_I32(dim.x))];
    const VertexBuffer::Vertex& tempVert3 = _physicsVerts[TER_COORD(posI.x, posI.y + 1, to_I32(dim.x))];
    const VertexBuffer::Vertex& tempVert4 = _physicsVerts[TER_COORD(posI.x + 1, posI.y + 1, to_I32(dim.x))];

    vec3<F32> normals[4];
    Util::UNPACK_VEC3(tempVert1._normal, normals[0]);
    Util::UNPACK_VEC3(tempVert2._normal, normals[1]);
    Util::UNPACK_VEC3(tempVert3._normal, normals[2]);
    Util::UNPACK_VEC3(tempVert4._normal, normals[3]);

    vec3<F32> tangents[4];
    Util::UNPACK_VEC3(tempVert1._tangent, tangents[0]);
    Util::UNPACK_VEC3(tempVert2._tangent, tangents[1]);
    Util::UNPACK_VEC3(tempVert3._tangent, tangents[2]);
    Util::UNPACK_VEC3(tempVert4._tangent, tangents[3]);

    Vert ret = {};
    ret._position.set(
        // X
        bbMin.x + x_clampf * (bbMax.x - bbMin.x),

        //Y
        tempVert1._position.y *  (1.0f - posD.x) * (1.0f - posD.y) +
        tempVert2._position.y * posD.x * (1.0f - posD.y) +
        tempVert3._position.y * (1.0f - posD.x) * posD.y +
        tempVert4._position.y * posD.x * posD.y,

        //Z
        bbMin.z + z_clampf * (bbMax.z - bbMin.z));

    ret._normal.set(normals[0] * (1.0f - posD.x) * (1.0f - posD.y) +
                    normals[1] * posD.x * (1.0f - posD.y) +
                    normals[2] * (1.0f - posD.x) * posD.y +
                    normals[3] * posD.x * posD.y);

    ret._tangent.set(tangents[0] * (1.0f - posD.x) * (1.0f - posD.y) +
                        tangents[1] * posD.x * (1.0f - posD.y) +
                        tangents[2] * (1.0f - posD.x) * posD.y +
                        tangents[3] * posD.x * posD.y);
    
    return ret;

}

Terrain::Vert Terrain::getVert(F32 x_clampf, F32 z_clampf) const {
    assert(!(x_clampf < .0f || z_clampf < .0f || x_clampf > 1.0f || z_clampf > 1.0f));

    const vec2<U16>& dim = _descriptor->dimensions();
    vec2<I32> posI(to_I32(x_clampf * dim.x), to_I32(z_clampf * dim.y));

    if (posI.x >= (I32)dim.x - 1) {
        posI.x = dim.x - 2;
    }

    if (posI.y >= (I32)dim.y - 1) {
        posI.y = dim.y - 2;
    }

    assert(posI.x >= 0 && posI.x < to_I32(dim.x) - 1 && posI.y >= 0 && posI.y < to_I32(dim.y) - 1);

    const VertexBuffer::Vertex& tempVert1 = _physicsVerts[TER_COORD(posI.x, posI.y, to_I32(dim.x))];

    Vert ret = {};
    ret._position.set(tempVert1._position);
    Util::UNPACK_VEC3(tempVert1._normal, ret._normal);
    Util::UNPACK_VEC3(tempVert1._tangent, ret._tangent);

    return ret;

}

vec3<F32> Terrain::getPosition(F32 x_clampf, F32 z_clampf, bool smooth) const {
    return getVert(x_clampf, z_clampf, smooth)._position;
}

vec3<F32> Terrain::getNormal(F32 x_clampf, F32 z_clampf, bool smooth) const {
    return getVert(x_clampf, z_clampf, smooth)._normal;
}

vec3<F32> Terrain::getTangent(F32 x_clampf, F32 z_clampf, bool smooth) const {
    return getVert(x_clampf, z_clampf, smooth)._tangent;

}

vec2<U16> Terrain::getDimensions() const {
    return _descriptor->dimensions();
}

vec2<F32> Terrain::getAltitudeRange() const {
    return _descriptor->altitudeRange();
}

void Terrain::getVegetationStats(U32& maxGrassInstances, U32& maxTreeInstances) const {
    for (TerrainChunk* chunk : _terrainChunks) {
        U32 grassInstanceCount = 0u, treeInstanceCount = 0u;
        Attorney::TerrainChunkTerrain::getVegetation(*chunk)->getStats(grassInstanceCount, treeInstanceCount);
        maxGrassInstances = std::max(maxGrassInstances, grassInstanceCount);
        maxTreeInstances = std::max(maxTreeInstances, treeInstanceCount);
    }
}

void Terrain::saveToXML(boost::property_tree::ptree& pt) const {

    pt.put("descriptor", _descriptor->getVariable("descriptor"));
    pt.put("wireframeDebugMode", to_base(_descriptor->wireframeDebug()));
    pt.put("parallaxMappingMode", to_base(_descriptor->parallaxMode()));
    pt.put("waterCaustics", _descriptor->getVariable("waterCaustics"));
    pt.put("underwaterAlbedoTexture", _descriptor->getVariable("underwaterAlbedoTexture"));
    pt.put("underwaterDetailTexture", _descriptor->getVariable("underwaterDetailTexture"));
    pt.put("tileNoiseTexture", _descriptor->getVariable("tileNoiseTexture"));
    pt.put("underwaterTileScale", _descriptor->getVariablef("underwaterTileScale"));
    Object3D::saveToXML(pt);
}

void Terrain::loadFromXML(const boost::property_tree::ptree& pt) {
 
    Object3D::loadFromXML(pt);
}

};