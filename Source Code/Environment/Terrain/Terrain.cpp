#include "stdafx.h"

#include "Headers/Terrain.h"
#include "Headers/TileRing.h"
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
#include "Platform/Video/Buffers/VertexBuffer/GenericBuffer/Headers/GenericVertexData.h"

namespace Divide {

namespace {
    // This doesn't seem to work for whatever reason, so using multiple buffers for now as I have
    // more important things to pull my hair over :/
    constexpr bool USE_BASE_VERTEX_OFFSETS = false;

    // This array defines the outer width of each successive terrain ring.
    constexpr I32 g_RingWidths[] = {
        0,
        16,
        16,
        16
    };

    constexpr F32 g_StartTileSize = 0.35f;

    vectorEASTLFast<U16> CreateTileQuadListIB()
    {
        vectorEASTLFast<U16> indices(TessellationParams::QUAD_LIST_INDEX_COUNT, 0u);

        U16 index = 0;

        // The IB describes one tile of NxN patches.
        // Four vertices per quad, with VTX_PER_TILE_EDGE-1 quads per tile edge.
        for (U8 y = 0; y < TessellationParams::VTX_PER_TILE_EDGE - 1; ++y)
        {
            const U16 rowStart = y * TessellationParams::VTX_PER_TILE_EDGE;

            for (U8 x = 0; x < TessellationParams::VTX_PER_TILE_EDGE - 1; ++x) {
                indices[index++] = rowStart + x;
                indices[index++] = rowStart + x + TessellationParams::VTX_PER_TILE_EDGE;
                indices[index++] = rowStart + x + TessellationParams::VTX_PER_TILE_EDGE + 1;
                indices[index++] = rowStart + x + 1;
            }
        }
        assert(index == TessellationParams::QUAD_LIST_INDEX_COUNT);

        return indices;
    }
}

void TessellationParams::fromDescriptor(const std::shared_ptr<TerrainDescriptor>& descriptor) {
    WorldScale((descriptor->dimensions() * 0.5f) / to_F32(PATCHES_PER_TILE_EDGE));
}

Terrain::Terrain(GFXDevice& context, ResourceCache* parentCache, size_t descriptorHash, const Str256& name)
    : Object3D(context, parentCache, descriptorHash, name, name, "", ObjectType::TERRAIN, to_U32(ObjectFlag::OBJECT_FLAG_NO_VB)),
      _terrainQuadtree(context)
{
    _renderState.addToDrawExclusionMask(RenderStage::SHADOW, RenderPassType::COUNT, to_U8(LightType::SPOT));
    _renderState.addToDrawExclusionMask(RenderStage::SHADOW, RenderPassType::COUNT, to_U8(LightType::POINT));
}

void Terrain::postLoad(SceneGraphNode* sgn) {

    if (!_initialSetupDone) {
        _editorComponent.onChangedCbk([this](std::string_view field) {onEditorChange(field); });

        EditorComponentField tessTriangleWidthField = {};
        tessTriangleWidthField._name = "Tessellated Triangle Width";
        tessTriangleWidthField._dataGetter = [&](void* dataOut) {
            *static_cast<U32*>(dataOut) = tessParams().tessellatedTriangleWidth();
        };
        tessTriangleWidthField._dataSetter = [&](const void* data) {
            _tessParams.tessellatedTriangleWidth(*static_cast<const U32*>(data));
        };
        tessTriangleWidthField._type = EditorComponentFieldType::SLIDER_TYPE;
        tessTriangleWidthField._readOnly = false;
        tessTriangleWidthField._serialise = true;
        tessTriangleWidthField._basicType = GFX::PushConstantType::UINT;
        tessTriangleWidthField._range = { 1.0f, 150.0f };
        tessTriangleWidthField._step = 1.0f;

        _editorComponent.registerField(std::move(tessTriangleWidthField));

        EditorComponentField parallaxHeightField = {};
        parallaxHeightField._name = "Parallax Height";
        parallaxHeightField._data = &_parallaxHeightScale;
        parallaxHeightField._type = EditorComponentFieldType::SLIDER_TYPE;
        parallaxHeightField._readOnly = false;
        parallaxHeightField._serialise = true;
        parallaxHeightField._basicType = GFX::PushConstantType::FLOAT;
        parallaxHeightField._range = { 0.01f, 10.0f };

        _editorComponent.registerField(std::move(parallaxHeightField));

        EditorComponentField toggleBoundsField = {};
        toggleBoundsField._name = "Toggle Quadtree Bounds";
        toggleBoundsField._range = { toggleBoundsField._name.length() * 10.0f, 20.0f };//dimensions
        toggleBoundsField._type = EditorComponentFieldType::BUTTON;
        toggleBoundsField._readOnly = false; //disabled/enabled
        _editorComponent.registerField(std::move(toggleBoundsField));

        PlatformContext& pContext = sgn->context();
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
        _initialSetupDone = true;
    }

    SceneGraphNodeDescriptor vegetationParentNode;
    vegetationParentNode._serialize = false;
    vegetationParentNode._name = "Vegetation";
    vegetationParentNode._usageContext = NodeUsageContext::NODE_STATIC;
    vegetationParentNode._componentMask = to_base(ComponentType::TRANSFORM) | to_base(ComponentType::BOUNDS);
    SceneGraphNode* vegParent = sgn->addChildNode(vegetationParentNode);
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

    sgn->get<RigidBodyComponent>()->physicsGroup(PhysicsGroup::GROUP_STATIC);
    sgn->get<RenderingComponent>()->lockLoD(0u);

    SceneNode::postLoad(sgn);
}

void Terrain::onEditorChange(std::string_view field) {
    if (field == "Toggle Quadtree Bounds") {
        toggleBoundingBoxes();
    } else {
        _editorDataDirtyState = EditorDataState::QUEUED;
    }
}

void Terrain::postBuild() {
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

    const U16 chunkSize = _descriptor->chunkSize();

    _terrainQuadtree.build(_boundingBox, _descriptor->dimensions(), chunkSize, this);

    // The terrain's final bounding box is the QuadTree's root bounding box
    _boundingBox.set(_terrainQuadtree.computeBoundingBox());

    {
        // widths[0] doesn't define a ring hence -1
        const size_t ringCount = sizeof(g_RingWidths) / sizeof(g_RingWidths[0]) - 1;
        _tileRings.reserve(ringCount);
        F32 tileWidth = g_StartTileSize;
        for (size_t i = 0; i < ringCount ; ++i) {
            _tileRings.emplace_back(eastl::make_unique<TileRing>(g_RingWidths[i] / 2, g_RingWidths[i + 1], tileWidth));
            tileWidth *= 2.0f;
        }

        // This is a whole fraction of the max tessellation, i.e., 64/N.  The intent is that 
        // the height field scrolls through the terrain mesh in multiples of the polygon spacing.
        // So polygon vertices mostly stay fixed relative to the displacement map and this reduces
        // scintillation.  Without snapping, it scintillates badly.  Additionally, we make the
        // snap size equal to one patch width, purely to stop the patches dancing around like crazy.
        // The non-debug rendering works fine either way, but crazy flickering of the debug patches 
        // makes understanding much harder.
        const vec2<F32> snapGridSize = tessParams().WorldScale() * _tileRings[ringCount - 1]->tileSize();
        _tessParams.SnapGridSize(snapGridSize / TessellationParams::PATCHES_PER_TILE_EDGE);

        vectorEASTLFast<U16> indices = CreateTileQuadListIB();

        { // Create a single buffer to hold the data for all of our tile rings
            Divide::GenericVertexData::IndexBuffer idxBuff = {};
            idxBuff.smallIndices = true;
            idxBuff.count = indices.size();
            idxBuff.data = indices.data();

            Divide::GenericVertexData::SetBufferParams params = {};
            params._elementSize = sizeof(TileRing::InstanceData);
            params._updateFrequency = Divide::BufferUpdateFrequency::ONCE;
            params._updateUsage = Divide::BufferUpdateUsage::CPU_W_GPU_R;
            params._storageType = Divide::BufferStorageType::NORMAL;
            params._instanceDivisor = 1u;
            params._sync = false;

            _terrainBuffer = _context.newGVD(1);
            if_constexpr(USE_BASE_VERTEX_OFFSETS) {
                _terrainBuffer->create(1);
            } else {
                _terrainBuffer->create(ringCount);
            }
            _terrainBuffer->setIndexBuffer(idxBuff, Divide::BufferUpdateFrequency::ONCE);
            if_constexpr(USE_BASE_VERTEX_OFFSETS) {
                vectorEASTL<TileRing::InstanceData> vbData;
                for (size_t i = 0; i < ringCount; ++i) {
                    vectorEASTL<TileRing::InstanceData> ringData = _tileRings[i]->createInstanceDataVB(to_I32(i));
                    vbData.insert(eastl::cend(vbData), eastl::cbegin(ringData), eastl::cend(ringData));
                }
                params._buffer = 0;
                params._data = vbData.data();
                params._elementCount = to_U32(vbData.size());
                _terrainBuffer->setBuffer(params);
            } else {
                for (size_t i = 0; i < ringCount; ++i) {
                    vectorEASTL<TileRing::InstanceData> ringData = _tileRings[i]->createInstanceDataVB(to_I32(i));
                    params._buffer = to_U32(i);
                    params._data = ringData.data();
                    params._elementCount = to_U32(ringData.size());
                    _terrainBuffer->setBuffer(params);
                }
            }
            AttributeDescriptor& descPosition  = _terrainBuffer->attribDescriptor(Divide::to_base(AttribLocation::POSITION));
            AttributeDescriptor& descAdjacency = _terrainBuffer->attribDescriptor(Divide::to_base(AttribLocation::COLOR));
            descPosition.set( 0u, 4, GFXDataFormat::FLOAT_32, false, 0u * sizeof(F32));
            descAdjacency.set(0u, 4, GFXDataFormat::FLOAT_32, false, 4u * sizeof(F32));
        }
    }
}

void Terrain::toggleBoundingBoxes() {
    _terrainQuadtree.toggleBoundingBoxes();
    _drawCommandsDirty = true;
}

void Terrain::sceneUpdate(const U64 deltaTimeUS, SceneGraphNode* sgn, SceneState& sceneState) {
    OPTICK_EVENT();

    _drawDistance = sceneState.renderState().generalVisibility();

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

void Terrain::onRefreshNodeData(const SceneGraphNode* sgn,
                                const RenderStagePass& renderStagePass,
                                const Camera& crtCamera,
                                bool refreshData,
                                GFX::CommandBuffer& bufferInOut) {
    if (_drawCommandsDirty) {
        rebuildDrawCommands(true);
        _drawCommandsDirty = false;
    }
}

bool Terrain::prepareRender(SceneGraphNode* sgn,
                            RenderingComponent& rComp,
                            const RenderStagePass& renderStagePass,
                            const Camera& camera,
                            bool refreshData) {
    RenderPackage& pkg = rComp.getDrawPackage(renderStagePass);
    if (_editorDataDirtyState == EditorDataState::CHANGED || _editorDataDirtyState == EditorDataState::PROCESSED) {
        rComp.getMaterialInstance()->parallaxFactor(parallaxHeightScale());
        _editorDataDirtyState = EditorDataState::PROCESSED;
    }

    if (!pkg.empty()) {
        F32 triangleWidth = to_F32(tessParams().tessellatedTriangleWidth());
        if (renderStagePass._stage == RenderStage::REFLECTION ||
            renderStagePass._stage == RenderStage::REFRACTION)                 
        {
            // Lower the level of detail in reflections and refractions
            triangleWidth *= 1.5f;
        } else if (renderStagePass._stage == RenderStage::SHADOW) {
            triangleWidth *= 2.0f;
        }

        const vec2<F32>& grid  = tessParams().SnapGridSize();
        const vec2<F32>& scale = tessParams().WorldScale();

        const vec2<F32> eye = camera.getEye().xz();

        vec2<F32> snapped = eye;
        vec2<F32> offset  = eye;
        for (U8 i = 0; i < 2; ++i) {
            if (grid[i] > 0.f) {
                snapped[i] = std::floorf(snapped[i] / grid[i]) * grid[i];
                offset[i]  = std::floorf(offset[i] / grid[i]) * grid[i];
            }
        }
#if 0
        // Why the 2x?  I'm confused.  But it works.
        snapped = eye - ((eye - snapped)/* * 2*/);
#else
        // The 2x DOES NOT WORK here. Causes awful "swimming" artifacts. Now I'm confused ... -Ionut
        snapped = eye - (eye - snapped);
#endif

        PushConstants& constants = pkg.pushConstants(0);
        constants.set(_ID("dvd_tessTriangleWidth"),  GFX::PushConstantType::FLOAT, triangleWidth);
        constants.set(_ID("dvd_tileWorldPosition"),  GFX::PushConstantType::VEC2,  snapped);
        constants.set(_ID("dvd_textureWorldOffset"), GFX::PushConstantType::VEC2,  offset / scale);
    }

    return Object3D::prepareRender(sgn, rComp, renderStagePass, camera, refreshData);
}

void Terrain::buildDrawCommands(SceneGraphNode* sgn,
                                const RenderStagePass& renderStagePass,
                                const Camera& crtCamera,
                                RenderPackage& pkgInOut) {

    const F32 triangleWidth = to_F32(tessParams().tessellatedTriangleWidth());
    GFX::SendPushConstantsCommand pushConstantsCommand = {};
    pushConstantsCommand._constants.set(_ID("dvd_tessTriangleWidth"),  GFX::PushConstantType::FLOAT, triangleWidth);
    pushConstantsCommand._constants.set(_ID("dvd_tileWorldPosition"),  GFX::PushConstantType::VEC2,  VECTOR2_ZERO);
    pushConstantsCommand._constants.set(_ID("dvd_textureWorldOffset"), GFX::PushConstantType::VEC2,  VECTOR2_ZERO);
    pkgInOut.add(pushConstantsCommand);

    GenericDrawCommand cmd = {};
    enableOption(cmd, CmdRenderOptions::RENDER_INDIRECT);

    cmd._primitiveType = PrimitiveType::PATCH;
    cmd._sourceBuffer = _terrainBuffer->handle();
    cmd._cmd.indexCount = to_U32(TessellationParams::QUAD_LIST_INDEX_COUNT);
    cmd._bufferIndex = 0u;

    for (const auto& tileRing : _tileRings) {
        cmd._cmd.primCount = tileRing->tileCount();
        pkgInOut.add(GFX::DrawCommand{ cmd });
        if_constexpr(USE_BASE_VERTEX_OFFSETS) {
            cmd._cmd.baseVertex += cmd._cmd.primCount;
        } else {
            ++cmd._bufferIndex;
        }
    }

    _terrainQuadtree.drawBBox(pkgInOut);

    Object3D::buildDrawCommands(sgn, renderStagePass, crtCamera, pkgInOut);
}

Terrain::Vert Terrain::getVertFromGlobal(F32 x, F32 z, bool smooth) const {
    x -= _boundingBox.getCenter().x;
    z -= _boundingBox.getCenter().z;
    const vec2<U16>& dim = _descriptor->dimensions();
    const F32 xClamp = ((0.5f * dim.width) + x) / dim.width;
    const F32 zClamp = ((0.5f * dim.height) - z) / dim.height;
    return getVert(xClamp, 1 - zClamp, smooth);
}

Terrain::Vert Terrain::getVert(F32 x_clampf, F32 z_clampf, bool smooth) const {
    return smooth ? getSmoothVert(x_clampf, z_clampf)
                  : getVert(x_clampf, z_clampf);
}

Terrain::Vert Terrain::getSmoothVert(F32 x_clampf, F32 z_clampf) const {
    assert(!(x_clampf < 0.0f || z_clampf < 0.0f || 
             x_clampf > 1.0f || z_clampf > 1.0f));

    const vec2<U16>& dim   = _descriptor->dimensions();
    const vec3<F32>& bbMin = _boundingBox.getMin();
    const vec3<F32>& bbMax = _boundingBox.getMax();

    const vec2<F32> posF(x_clampf * dim.width,    z_clampf * dim.height);
          vec2<I32> posI(to_I32(posF.width),      to_I32(posF.height));
    const vec2<F32> posD(posF.width - posI.width, posF.height - posI.height);

    if (posI.width >= to_I32(dim.width) - 1) {
        posI.width = dim.width - 2;
    }

    if (posI.height >= to_I32(dim.height) - 1) {
        posI.height = dim.height - 2;
    }

    assert(posI.width  >= 0 && posI.width  < to_I32(dim.width)  - 1 &&
           posI.height >= 0 && posI.height < to_I32(dim.height) - 1);

    const VertexBuffer::Vertex& tempVert1 = _physicsVerts[TER_COORD(posI.width,     posI.height,     to_I32(dim.width))];
    const VertexBuffer::Vertex& tempVert2 = _physicsVerts[TER_COORD(posI.width + 1, posI.height,     to_I32(dim.width))];
    const VertexBuffer::Vertex& tempVert3 = _physicsVerts[TER_COORD(posI.width,     posI.height + 1, to_I32(dim.width))];
    const VertexBuffer::Vertex& tempVert4 = _physicsVerts[TER_COORD(posI.width + 1, posI.height + 1, to_I32(dim.width))];

    const vec3<F32> normals[4]{
        Util::UNPACK_VEC3(tempVert1._normal),
        Util::UNPACK_VEC3(tempVert2._normal),
        Util::UNPACK_VEC3(tempVert3._normal),
        Util::UNPACK_VEC3(tempVert4._normal)
    };

    const vec3<F32> tangents[4]{
        Util::UNPACK_VEC3(tempVert1._tangent),
        Util::UNPACK_VEC3(tempVert2._tangent),
        Util::UNPACK_VEC3(tempVert3._tangent),
        Util::UNPACK_VEC3(tempVert4._tangent)
    };

    Vert ret = {};
    ret._position.set(
        // X
        bbMin.x + x_clampf * (bbMax.x - bbMin.x),

        //Y
        tempVert1._position.y * (1.0f - posD.width) * (1.0f - posD.height) +
        tempVert2._position.y *         posD.width  * (1.0f - posD.height) +
        tempVert3._position.y * (1.0f - posD.width) *         posD.height +
        tempVert4._position.y *         posD.width  *         posD.height,

        //Z
        bbMin.z + z_clampf * (bbMax.z - bbMin.z));

    ret._normal.set(normals[0] * (1.0f - posD.width) * (1.0f - posD.height) +
                    normals[1] *         posD.width  * (1.0f - posD.height) +
                    normals[2] * (1.0f - posD.width) *         posD.height +
                    normals[3] *         posD.width  *         posD.height);

    ret._tangent.set(tangents[0] * (1.0f - posD.width) * (1.0f - posD.height) +
                     tangents[1] *         posD.width  * (1.0f - posD.height) +
                     tangents[2] * (1.0f - posD.width) *         posD.height +
                     tangents[3] *         posD.width  *         posD.height);
    
    return ret;

}

Terrain::Vert Terrain::getVert(F32 x_clampf, F32 z_clampf) const {
    assert(!(x_clampf < 0.0f || z_clampf < 0.0f ||
             x_clampf > 1.0f || z_clampf > 1.0f));

    const vec2<U16>& dim = _descriptor->dimensions();
    
    vec2<I32> posI(to_I32(x_clampf * dim.width), 
                   to_I32(z_clampf * dim.height));

    if (posI.width >= to_I32(dim.width) - 1) {
        posI.width = dim.width - 2;
    }

    if (posI.height >= to_I32(dim.height) - 1) {
        posI.height = dim.height - 2;
    }

    assert(posI.width  >= 0 && posI.width  < to_I32(dim.width)  - 1 &&
           posI.height >= 0 && posI.height < to_I32(dim.height) - 1);

    const VertexBuffer::Vertex& tempVert1 = _physicsVerts[TER_COORD(posI.width, posI.height, to_I32(dim.width))];

    Vert ret = {};
    ret._position.set(tempVert1._position);
    ret._normal.set(Util::UNPACK_VEC3(tempVert1._normal));
    ret._tangent.set(Util::UNPACK_VEC3(tempVert1._tangent));

    return ret;

}

vec2<U16> Terrain::getDimensions() const noexcept {
    return _descriptor->dimensions();
}

vec2<F32> Terrain::getAltitudeRange() const noexcept {
    return _descriptor->altitudeRange();
}

void Terrain::getVegetationStats(U32& maxGrassInstances, U32& maxTreeInstances) const {
    U32 grassInstanceCount = 0u, treeInstanceCount = 0u;
    for (TerrainChunk* chunk : _terrainChunks) {
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