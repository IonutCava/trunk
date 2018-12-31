#include "stdafx.h"

#include "Headers/Terrain.h"
#include "Headers/TerrainChunk.h"
#include "Headers/TerrainDescriptor.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Managers/Headers/SceneManager.h"
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

Terrain::Terrain(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name)
    : Object3D(context, parentCache, descriptorHash, name, ObjectType::TERRAIN),
      _shaderData(nullptr),
      _drawBBoxes(false),
      _drawDistance(0.0f),
      _editorDataDirtyState(EditorDataState::IDLE)
{
}

Terrain::~Terrain()
{
}

bool Terrain::unload() {
    MemoryManager::DELETE(_terrainTextures);
    return Object3D::unload();
}

void Terrain::postLoad(SceneGraphNode& sgn) {

    SceneGraphNodeDescriptor vegetationNodeDescriptor;
    vegetationNodeDescriptor._usageContext = NodeUsageContext::NODE_STATIC;
    vegetationNodeDescriptor._componentMask = to_base(ComponentType::TRANSFORM) |
                                              to_base(ComponentType::BOUNDS) |
                                              to_base(ComponentType::RENDERING);

    for (TerrainChunk* chunk : _terrainChunks) {
        vegetationNodeDescriptor._node = Attorney::TerrainChunkTerrain::getVegetation(*chunk);
        vegetationNodeDescriptor._name = Util::StringFormat("Grass_chunk_%d", chunk->ID());
        sgn.addNode(vegetationNodeDescriptor);
    }

    ShaderBufferDescriptor bufferDescriptor;
    bufferDescriptor._elementCount = Terrain::MAX_RENDER_NODES * to_base(RenderStage::COUNT);
    bufferDescriptor._elementSize = sizeof(TessellatedNodeData);
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._flags = USE_TERRAIN_UBO ? to_U32(ShaderBuffer::Flags::NONE) 
                                              : (to_U32(ShaderBuffer::Flags::UNBOUND_STORAGE) |
                                                 to_U32(ShaderBuffer::Flags::AUTO_RANGE_FLUSH) |
                                                 to_U32(ShaderBuffer::Flags::ALLOW_THREADED_WRITES));
                              
    //Should be once per frame
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::OCASSIONAL;

    bufferDescriptor._name = "TERRAIN_RENDER_NODES";
    _shaderData = _context.newSB(bufferDescriptor);

    sgn.get<RigidBodyComponent>()->physicsGroup(PhysicsGroup::GROUP_STATIC);

    _editorComponent.onChangedCbk([this](EditorComponentField& field) {onEditorChange(field); });

    _editorComponent.registerField(
        "Tessellation Range",
        [this]() { return _descriptor->getTessellationRange().xy(); },
        [this](void* data) {_descriptor->setTessellationRange(vec3<F32>(*(vec2<F32>*)data, _descriptor->getTessellationRange().z)); },
        EditorComponentFieldType::PUSH_TYPE,
        false,
        GFX::PushConstantType::VEC2);

    SceneNode::postLoad(sgn);
}

void Terrain::onEditorChange(EditorComponentField& field) {
    _editorDataDirtyState = EditorDataState::QUEUED;
}

void Terrain::postBuild() {
    const U32 terrainWidth = _descriptor->getDimensions().width;
    const U32 terrainHeight = _descriptor->getDimensions().height;

    reserveTriangleCount((terrainWidth - 1) * (terrainHeight - 1) * 2);

    // Generate index buffer
    vector<vec3<U32>>& triangles = getTriangles();
    triangles.resize(terrainHeight * terrainWidth * 2);

    // ToDo: Use parallel_for for this
    I32 vectorIndex = 0;
    for (U32 height = 0; height < (terrainHeight - 1); ++height) {
        for (U32 width = 0; width < (terrainWidth - 1); ++width) {
            I32 vertexIndex = TER_COORD(width, height, terrainWidth);
            // Top triangle (T0)
            triangles[vectorIndex++].set(vertexIndex, vertexIndex + terrainWidth + 1, vertexIndex + 1);
            // Bottom triangle (T1)
            triangles[vectorIndex++].set(vertexIndex, vertexIndex + terrainWidth, vertexIndex + terrainWidth + 1);
        }
    }

    TerrainTextureLayer* textureLayer = _terrainTextures;
    getMaterialTpl()->addExternalTexture(textureLayer->blendMaps(),  to_U8(ShaderProgram::TextureUsage::COUNT) + 0);
    getMaterialTpl()->addExternalTexture(textureLayer->tileMaps(),   to_U8(ShaderProgram::TextureUsage::COUNT) + 1);
    getMaterialTpl()->addExternalTexture(textureLayer->normalMaps(), to_U8(ShaderProgram::TextureUsage::COUNT) + 2);

    // Approximate bounding box
    F32 halfWidth = terrainWidth * 0.5f;
    _boundingBox.setMin(-halfWidth, _descriptor->getAltitudeRange().min, -halfWidth);
    _boundingBox.setMax(halfWidth, _descriptor->getAltitudeRange().max, halfWidth);

    U32 chunkSize = to_U32(_descriptor->getTessellationRange().z);
    U32 maxChunkCount = to_U32(std::ceil((terrainWidth * terrainHeight) / (chunkSize * chunkSize * 1.0f)));
    Vegetation::precomputeStaticData(_context.context(), chunkSize, maxChunkCount);

    _terrainQuadtree.build(_context,
        _boundingBox,
        _descriptor->getDimensions(),
        chunkSize,
        this);

    // The terrain's final bounding box is the QuadTree's root bounding box
    _boundingBox.set(_terrainQuadtree.computeBoundingBox());
}

void Terrain::frameStarted(SceneGraphNode& sgn) {
    if (_editorDataDirtyState == EditorDataState::QUEUED) {
        _editorDataDirtyState = EditorDataState::CHANGED;
    } else if (_editorDataDirtyState == EditorDataState::CHANGED) {
        _editorDataDirtyState = EditorDataState::IDLE;
    }
}

void Terrain::sceneUpdate(const U64 deltaTimeUS, SceneGraphNode& sgn, SceneState& sceneState) {
    _terrainTessellatorFlags[sgn.getGUID()].fill(false);
    _drawDistance = sceneState.renderState().generalVisibility();

    Object3D::sceneUpdate(deltaTimeUS, sgn, sceneState);
}

bool Terrain::onRender(SceneGraphNode& sgn,
                       const Camera& camera,
                       RenderStagePass renderStagePass) {
    const U8 stageIndex = to_U8(renderStagePass._stage);

    TerrainTessellator& tessellator = _terrainTessellator[stageIndex];
    bool& wasUpdated = _terrainTessellatorFlags[sgn.getGUID()][stageIndex];

    U32 offset = to_U32(stageIndex * Terrain::MAX_RENDER_NODES);

    RenderPackage& pkg = sgn.get<RenderingComponent>()->getDrawPackage(renderStagePass);
    if (_editorDataDirtyState == EditorDataState::CHANGED) {
        PushConstants constants = pkg.pushConstants(0);
        constants.set("tessellationRange", GFX::PushConstantType::VEC2, _descriptor->getTessellationRange().xy());
        pkg.pushConstants(0, constants);
    }

    if (!wasUpdated) 
    {
        const vec3<F32>& newEye = camera.getEye();
        const vec3<F32>& crtPos = sgn.get<TransformComponent>()->getPosition();

        if (tessellator.getEye() != newEye || tessellator.getOrigin() != crtPos)
        {
            tessellator.createTree(newEye, crtPos, _descriptor->getDimensions());
            U16 depth = 0;
            bufferPtr data = (bufferPtr)tessellator.updateAndGetRenderData(depth);

            STUBBED("This may cause stalls. Profile! -Ionut");
            _shaderData->writeData(offset, depth, data);

            DescriptorSet set = pkg.descriptorSet(0);
            set.addShaderBuffer({ ShaderBufferLocation::TERRAIN_DATA,
                                 _shaderData,
                                 vec2<U32>(offset, Terrain::MAX_RENDER_NODES) });
            pkg.descriptorSet(0, set);
        }

        wasUpdated = true;
    }

    GenericDrawCommand cmd = pkg.drawCommand(0, 0);
    cmd._drawCount = tessellator.getRenderDepth();
    pkg.drawCommand(0, 0, cmd);

    if (renderStagePass == RenderStagePass(RenderStage::DISPLAY, RenderPassType::DEPTH_PASS)) {
        _terrainQuadtree.updateVisibility(camera, _drawDistance);
    }

    return Object3D::onRender(sgn, camera, renderStagePass);
}

void Terrain::buildDrawCommands(SceneGraphNode& sgn,
                                RenderStagePass renderStagePass,
                                RenderPackage& pkgInOut) {

    GFX::SendPushConstantsCommand pushConstantsCommand = {};
    pushConstantsCommand._constants.set("tessellationRange", GFX::PushConstantType::VEC2, _descriptor->getTessellationRange().xy());
    pushConstantsCommand._constants.set("tileScale", GFX::PushConstantType::VEC4, _terrainTextures->getTileScales());
    pkgInOut.addPushConstantsCommand(pushConstantsCommand);

    GenericDrawCommand cmd = {};
    cmd._primitiveType = PrimitiveType::PATCH;
    cmd._sourceBuffer = getGeometryVB();
    cmd._bufferIndex = renderStagePass.index();
    cmd._patchVertexCount = 4;
    cmd._cmd.indexCount = getGeometryVB()->getIndexCount();
    enableOption(cmd, CmdRenderOptions::RENDER_INDIRECT);
    enableOption(cmd, CmdRenderOptions::CONVERT_TO_INDIRECT);
    enableOption(cmd, CmdRenderOptions::RENDER_TESSELLATED);
    
    GFX::DrawCommand drawCommand = {};
    drawCommand._drawCommands.push_back(cmd);
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

    pt.put("heightmapLocation", _descriptor->getVariable("heightmapLocation"));
    pt.put("heightmap", _descriptor->getVariable("heightmap"));
    pt.put("is16Bit", _descriptor->is16Bit());
    pt.put("terrainWidth", _descriptor->getDimensions().width);
    pt.put("terrainHeight", _descriptor->getDimensions().height);
    pt.put("altitudeRange.<xmlattr>.min", _descriptor->getAltitudeRange().min);
    pt.put("altitudeRange.<xmlattr>.max", _descriptor->getAltitudeRange().max);
    pt.put("tessellationRange.<xmlattr>.min", _descriptor->getTessellationRange().x);
    pt.put("tessellationRange.<xmlattr>.max", _descriptor->getTessellationRange().y);
    pt.put("tessellationRange.<xmlattr>.chunkSize", _descriptor->getTessellationRange().z);
    pt.put("textureLocation", _descriptor->getVariable("textureLocation"));
    pt.put("waterCaustics", _descriptor->getVariable("waterCaustics"));
    pt.put("underwaterAlbedoTexture", _descriptor->getVariable("underwaterAlbedoTexture"));
    pt.put("underwaterDetailTexture", _descriptor->getVariable("underwaterDetailTexture"));
    pt.put("underwaterTileScale", _descriptor->getVariablef("underwaterTileScale"));
    pt.put("vegetation.<xmlattr>.grassDensity", _descriptor->getGrassDensity());
    pt.put("vegetation.<xmlattr>.treeDensity", _descriptor->getTreeDensity());
    pt.put("vegetation.<xmlattr>.grassScale", _descriptor->getGrassScale());
    pt.put("vegetation.<xmlattr>.treeScale", _descriptor->getTreeScale());
    pt.put("vegetation.vegetationTextureLocation", _descriptor->getVariable("grassMapLocation"));
    pt.put("vegetation.map", _descriptor->getVariable("grassMap"));
    if (!_descriptor->getVariable("grassBillboard1").empty()) {
        pt.put("vegetation.grassBillboard1", _descriptor->getVariable("grassBillboard1"));
    }
    if (!_descriptor->getVariable("grassBillboard2").empty()) {
        pt.put("vegetation.grassBillboard2", _descriptor->getVariable("grassBillboard2"));
    }
    if (!_descriptor->getVariable("grassBillboard3").empty()) {
        pt.put("vegetation.grassBillboard3", _descriptor->getVariable("grassBillboard3"));
    }
    if (!_descriptor->getVariable("grassBillboard4").empty()) {
        pt.put("vegetation.grassBillboard4", _descriptor->getVariable("grassBillboard4"));
    }
    
    U8 prevAlbedoCount = 0u;
    U8 prevDetailCount = 0u;

    const stringImpl layerPrefix = "textureLayers.layer";
    for (U8 layer = 0; layer < _terrainTextures->layerCount(); ++layer) {
        stringImpl crtLayerPrefix = layerPrefix + to_stringImpl(layer);
        pt.put(crtLayerPrefix + ".blendMap", _terrainTextures->blendMaps()->getDescriptor().sourceFileList()[layer]);

        U8 albedoCount = _terrainTextures->albedoCountPerLayer(layer);
        U8 detailCount = _terrainTextures->detailCountPerLayer(layer);

        if (albedoCount > 0) {
            //R
            pt.put(crtLayerPrefix + ".redAlbedo", _terrainTextures->tileMaps()->getDescriptor().sourceFileList()[0 + prevAlbedoCount]);
            if (detailCount > 0) {
                pt.put(crtLayerPrefix + ".redDetail", _terrainTextures->normalMaps()->getDescriptor().sourceFileList()[0 + prevDetailCount]);
            }
            pt.put(crtLayerPrefix + ".redTileScale", _terrainTextures->getTileScales()[layer].r);
            if (albedoCount > 1) {
                //G
                pt.put(crtLayerPrefix + ".greenAlbedo", _terrainTextures->tileMaps()->getDescriptor().sourceFileList()[1 + prevAlbedoCount]);
                if (detailCount > 1) {
                    pt.put(crtLayerPrefix + ".greenDetail", _terrainTextures->normalMaps()->getDescriptor().sourceFileList()[1 + prevDetailCount]);
                }
                pt.put(crtLayerPrefix + ".greenTileScale", _terrainTextures->getTileScales()[layer].g);
                if (albedoCount > 2) {
                    //B
                    pt.put(crtLayerPrefix + ".blueAlbedo", _terrainTextures->tileMaps()->getDescriptor().sourceFileList()[2 + prevAlbedoCount]);
                    if (detailCount > 2) {
                        pt.put(crtLayerPrefix + ".blueDetail", _terrainTextures->normalMaps()->getDescriptor().sourceFileList()[2 + prevDetailCount]);
                    }
                    pt.put(crtLayerPrefix + ".blueTileScale", _terrainTextures->getTileScales()[layer].b);
                    if (albedoCount > 3) {
                        //A
                        pt.put(crtLayerPrefix + ".alphaAlbedo", _terrainTextures->tileMaps()->getDescriptor().sourceFileList()[3 + prevAlbedoCount]);
                        if (detailCount > 3) {
                            pt.put(crtLayerPrefix + ".alphaDetail", _terrainTextures->normalMaps()->getDescriptor().sourceFileList()[3 + prevDetailCount]);
                        }
                        pt.put(crtLayerPrefix + ".alphaTileScale", _terrainTextures->getTileScales()[layer].a);
                    }
                }
            }
        }

        prevAlbedoCount = albedoCount;
        prevDetailCount = detailCount;
    }

    Object3D::saveToXML(pt);
}

void Terrain::loadFromXML(const boost::property_tree::ptree& pt) {
 
    Object3D::loadFromXML(pt);
}


TerrainTextureLayer::~TerrainTextureLayer()
{
}

};