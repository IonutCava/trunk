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

    void CreateTileQuadListIB(vector<U32>& indices)
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

Terrain::Terrain(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name)
    : Object3D(context, parentCache, descriptorHash, name, ObjectType::TERRAIN),
      _shaderData(nullptr),
      _drawBBoxes(false),
      _shaderDataDirty(false),
      _vegetationGrassNode(nullptr),
      _initBufferWriteCounter(0),
      _drawDistance(0.0f),
      _editorDataDirtyState(EditorDataState::IDLE)
{
    _nodeDataIndex = RenderPassManager::getUniqueNodeDataIndex();

    TerrainTessellator::Configuration shadowConfig = {};
    shadowConfig._useCameraDistance = false;

    constexpr U32 passPerLight = ShadowMap::MAX_SHADOW_PASSES / Config::Lighting::MAX_SHADOW_CASTING_LIGHTS;
    for (U32 i = 0; i < ShadowMap::MAX_SHADOW_PASSES; ++i) {
        if (i % passPerLight > 0) {
            _shadowTessellators[i].overrideConfig(shadowConfig);
        }
    }
}

Terrain::~Terrain()
{
}

bool Terrain::unload() noexcept {
    return Object3D::unload();
}

void Terrain::postLoad(SceneGraphNode& sgn) {

    SceneGraphNodeDescriptor vegetationParentNode;
    vegetationParentNode._serialize = false;
    vegetationParentNode._name = "Vegetation";
    vegetationParentNode._usageContext = NodeUsageContext::NODE_STATIC;
    vegetationParentNode._componentMask = to_base(ComponentType::TRANSFORM) | to_base(ComponentType::BOUNDS);
    SceneGraphNode* vegParent = sgn.addNode(vegetationParentNode);
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
        vegParent->addNode(vegetationNodeDescriptor);
    }

    static_assert(MAX_RENDER_NODES * sizeof(TessellatedNodeData) < 64 * 1024 * 1024, "Too many terrain nodes to fit in an UBO!");

    _initBufferWriteCounter = ((to_base(RenderStage::COUNT) - 1) + ShadowMap::MAX_SHADOW_PASSES);
    _initBufferWriteCounter *= g_bufferFrameDelay;

    ShaderBufferDescriptor bufferDescriptor;
    bufferDescriptor._elementCount = Terrain::MAX_RENDER_NODES * ((to_base(RenderStage::COUNT) - 1) + ShadowMap::MAX_SHADOW_PASSES);
    bufferDescriptor._elementSize = sizeof(TessellatedNodeData);
    bufferDescriptor._ringBufferLength = g_bufferFrameDelay;
    bufferDescriptor._separateReadWrite = false;
    bufferDescriptor._usage = ShaderBuffer::Usage::CONSTANT_BUFFER;
    bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::ALLOW_THREADED_WRITES);
                              
    //Should be once per frame
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::OFTEN;

    bufferDescriptor._name = "TERRAIN_RENDER_NODES";
    _shaderData = _context.newSB(bufferDescriptor);

    sgn.get<RigidBodyComponent>()->physicsGroup(PhysicsGroup::GROUP_STATIC);
    sgn.get<RenderingComponent>()->setDataIndex(_nodeDataIndex);

    _editorComponent.onChangedCbk([this](EditorComponentField& field) {onEditorChange(field); });
    _editorComponent.registerField(
        "Tessellated Triangle Width",
        &_descriptor->_tessellatedTriangleWidth,
        EditorComponentFieldType::SLIDER_TYPE,
        false,
        GFX::PushConstantType::FLOAT,
        {1.0f, 150.0f},
        1.0f);
    _editorComponent.registerField(
        "Parallax Height",
        &_descriptor->_parallaxHeightScale,
        EditorComponentFieldType::SLIDER_TYPE,
        false,
        GFX::PushConstantType::FLOAT,
        { 0.01f, 10.0f });
    SceneNode::postLoad(sgn);
}

void Terrain::onEditorChange(EditorComponentField& field) {
    _editorDataDirtyState = EditorDataState::QUEUED;
}

void Terrain::postBuild() {
    const U16 terrainWidth = _descriptor->getDimensions().width;
    const U16 terrainHeight = _descriptor->getDimensions().height;

    reserveTriangleCount((terrainWidth - 1) * (terrainHeight - 1) * 2);

    // Generate index buffer
    vectorEASTL<vec3<U32>>& triangles = getTriangles();
    triangles.resize(terrainHeight * terrainWidth * 2u);

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
    F32 halfWidth = terrainWidth * 0.5f;
    _boundingBox.setMin(-halfWidth, _descriptor->getAltitudeRange().min, -halfWidth);
    _boundingBox.setMax(halfWidth, _descriptor->getAltitudeRange().max, halfWidth);

    U32 chunkSize = to_U32(_descriptor->getTessellationSettings().x);

    _terrainQuadtree.build(_context,
        _boundingBox,
        _descriptor->getDimensions(),
        chunkSize,
        this);

    // The terrain's final bounding box is the QuadTree's root bounding box
    _boundingBox.set(_terrainQuadtree.computeBoundingBox());

    vector<U32> indices;
    CreateTileQuadListIB(indices);

    VertexBuffer* vb = getGeometryVB();
    vb->resizeVertexCount(SQUARED(Terrain::VTX_PER_TILE_EDGE));
    vb->addIndices(indices, false);
    vb->keepData(false);
    vb->create(true);
}

void Terrain::frameStarted(SceneGraphNode& sgn) {
    if (_editorDataDirtyState == EditorDataState::QUEUED) {
        _editorDataDirtyState = EditorDataState::CHANGED;
    } else if (_editorDataDirtyState == EditorDataState::CHANGED) {
        _editorDataDirtyState = EditorDataState::IDLE;
    }
}

void Terrain::sceneUpdate(const U64 deltaTimeUS, SceneGraphNode& sgn, SceneState& sceneState) {
    _drawDistance = sceneState.renderState().generalVisibility();
    if (_shaderDataDirty) {
        _shaderData->incQueue();
        _shaderDataDirty = false;
    }

    Object3D::sceneUpdate(deltaTimeUS, sgn, sceneState);
}

bool Terrain::preRender(SceneGraphNode& sgn,
                        const Camera& camera,
                        RenderStagePass renderStagePass,
                        bool refreshData,
                        bool& rebuildCommandsOut) {
    //ToDo: If we swap shaders (e.g. for wireframe debug), mark rebuild as true to rebuild the pipeline
    return SceneNode::preRender(sgn, camera, renderStagePass, refreshData, rebuildCommandsOut);
}

bool Terrain::onRender(SceneGraphNode& sgn,
                       const Camera& camera,
                       RenderStagePass renderStagePass,
                       bool refreshData) {

    const U8 stageIndex = to_U8(renderStagePass._stage);

    U32 offset = 0;
    TerrainTessellator* tessellator = nullptr;

    if (renderStagePass._stage == RenderStage::SHADOW) {
        offset = Terrain::MAX_RENDER_NODES * renderStagePass._passIndex;
        tessellator = &_shadowTessellators[renderStagePass._passIndex];
    } else {
        offset = ShadowMap::MAX_SHADOW_PASSES * Terrain::MAX_RENDER_NODES;
        offset += (to_U32((stageIndex - 1) * Terrain::MAX_RENDER_NODES));
        tessellator = &_terrainTessellator[stageIndex - 1];
    }

    RenderPackage& pkg = sgn.get<RenderingComponent>()->getDrawPackage(renderStagePass);
    if (_editorDataDirtyState == EditorDataState::CHANGED) {
        PushConstants constants = pkg.pushConstants(0);
        constants.set("tessTriangleWidth", GFX::PushConstantType::FLOAT, std::ceil(_descriptor->getTessellatedTriangleWidth()));
        constants.set("height_scale", GFX::PushConstantType::FLOAT, _descriptor->getParallaxHeightScale());
        pkg.pushConstants(0, constants);
    }

    U16 depth = tessellator->getRenderDepth();
    if (refreshData) {
        const Frustum& frustum = camera.getFrustum();
        const vec3<F32>& crtPos = sgn.get<TransformComponent>()->getPosition();

        bool update = tessellator->getOrigin() != crtPos || tessellator->getFrustum() != frustum;
        if (_initBufferWriteCounter > 0) {
            update = true;
            _initBufferWriteCounter--;
        }

        if (update)
        {
            tessellator->createTree(camera.getEye(), frustum, crtPos, _descriptor->getDimensions(), _descriptor->getTessellationSettings().y);
            U8 LoD = (renderStagePass._stage == RenderStage::REFLECTION || renderStagePass._stage == RenderStage::REFRACTION) ? 1 : 0;

            bufferPtr data = (bufferPtr)tessellator->updateAndGetRenderData(depth, LoD);

            _shaderData->writeData(offset, depth, data);
            _shaderDataDirty = true;
        }
    }
     
    GenericDrawCommand cmd = pkg.drawCommand(0, 0);
    if (cmd._cmd.primCount != depth) {
        cmd._cmd.primCount = depth;
        pkg.drawCommand(0, 0, cmd);
    }

    return Object3D::onRender(sgn, camera, renderStagePass, refreshData);
}

void Terrain::buildDrawCommands(SceneGraphNode& sgn,
                                RenderStagePass renderStagePass,
                                RenderPackage& pkgInOut) {

    U32 offset = 0;
    if (renderStagePass._stage == RenderStage::SHADOW) {
        offset = Terrain::MAX_RENDER_NODES * renderStagePass._passIndex;
    } else {
        const U8 stageIndex = to_U8(renderStagePass._stage);
        offset = ShadowMap::MAX_SHADOW_PASSES * Terrain::MAX_RENDER_NODES;
        offset += (to_U32((stageIndex - 1) * Terrain::MAX_RENDER_NODES));
    }

    ShaderBufferBinding buffer = {};
    buffer._binding = ShaderBufferLocation::TERRAIN_DATA;
    buffer._buffer = _shaderData;
    buffer._elementRange = { offset, MAX_RENDER_NODES };

    pkgInOut.addShaderBuffer(0, buffer);

    GFX::SendPushConstantsCommand pushConstantsCommand = {};
    pushConstantsCommand._constants.set("tessTriangleWidth", GFX::PushConstantType::FLOAT, _descriptor->getTessellatedTriangleWidth());
    pushConstantsCommand._constants.set("height_scale", GFX::PushConstantType::FLOAT, 0.3f);

    GenericDrawCommand cmd = {};
    enableOption(cmd, CmdRenderOptions::RENDER_INDIRECT);
    cmd._bufferIndex = renderStagePass.index();
    cmd._primitiveType = PrimitiveType::PATCH;
    cmd._cmd.indexCount = to_U32(Terrain::QUAD_LIST_INDEX_COUNT);

    cmd._sourceBuffer = getGeometryVB()->handle();
    cmd._cmd.primCount = Terrain::MAX_RENDER_NODES;

    pkgInOut.addPushConstantsCommand(pushConstantsCommand);    
    GFX::DrawCommand drawCommand = {cmd};
    pkgInOut.addDrawCommand(drawCommand);

    Object3D::buildDrawCommands(sgn, renderStagePass, pkgInOut);
}

vec3<F32> Terrain::getPositionFromGlobal(F32 x, F32 z, bool smooth) const {
    x -= _boundingBox.getCenter().x;
    z -= _boundingBox.getCenter().z;
    F32 xClamp = (0.5f * _descriptor->getDimensions().x) + x;
    F32 zClamp = (0.5f * _descriptor->getDimensions().y) - z;
    xClamp /= _descriptor->getDimensions().x;
    zClamp /= _descriptor->getDimensions().y;
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

    const vec2<U16>& dim = _descriptor->getDimensions();
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

    const vec2<U16>& dim = _descriptor->getDimensions();
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
    return _descriptor->getDimensions();
}

vec2<F32> Terrain::getAltitudeRange() const {
    return _descriptor->getAltitudeRange();
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