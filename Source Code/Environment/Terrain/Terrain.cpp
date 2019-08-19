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
    I32 g_nRings = 0;

    vectorEASTL<U32> g_indices;
    void CreateTileQuadListIB(vectorEASTL<U32>& indices)
    {
        indices.resize(Terrain::QUAD_LIST_INDEX_COUNT, 0u);
        I32 index = 0;

        // The IB describes one tile of NxN patches.
        // Four vertices per quad, with VTX_PER_TILE_EDGE-1 quads per tile edge.
        for (I32 y = 0; y < Terrain::VTX_PER_TILE_EDGE - 1; ++y)
        {
            const I32 rowStart = y * Terrain::VTX_PER_TILE_EDGE;

            for (I32 x = 0; x < Terrain::VTX_PER_TILE_EDGE - 1; ++x)
            {
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
      _drawBBoxes(false),
      _vegetationGrassNode(nullptr),
      _editorDataDirtyState(EditorDataState::IDLE)
{
    _nodeDataIndex = RenderPassManager::getUniqueNodeDataIndex();
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

    sgn.get<RigidBodyComponent>()->physicsGroup(PhysicsGroup::GROUP_STATIC);
    sgn.get<RenderingComponent>()->setDataIndex(_nodeDataIndex);

    _editorComponent.onChangedCbk([this](EditorComponentField& field) {onEditorChange(field); });

    _editorComponent.registerField(
        "Tessellation Range",
        [this](void* dataOut) {
            std::memcpy(dataOut, _descriptor->getTessellationRange().xy()._v, 2 * sizeof(F32));
        },

        [this](const void* data) {
            vec2<F32> newTess = *(vec2<F32>*)data;
            _descriptor->setTessellationRange(vec4<F32>(newTess, _descriptor->getTessellationRange().z, _descriptor->getTessellationRange().w));
        },
        EditorComponentFieldType::PUSH_TYPE,
        false,
        GFX::PushConstantType::VEC2);

    _editorComponent.registerField(
        "Tessellated Triangle Width",
        &_descriptor->_tessellatedTriangleWidth,
        EditorComponentFieldType::PUSH_TYPE,
        false,
        GFX::PushConstantType::FLOAT);

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

    U32 chunkSize = to_U32(_descriptor->getTessellationRange().z);

    _terrainQuadtree.build(_context,
        _boundingBox,
        _descriptor->getDimensions(),
        chunkSize,
        this);

    // The terrain's final bounding box is the QuadTree's root bounding box
    _boundingBox.set(_terrainQuadtree.computeBoundingBox());

    I32 widths[] = { 0, 16, 16, 16, 16 };
    g_nRings = sizeof(widths) / sizeof(widths[0]) - 1; // widths[0] doesn't define a ring hence -1
    assert(g_nRings <= MAX_RINGS);

    F32 tileWidth = 0.125f;
    for (I32 i = 0; i != g_nRings && i != MAX_RINGS; ++i) {
        _tileRings[i] = std::make_shared<TileRing>(_context, widths[i] / 2, widths[i + 1], tileWidth);
        tileWidth *= 2.0f;
    }

    CreateTileQuadListIB(g_indices);

    GenericVertexData::IndexBuffer idxBuff;
    idxBuff.smallIndices = false;
    idxBuff.count = g_indices.size();
    idxBuff.data = (bufferPtr)(g_indices.data());

    for (I32 i = 0; i != g_nRings && i != MAX_RINGS; ++i) {
        _tileRings[i]->CreateInputLayout(idxBuff);
    }
}

void Terrain::frameStarted(SceneGraphNode& sgn) {
    if (_editorDataDirtyState == EditorDataState::QUEUED) {
        _editorDataDirtyState = EditorDataState::CHANGED;
    } else if (_editorDataDirtyState == EditorDataState::CHANGED) {
        _editorDataDirtyState = EditorDataState::IDLE;
    }
}

void Terrain::sceneUpdate(const U64 deltaTimeUS, SceneGraphNode& sgn, SceneState& sceneState) {
    Object3D::sceneUpdate(deltaTimeUS, sgn, sceneState);
}

bool Terrain::onRender(SceneGraphNode& sgn,
                       const Camera& camera,
                       RenderStagePass renderStagePass,
                       bool refreshData) {

    const U8 stageIndex = to_U8(renderStagePass._stage);

    RenderPackage& pkg = sgn.get<RenderingComponent>()->getDrawPackage(renderStagePass);
    if (_editorDataDirtyState == EditorDataState::CHANGED) {
        PushConstants constants = pkg.pushConstants(0);
        constants.set("tessellationRange", GFX::PushConstantType::VEC2, _descriptor->getTessellationRange().xy());
        constants.set("tessTriangleWidth", GFX::PushConstantType::FLOAT, _descriptor->getTessellatedTriangleWidth());
        constants.set("height_scale", GFX::PushConstantType::FLOAT, 0.3f);
        pkg.pushConstants(0, constants);
    }

    return Object3D::onRender(sgn, camera, renderStagePass, refreshData);
}

void Terrain::buildDrawCommands(SceneGraphNode& sgn,
                                RenderStagePass renderStagePass,
                                RenderPackage& pkgInOut) {

    GFX::SendPushConstantsCommand pushConstantsCommand = {};
    pushConstantsCommand._constants.set("tessTriangleWidth", GFX::PushConstantType::FLOAT, _descriptor->getTessellatedTriangleWidth());
    pushConstantsCommand._constants.set("height_scale", GFX::PushConstantType::FLOAT, 0.3f);

    GenericDrawCommand cmd = {};
    enableOption(cmd, CmdRenderOptions::RENDER_INDIRECT);
    enableOption(cmd, CmdRenderOptions::RENDER_TESSELLATED);
    cmd._primitiveType = PrimitiveType::PATCH;
    cmd._patchVertexCount = 4;

    for (I32 i = 0; i < g_nRings; ++i) {
        pushConstantsCommand._constants.set("g_tileSize", GFX::PushConstantType::FLOAT, _tileRings[i]->tileSize());
        pkgInOut.addPushConstantsCommand(pushConstantsCommand);

        cmd._cmd.primCount  = _tileRings[i]->nTiles();
        cmd._sourceBuffer   = _tileRings[i]->getBuffer();
        cmd._cmd.indexCount = to_U32(_tileRings[i]->getBuffer()->indexBuffer().count);

        GFX::DrawCommand drawCommand = {};
        drawCommand._drawCommands.push_back(cmd);
        pkgInOut.addDrawCommand(drawCommand);
    }

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