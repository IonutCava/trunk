#include "stdafx.h"

#include "Headers/Terrain.h"
#include "Headers/TerrainChunk.h"
#include "Headers/TerrainDescriptor.h"

#include "Quadtree/Headers/QuadtreeNode.h"

#include "Core/Headers/PlatformContext.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"

#include "Geometry/Material/Headers/Material.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

Terrain::Terrain(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name)
    : Object3D(context, parentCache, descriptorHash, name, ObjectType::TERRAIN, ObjectFlag::OBJECT_FLAG_NONE),
      _plane(nullptr),
      _shaderData(nullptr),
      _drawBBoxes(false),
      _chunkSize(1),
      _waterHeight(0.0f),
      _altitudeRange(0.0f, 1.0f)
{
    _cameraUpdated.fill(true);
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
    terrainNodeDescriptor._physicsGroup = PhysicsGroup::GROUP_STATIC;
    terrainNodeDescriptor._isSelectable = false;
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
    bufferDescriptor._primitiveCount = Terrain::MAX_RENDER_NODES * to_base(RenderStage::COUNT);
    bufferDescriptor._primitiveSizeInBytes = sizeof(TessellatedNodeData);
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._unbound = true;
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::OFTEN;

    _shaderData = _context.newSB(bufferDescriptor);
    sgn.get<RenderingComponent>()->registerShaderBuffer(ShaderBufferLocation::TERRAIN_DATA,
                                                        vec2<U32>(0, Terrain::MAX_RENDER_NODES),
                                                        *_shaderData);

    sgn.get<TransformComponent>()->setPosition(_offsetPosition);

    SceneNode::postLoad(sgn);
}

void Terrain::buildQuadtree() {
    reserveTriangleCount((_terrainDimensions.x - 1) * (_terrainDimensions.y - 1) * 2);
    _terrainQuadtree.Build(_context,
                           _boundingBox,
                           vec2<U32>(_terrainDimensions.x, _terrainDimensions.y),
                           _chunkSize,
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
    ACKNOWLEDGE_UNUSED(cameraNameHash);
    ACKNOWLEDGE_UNUSED(posOffset);
    ACKNOWLEDGE_UNUSED(rotationOffset);

    _cameraUpdated[to_base(_context.getRenderStage().stage())] = true;
}

bool Terrain::onRender(SceneGraphNode& sgn,
                       const SceneRenderState& sceneRenderState,
                       const RenderStagePass& renderStagePass) {
    RenderingComponent* renderComp = sgn.get<RenderingComponent>();
    RenderPackage& pkg = renderComp->getDrawPackage(renderStagePass);

    FrustumClipPlanes clipPlanes = pkg.clipPlanes(0);
    clipPlanes.set(to_U32(ClipPlaneIndex::CLIP_PLANE_0),
                   Plane<F32>(WORLD_Y_AXIS, _waterHeight),
                   true);
    pkg.clipPlanes(0, clipPlanes);

    const U8 stageIndex = to_U8(renderStagePass.stage());
    bool& cameraUpdated = _cameraUpdated[stageIndex];
    TerrainTessellator& tessellator = _terrainTessellator[stageIndex];

    if (cameraUpdated) {
        Camera* camera = sceneRenderState.parentScene().playerCamera();
        const vec3<F32>& newEye = camera->getEye();
        if (tessellator.getEye() != newEye) {
            tessellator.createTree(newEye,
                                   vec3<F32>(0),
                                   _terrainDimensions);
            tessellator.updateRenderData();
        }

        cameraUpdated = false;
    }
    {
        GenericDrawCommand cmd = pkg.drawCommand(0, 0);
        cmd.drawCount(tessellator.renderDepth());
        pkg.drawCommand(0, 0, cmd);
    }
    U32 offset = to_U32(stageIndex * Terrain::MAX_RENDER_NODES);

    STUBBED("This may cause stalls. Profile! -Ionut");
    _shaderData->writeData(offset, tessellator.renderDepth(), (bufferPtr)tessellator.renderData().data());

    sgn.get<RenderingComponent>()->registerShaderBuffer(ShaderBufferLocation::TERRAIN_DATA,
                                                        vec2<U32>(offset, Terrain::MAX_RENDER_NODES),
                                                        *_shaderData);

    if (renderStagePass.stage() == RenderStage::DISPLAY) {
        // draw infinite plane
        assert(pkg.drawCommand(1, 0).drawCount() == 1u);

        const Pipeline* pipeline = pkg.pipeline(1);
        PipelineDescriptor descriptor = pipeline->descriptor();
        descriptor._shaderProgram = (renderStagePass.pass() == RenderPassType::DEPTH_PASS
                                     ? _planeDepthShader
                                     : _planeShader);

        pkg.pipeline(1, _context.newPipeline(descriptor));

        // draw bounding boxes;
        U16 state = _drawBBoxes ? 1 : 0;
        for (I32 i = 2; i < pkg.drawCommandCount(); ++i) {
            GenericDrawCommand cmd = pkg.drawCommand(i, 0);
            cmd.drawCount(state);
            pkg.drawCommand(i, 0, cmd);
        }
    }

    return Object3D::onRender(sgn, sceneRenderState, renderStagePass);
}

void Terrain::buildDrawCommands(SceneGraphNode& sgn,
                                const RenderStagePass& renderStagePass,
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
    cmd.primitiveType(PrimitiveType::PATCH);
    cmd.enableOption(GenericDrawCommand::RenderOptions::RENDER_TESSELLATED);
    cmd.sourceBuffer(getGeometryVB());
    cmd.patchVertexCount(4);
    cmd.cmd().indexCount = getGeometryVB()->getIndexCount();
    {
        GFX::DrawCommand drawCommand;
        drawCommand._drawCommands.push_back(cmd);
        pkgInOut.addDrawCommand(drawCommand);
    }
    if (renderStagePass.stage() == RenderStage::DISPLAY) {

        PipelineDescriptor pipelineDescriptor;
        pipelineDescriptor._shaderProgram = 
            (renderStagePass.pass() == RenderPassType::DEPTH_PASS
                                     ? _planeDepthShader
                                     : _planeShader);
        {
            GFX::BindPipelineCommand pipelineCommand;
            pipelineCommand._pipeline = &_context.newPipeline(pipelineDescriptor);
            pkgInOut.addPipelineCommand(pipelineCommand);
        }
        //infinite plane
        GenericDrawCommand planeCmd;
        planeCmd.primitiveType(PrimitiveType::TRIANGLE_STRIP);
        planeCmd.cmd().firstIndex = 0;
        planeCmd.cmd().indexCount = _plane->getGeometryVB()->getIndexCount();
        planeCmd.LoD(0);
        planeCmd.sourceBuffer(_plane->getGeometryVB());

        {
            GFX::DrawCommand drawCommand;
            drawCommand._drawCommands.push_back(planeCmd);
            pkgInOut.addDrawCommand(drawCommand);
        }

        pipelineDescriptor._shaderProgram = ShaderProgram::defaultShader();
        {
            GFX::BindPipelineCommand pipelineCommand;
            pipelineCommand._pipeline = &_context.newPipeline(pipelineDescriptor);
            pkgInOut.addPipelineCommand(pipelineCommand);
        }

        //BoundingBoxes
        _terrainQuadtree.drawBBox(_context, pkgInOut);
    }

    Object3D::buildDrawCommands(sgn, renderStagePass, pkgInOut);

    _cameraUpdated[to_base(renderStagePass.stage())] = true;
}

vec3<F32> Terrain::getPositionFromGlobal(F32 x, F32 z) const {
    x -= _boundingBox.getCenter().x;
    z -= _boundingBox.getCenter().z;
    F32 xClamp = (0.5f * _terrainDimensions.x) + x;
    F32 zClamp = (0.5f * _terrainDimensions.y) - z;
    xClamp /= _terrainDimensions.x;
    zClamp /= _terrainDimensions.y;
    zClamp = 1 - zClamp;
    vec3<F32> temp = getPosition(xClamp, zClamp);

    return temp;
}

vec3<F32> Terrain::getPosition(F32 x_clampf, F32 z_clampf) const {
    assert(!(x_clampf < .0f || z_clampf < .0f || x_clampf > 1.0f || z_clampf > 1.0f));

    vec2<F32> posF(x_clampf * _terrainDimensions.x,
                   z_clampf * _terrainDimensions.y);
    vec2<I32> posI(to_I32(posF.x), to_I32(posF.y));
    vec2<F32> posD(posF.x - posI.x, posF.y - posI.y);

    if (posI.x >= (I32)_terrainDimensions.x - 1)
        posI.x = _terrainDimensions.x - 2;
    if (posI.y >= (I32)_terrainDimensions.y - 1)
        posI.y = _terrainDimensions.y - 2;

    assert(posI.x >= 0 && posI.x < to_I32(_terrainDimensions.x) - 1 &&
           posI.y >= 0 && posI.y < to_I32(_terrainDimensions.y) - 1);

    vec3<F32> pos(
        _boundingBox.getMin().x +
            x_clampf * (_boundingBox.getMax().x - _boundingBox.getMin().x),
        0.0f,
        _boundingBox.getMin().z +
            z_clampf * (_boundingBox.getMax().z - _boundingBox.getMin().z));

    auto height = [&](U32 index) -> F32 {
        return _physicsVerts[index]._position.y;
    };

    pos.y = height(TER_COORD(posI.x, posI.y, to_I32(_terrainDimensions.x))) *
            (1.0f - posD.x) * (1.0f - posD.y) + 
            height(TER_COORD(posI.x + 1, posI.y, to_I32(_terrainDimensions.x))) *
            posD.x * (1.0f - posD.y) +
            height(TER_COORD(posI.x, posI.y + 1, to_I32(_terrainDimensions.x))) *
            (1.0f - posD.x) * posD.y +
            height(TER_COORD(posI.x + 1, posI.y + 1,to_I32(_terrainDimensions.x))) *
            posD.x * posD.y;

    return pos;
}

vec3<F32> Terrain::getNormal(F32 x_clampf, F32 z_clampf) const {
    assert(!(x_clampf < .0f || z_clampf < .0f || x_clampf > 1.0f || z_clampf > 1.0f));

    vec2<F32> posF(x_clampf * _terrainDimensions.x, z_clampf * _terrainDimensions.y);

    vec2<I32> posI(to_I32(x_clampf * _terrainDimensions.x), to_I32(z_clampf * _terrainDimensions.y));

    vec2<F32> posD(posF.x - posI.x, posF.y - posI.y);

    if (posI.x >= to_I32(_terrainDimensions.x) - 1) {
        posI.x = to_I32(_terrainDimensions.x) - 2;
    }
    if (posI.y >= to_I32(_terrainDimensions.y) - 1) {
        posI.y = to_I32(_terrainDimensions.y) - 2;
    }
    assert(posI.x >= 0 && posI.x < to_I32(_terrainDimensions.x) - 1 &&
           posI.y >= 0 && posI.y < to_I32(_terrainDimensions.y) - 1);

    vec3<F32> normals[4];

    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x, posI.y, to_I32(_terrainDimensions.x))]._normal, normals[0]);
    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x + 1, posI.y, to_I32(_terrainDimensions.x))]._normal, normals[1]);
    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x, posI.y + 1, to_I32(_terrainDimensions.x))]._normal, normals[2]);
    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x + 1, posI.y + 1, to_I32(_terrainDimensions.x))]._normal, normals[3]);

    return normals[0] * (1.0f - posD.x) * (1.0f - posD.y) +
           normals[1] * posD.x * (1.0f - posD.y) +
           normals[2] * (1.0f - posD.x) * posD.y + 
           normals[3] * posD.x * posD.y;
}

vec3<F32> Terrain::getTangent(F32 x_clampf, F32 z_clampf) const {
    assert(!(x_clampf < .0f || z_clampf < .0f || x_clampf > 1.0f || z_clampf > 1.0f));

    vec2<F32> posF(x_clampf * _terrainDimensions.x, z_clampf * _terrainDimensions.y);
    vec2<I32> posI(to_I32(x_clampf * _terrainDimensions.x), to_I32(z_clampf * _terrainDimensions.y));
    vec2<F32> posD(posF.x - posI.x, posF.y - posI.y);

    if (posI.x >= to_I32(_terrainDimensions.x) - 1) {
        posI.x = to_I32(_terrainDimensions.x) - 2;
    }

    if (posI.y >= to_I32(_terrainDimensions.y) - 1) {
        posI.y = to_I32(_terrainDimensions.y) - 2;
    }

    assert(posI.x >= 0 && posI.x < to_I32(_terrainDimensions.x) - 1 &&
           posI.y >= 0 && posI.y < to_I32(_terrainDimensions.y) - 1);

    vec3<F32> tangents[4];

    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x, posI.y, to_I32(_terrainDimensions.x))]._tangent, tangents[0]);
    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x + 1, posI.y, to_I32(_terrainDimensions.x))]._tangent, tangents[1]);
    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x, posI.y + 1, to_I32(_terrainDimensions.x))]._tangent, tangents[2]);
    Util::UNPACK_VEC3(_physicsVerts[TER_COORD(posI.x + 1, posI.y + 1, to_I32(_terrainDimensions.x))]._tangent, tangents[3]);

    return tangents[0] * (1.0f - posD.x) * (1.0f - posD.y) +
           tangents[1] * posD.x * (1.0f - posD.y) +
           tangents[2] * (1.0f - posD.x) * posD.y +
           tangents[3] * posD.x * posD.y;
}

TerrainTextureLayer::~TerrainTextureLayer()
{
}

};