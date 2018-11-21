#include "stdafx.h"

#include "Headers/Terrain.h"
#include "Headers/TerrainChunk.h"
#include "Headers/TerrainDescriptor.h"

#include "Quadtree/Headers/QuadtreeNode.h"

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
      _plane(nullptr),
      _shaderData(nullptr),
      _drawBBoxes(false),
      _waterHeight(0.0f)
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
    SceneGraphNodeDescriptor terrainNodeDescriptor;
    terrainNodeDescriptor._node = _plane;
    terrainNodeDescriptor._usageContext = NodeUsageContext::NODE_STATIC;
    terrainNodeDescriptor._componentMask = to_base(ComponentType::NAVIGATION) |
                                           to_base(ComponentType::TRANSFORM) |
                                           to_base(ComponentType::RIGID_BODY) |
                                           to_base(ComponentType::BOUNDS) |
                                           to_base(ComponentType::RENDERING) |
                                           to_base(ComponentType::NETWORKING);

    SceneGraphNode* planeSGN = sgn.addNode(terrainNodeDescriptor);
    planeSGN->setActive(false);
    //for (TerrainChunk* chunk : _terrainChunks) {
        //SceneGraphNode* vegetation = sgn.addNode(Attorney::TerrainChunkTerrain::getVegetation(*chunk), normalMask);
        //vegetation->lockVisibility(true);
    //}
    // Skip Object3D::load() to avoid triangle list computation (extremely expensive)!!!

    ShaderBufferDescriptor bufferDescriptor;
    bufferDescriptor._elementCount = Terrain::MAX_RENDER_NODES * to_base(RenderStage::COUNT);
    bufferDescriptor._elementSize = sizeof(TessellatedNodeData);
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::UNBOUND_STORAGE) | to_U32(ShaderBuffer::Flags::ALLOW_THREADED_WRITES);
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::OFTEN;
    bufferDescriptor._name = "TERRAIN_RENDER_NODES";
    _shaderData = _context.newSB(bufferDescriptor);
    sgn.get<RenderingComponent>()->registerShaderBuffer(ShaderBufferLocation::TERRAIN_DATA,
                                                        vec2<U32>(0, Terrain::MAX_RENDER_NODES),
                                                        *_shaderData);

    sgn.get<RigidBodyComponent>()->physicsGroup(PhysicsGroup::GROUP_STATIC);

    SceneNode::postLoad(sgn);
}

void Terrain::buildQuadtree() {
    reserveTriangleCount((_descriptor->getDimensions().x - 1) * (_descriptor->getDimensions().y - 1) * 2);
    _terrainQuadtree.Build(_context,
                           _boundingBox,
                           vec2<U32>(_descriptor->getDimensions().x, _descriptor->getDimensions().y),
                           _descriptor->getChunkSize(),
                           this);

    // The terrain's final bounding box is the QuadTree's root bounding box
    _boundingBox.set(_terrainQuadtree.computeBoundingBox());

    TerrainTextureLayer* textureLayer = _terrainTextures;
    getMaterialTpl()->addExternalTexture(textureLayer->blendMaps(),  to_U8(ShaderProgram::TextureUsage::COUNT) + 0);
    getMaterialTpl()->addExternalTexture(textureLayer->tileMaps(),   to_U8(ShaderProgram::TextureUsage::COUNT) + 1);
    getMaterialTpl()->addExternalTexture(textureLayer->normalMaps(), to_U8(ShaderProgram::TextureUsage::COUNT) + 2);
}

void Terrain::sceneUpdate(const U64 deltaTimeUS,
                          SceneGraphNode& sgn,
                          SceneState& sceneState) {
    _waterHeight = sceneState.waterLevel();
    _terrainQuadtree.sceneUpdate(deltaTimeUS, sgn, sceneState);
    Object3D::sceneUpdate(deltaTimeUS, sgn, sceneState);
}

void Terrain::onCameraUpdate(SceneGraphNode& sgn,
                             const U64 cameraNameHash,
                             const vec3<F32>& posOffset,
                             const mat4<F32>& rotationOffset) {
    ACKNOWLEDGE_UNUSED(sgn);
    ACKNOWLEDGE_UNUSED(posOffset);
    ACKNOWLEDGE_UNUSED(rotationOffset);

    _cameraUpdated[cameraNameHash] = true;
}

bool Terrain::onRender(SceneGraphNode& sgn,
                       const SceneRenderState& sceneRenderState,
                       RenderStagePass renderStagePass) {
    RenderingComponent* renderComp = sgn.get<RenderingComponent>();
    RenderPackage& pkg = renderComp->getDrawPackage(renderStagePass);

    FrustumClipPlanes clipPlanes = pkg.clipPlanes(0);
    clipPlanes.set(to_U32(ClipPlaneIndex::CLIP_PLANE_0),
                   Plane<F32>(WORLD_Y_AXIS, _waterHeight),
                   true);
    pkg.clipPlanes(0, clipPlanes);

    Camera* camera = sceneRenderState.parentScene().playerCamera();

    const U8 stageIndex = to_U8(renderStagePass._stage);

    //ToDo: maybe do a "find" first? -Ionut
    bool& cameraUpdated = _cameraUpdated[_ID(camera->name().c_str())];
    TerrainTessellator& tessellator = _terrainTessellator[stageIndex];

    if (cameraUpdated) {
        const vec3<F32>& newEye = camera->getEye();
        if (tessellator.getEye() != newEye) {
            tessellator.createTree(newEye,
                                   vec3<F32>(0),
                                   _descriptor->getDimensions());
            tessellator.updateRenderData();
        }

        cameraUpdated = false;
    }
    {
        GenericDrawCommand cmd = pkg.drawCommand(0, 0);
        cmd._drawCount = tessellator.renderDepth();
        pkg.drawCommand(0, 0, cmd);
    }
    U32 offset = to_U32(stageIndex * Terrain::MAX_RENDER_NODES);

    STUBBED("This may cause stalls. Profile! -Ionut");
    _shaderData->writeData(offset, tessellator.renderDepth(), (bufferPtr)tessellator.renderData().data());

    sgn.get<RenderingComponent>()->registerShaderBuffer(ShaderBufferLocation::TERRAIN_DATA,
                                                        vec2<U32>(offset, Terrain::MAX_RENDER_NODES),
                                                        *_shaderData);

    if (renderStagePass._stage == RenderStage::DISPLAY) {
        // draw infinite plane
        assert(pkg.drawCommand(1, 0)._drawCount == 1u);

        const Pipeline* pipeline = pkg.pipeline(1);
        PipelineDescriptor descriptor = pipeline->descriptor();
        descriptor._shaderProgramHandle = (renderStagePass._passType == RenderPassType::DEPTH_PASS
                                                                      ? _planeDepthShader
                                                                      : _planeShader)->getID();

        pkg.pipeline(1, *_context.newPipeline(descriptor));

        // draw bounding boxes;
        U16 state = _drawBBoxes ? 1 : 0;
        for (I32 i = 2; i < pkg.drawCommandCount(); ++i) {
            GenericDrawCommand cmd = pkg.drawCommand(i, 0);
            cmd._drawCount = state;
            pkg.drawCommand(i, 0, cmd);
        }
    }

    return Object3D::onRender(sgn, sceneRenderState, renderStagePass);
}

void Terrain::buildDrawCommands(SceneGraphNode& sgn,
                                RenderStagePass renderStagePass,
                                RenderPackage& pkgInOut) {

    const vec3<F32>& bbMin = _boundingBox.getMin();
    const vec3<F32>& bbExtent = _boundingBox.getExtent();
    TerrainTextureLayer* textureLayer = _terrainTextures;

    PushConstants constants = pkgInOut.pushConstants(0);
    constants.set("bbox_min",     GFX::PushConstantType::VEC3, bbMin);
    constants.set("bbox_extent",  GFX::PushConstantType::VEC3, bbExtent);
    constants.set("diffuseScale", GFX::PushConstantType::VEC4, textureLayer->getDiffuseScales());
    constants.set("detailScale",  GFX::PushConstantType::VEC4, textureLayer->getDetailScales());
    pkgInOut.pushConstants(0, constants);

    GFX::SetClipPlanesCommand clipPlanesCommand;
    clipPlanesCommand._clippingPlanes.set(to_U32(ClipPlaneIndex::CLIP_PLANE_0),
                                          Plane<F32>(WORLD_Y_AXIS, _waterHeight),
                                          false);
    pkgInOut.addClipPlanesCommand(clipPlanesCommand);

    GenericDrawCommand cmd;
    cmd._primitiveType = PrimitiveType::PATCH;
    cmd._sourceBuffer = getGeometryVB();
    cmd._bufferIndex = renderStagePass.index();
    cmd._patchVertexCount = 4;
    cmd._cmd.indexCount = getGeometryVB()->getIndexCount();
    enableOption(cmd, CmdRenderOptions::RENDER_TESSELLATED);
    {
        GFX::DrawCommand drawCommand;
        drawCommand._drawCommands.push_back(cmd);
        pkgInOut.addDrawCommand(drawCommand);
    }
    if (renderStagePass._stage == RenderStage::DISPLAY) {

        PipelineDescriptor pipelineDescriptor;
        pipelineDescriptor._shaderProgramHandle = 
            (renderStagePass._passType == RenderPassType::DEPTH_PASS
                                        ? _planeDepthShader
                                        : _planeShader)->getID();
        {
            GFX::BindPipelineCommand pipelineCommand;
            pipelineCommand._pipeline = _context.newPipeline(pipelineDescriptor);
            pkgInOut.addPipelineCommand(pipelineCommand);
        }
        //infinite plane
        GenericDrawCommand planeCmd;
        planeCmd._primitiveType = PrimitiveType::TRIANGLE_STRIP;
        planeCmd._cmd.firstIndex = 0;
        planeCmd._cmd.indexCount = _plane->getGeometryVB()->getIndexCount();
        planeCmd._lodIndex = 0;
        planeCmd._sourceBuffer = _plane->getGeometryVB();
        planeCmd._bufferIndex = renderStagePass.index();

        {
            GFX::DrawCommand drawCommand;
            drawCommand._drawCommands.push_back(planeCmd);
            pkgInOut.addDrawCommand(drawCommand);
        }

        pipelineDescriptor._shaderProgramHandle = ShaderProgram::defaultShader()->getID();
        {
            GFX::BindPipelineCommand pipelineCommand;
            pipelineCommand._pipeline = _context.newPipeline(pipelineDescriptor);
            pkgInOut.addPipelineCommand(pipelineCommand);
        }

        //BoundingBoxes
        _terrainQuadtree.drawBBox(_context, pkgInOut);
    }

    Object3D::buildDrawCommands(sgn, renderStagePass, pkgInOut);

    _cameraUpdated[to_base(renderStagePass._stage)] = true;
}

vec3<F32> Terrain::getPositionFromGlobal(F32 x, F32 z) const {
    x -= _boundingBox.getCenter().x;
    z -= _boundingBox.getCenter().z;
    F32 xClamp = (0.5f * _descriptor->getDimensions().x) + x;
    F32 zClamp = (0.5f * _descriptor->getDimensions().y) - z;
    xClamp /= _descriptor->getDimensions().x;
    zClamp /= _descriptor->getDimensions().y;
    zClamp = 1 - zClamp;
    vec3<F32> temp = getPosition(xClamp, zClamp);

    return temp;
}

vec3<F32> Terrain::getPosition(F32 x_clampf, F32 z_clampf) const {
    assert(!(x_clampf < .0f || z_clampf < .0f || x_clampf > 1.0f || z_clampf > 1.0f));

    vec2<F32> posF(x_clampf * _descriptor->getDimensions().x,
                   z_clampf * _descriptor->getDimensions().y);
    vec2<I32> posI(to_I32(posF.x), to_I32(posF.y));
    vec2<F32> posD(posF.x - posI.x, posF.y - posI.y);

    if (posI.x >= (I32)_descriptor->getDimensions().x - 1)
        posI.x = _descriptor->getDimensions().x - 2;
    if (posI.y >= (I32)_descriptor->getDimensions().y - 1)
        posI.y = _descriptor->getDimensions().y - 2;

    assert(posI.x >= 0 && posI.x < to_I32(_descriptor->getDimensions().x) - 1 &&
           posI.y >= 0 && posI.y < to_I32(_descriptor->getDimensions().y) - 1);

    vec3<F32> pos(
        _boundingBox.getMin().x +
            x_clampf * (_boundingBox.getMax().x - _boundingBox.getMin().x),
        0.0f,
        _boundingBox.getMin().z +
            z_clampf * (_boundingBox.getMax().z - _boundingBox.getMin().z));

    auto height = [&](U32 index) -> F32 {
        return _physicsVerts[index]._position.y;
    };

    pos.y = height(TER_COORD(posI.x, posI.y, to_I32(_descriptor->getDimensions().x))) *
            (1.0f - posD.x) * (1.0f - posD.y) + 
            height(TER_COORD(posI.x + 1, posI.y, to_I32(_descriptor->getDimensions().x))) *
            posD.x * (1.0f - posD.y) +
            height(TER_COORD(posI.x, posI.y + 1, to_I32(_descriptor->getDimensions().x))) *
            (1.0f - posD.x) * posD.y +
            height(TER_COORD(posI.x + 1, posI.y + 1,to_I32(_descriptor->getDimensions().x))) *
            posD.x * posD.y;

    return pos;
}

vec3<F32> Terrain::getNormal(F32 x_clampf, F32 z_clampf) const {
    assert(!(x_clampf < .0f || z_clampf < .0f || x_clampf > 1.0f || z_clampf > 1.0f));

    vec2<F32> posF(x_clampf * _descriptor->getDimensions().x, z_clampf * _descriptor->getDimensions().y);

    vec2<I32> posI(to_I32(x_clampf * _descriptor->getDimensions().x), to_I32(z_clampf * _descriptor->getDimensions().y));

    vec2<F32> posD(posF.x - posI.x, posF.y - posI.y);

    if (posI.x >= to_I32(_descriptor->getDimensions().x) - 1) {
        posI.x = to_I32(_descriptor->getDimensions().x) - 2;
    }
    if (posI.y >= to_I32(_descriptor->getDimensions().y) - 1) {
        posI.y = to_I32(_descriptor->getDimensions().y) - 2;
    }
    assert(posI.x >= 0 && posI.x < to_I32(_descriptor->getDimensions().x) - 1 &&
           posI.y >= 0 && posI.y < to_I32(_descriptor->getDimensions().y) - 1);

    vec3<F32> normals[4];

    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x, posI.y, to_I32(_descriptor->getDimensions().x))]._normal, normals[0]);
    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x + 1, posI.y, to_I32(_descriptor->getDimensions().x))]._normal, normals[1]);
    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x, posI.y + 1, to_I32(_descriptor->getDimensions().x))]._normal, normals[2]);
    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x + 1, posI.y + 1, to_I32(_descriptor->getDimensions().x))]._normal, normals[3]);

    return normals[0] * (1.0f - posD.x) * (1.0f - posD.y) +
           normals[1] * posD.x * (1.0f - posD.y) +
           normals[2] * (1.0f - posD.x) * posD.y + 
           normals[3] * posD.x * posD.y;
}

vec3<F32> Terrain::getTangent(F32 x_clampf, F32 z_clampf) const {
    assert(!(x_clampf < .0f || z_clampf < .0f || x_clampf > 1.0f || z_clampf > 1.0f));

    vec2<F32> posF(x_clampf * _descriptor->getDimensions().x, z_clampf * _descriptor->getDimensions().y);
    vec2<I32> posI(to_I32(x_clampf * _descriptor->getDimensions().x), to_I32(z_clampf * _descriptor->getDimensions().y));
    vec2<F32> posD(posF.x - posI.x, posF.y - posI.y);

    if (posI.x >= to_I32(_descriptor->getDimensions().x) - 1) {
        posI.x = to_I32(_descriptor->getDimensions().x) - 2;
    }

    if (posI.y >= to_I32(_descriptor->getDimensions().y) - 1) {
        posI.y = to_I32(_descriptor->getDimensions().y) - 2;
    }

    assert(posI.x >= 0 && posI.x < to_I32(_descriptor->getDimensions().x) - 1 &&
           posI.y >= 0 && posI.y < to_I32(_descriptor->getDimensions().y) - 1);

    vec3<F32> tangents[4];

    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x, posI.y, to_I32(_descriptor->getDimensions().x))]._tangent, tangents[0]);
    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x + 1, posI.y, to_I32(_descriptor->getDimensions().x))]._tangent, tangents[1]);
    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x, posI.y + 1, to_I32(_descriptor->getDimensions().x))]._tangent, tangents[2]);
    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x + 1, posI.y + 1, to_I32(_descriptor->getDimensions().x))]._tangent, tangents[3]);

    return tangents[0] * (1.0f - posD.x) * (1.0f - posD.y) +
           tangents[1] * posD.x * (1.0f - posD.y) +
           tangents[2] * (1.0f - posD.x) * posD.y +
           tangents[3] * posD.x * posD.y;
}

vec2<U16> Terrain::getDimensions() const {
    return _descriptor->getDimensions();
}

void Terrain::saveToXML(boost::property_tree::ptree& pt) const {

    pt.put("heightmapLocation", _descriptor->getVariable("heightmapLocation"));
    pt.put("heightmap", _descriptor->getVariable("heightmap"));
    pt.put("is16Bit", _descriptor->is16Bit());
    pt.put("heightTexture", _descriptor->getVariable("heightTexture"));
    pt.put("terrainWidth", _descriptor->getDimensions().width);
    pt.put("terrainHeight", _descriptor->getDimensions().height);
    pt.put("altitudeRange.<xmlattr>.min", _descriptor->getAltitudeRange().min);
    pt.put("altitudeRange.<xmlattr>.max", _descriptor->getAltitudeRange().max);
    pt.put("targetChunkSize", _descriptor->getChunkSize());
    pt.put("textureLocation", _descriptor->getVariable("textureLocation"));
    pt.put("waterCaustics", _descriptor->getVariable("waterCaustics"));
    pt.put("underwaterAlbedoTexture", _descriptor->getVariable("underwaterAlbedoTexture"));
    pt.put("underwaterDetailTexture", _descriptor->getVariable("underwaterDetailTexture"));
    pt.put("underwaterDiffuseScale", _descriptor->getVariablef("underwaterDiffuseScale"));
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
            pt.put(crtLayerPrefix + ".redDiffuseScale", _terrainTextures->getDiffuseScales()[layer].r);
            if (albedoCount > 1) {
                //G
                pt.put(crtLayerPrefix + ".greenAlbedo", _terrainTextures->tileMaps()->getDescriptor().sourceFileList()[1 + prevAlbedoCount]);
                pt.put(crtLayerPrefix + ".greenDiffuseScale", _terrainTextures->getDiffuseScales()[layer].g);
                if (albedoCount > 2) {
                    //B
                    pt.put(crtLayerPrefix + ".blueAlbedo", _terrainTextures->tileMaps()->getDescriptor().sourceFileList()[2 + prevAlbedoCount]);
                    pt.put(crtLayerPrefix + ".blueDiffuseScale", _terrainTextures->getDiffuseScales()[layer].b);
                    if (albedoCount > 3) {
                        //A
                        pt.put(crtLayerPrefix + ".alphaAlbedo", _terrainTextures->tileMaps()->getDescriptor().sourceFileList()[3 + prevAlbedoCount]);
                        pt.put(crtLayerPrefix + ".alphaDiffuseScale", _terrainTextures->getDiffuseScales()[layer].a);
                    }
                }
            }
        }
        
        if (detailCount > 0) {
            pt.put(crtLayerPrefix + ".redDetail", _terrainTextures->normalMaps()->getDescriptor().sourceFileList()[0 + prevDetailCount]);
            pt.put(crtLayerPrefix + ".redDetailScale", _terrainTextures->getDetailScales()[layer].r);

            if (detailCount > 1) {
                pt.put(crtLayerPrefix + ".greenDetail", _terrainTextures->normalMaps()->getDescriptor().sourceFileList()[1 + prevDetailCount]);
                pt.put(crtLayerPrefix + ".greenDetailScale", _terrainTextures->getDetailScales()[layer].g);

                if (detailCount > 2) {
                    pt.put(crtLayerPrefix + ".blueDetail", _terrainTextures->normalMaps()->getDescriptor().sourceFileList()[2 + prevDetailCount]);
                    pt.put(crtLayerPrefix + ".blueDetailScale", _terrainTextures->getDetailScales()[layer].b);

                    if (detailCount > 3) {
                        pt.put(crtLayerPrefix + ".alphaDetail", _terrainTextures->normalMaps()->getDescriptor().sourceFileList()[3 + prevDetailCount]);
                        pt.put(crtLayerPrefix + ".alphaDetailScale", _terrainTextures->getDetailScales()[layer].a);
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