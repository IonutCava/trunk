#include "stdafx.h"

#include "Headers/Terrain.h"
#include "Headers/TerrainChunk.h"
#include "Headers/TerrainDescriptor.h"

#include "Quadtree/Headers/QuadtreeNode.h"

#include "Core/Headers/PlatformContext.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Predefined/Patch3D.h"

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
    MemoryManager::DELETE_VECTOR(_terrainTextures);
    return Object3D::unload();
}

void Terrain::postLoad(SceneGraphNode& sgn) {
    static const U32 normalMask = to_base(SGNComponent::ComponentType::NAVIGATION) |
                                  to_base(SGNComponent::ComponentType::PHYSICS) |
                                  to_base(SGNComponent::ComponentType::BOUNDS) |
                                  to_base(SGNComponent::ComponentType::RENDERING) |
                                  to_base(SGNComponent::ComponentType::NETWORKING);

    SceneGraphNode_ptr planeSGN(sgn.addNode(_plane, normalMask, PhysicsGroup::GROUP_STATIC));
    planeSGN->setActive(false);
    //for (TerrainChunk* chunk : _terrainChunks) {
        //SceneGraphNode_ptr vegetation = sgn.addNode(Attorney::TerrainChunkTerrain::getVegetation(*chunk), normalMask);
        //vegetation->lockVisibility(true);
    //}
    // Skip Object3D::load() to avoid triangle list computation (extremely expensive)!!!

    ShaderBufferDescriptor bufferDescriptor;
    bufferDescriptor._primitiveCount = Terrain::MAX_RENDER_NODES;
    bufferDescriptor._primitiveSizeInBytes = sizeof(TessellatedNodeData);
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._unbound = true;
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::OFTEN;

    _shaderData = _context.newSB(bufferDescriptor);
    sgn.get<RenderingComponent>()->registerShaderBuffer(ShaderBufferLocation::TERRAIN_DATA,
                                                        vec2<U32>(0, Terrain::MAX_RENDER_NODES),
                                                        *_shaderData);

    sgn.get<PhysicsComponent>()->setPosition(_offsetPosition);

    SceneNode::postLoad(sgn);
}

void Terrain::buildQuadtree() {
    reserveTriangleCount((_terrainDimensions.x - 1) *
                         (_terrainDimensions.y - 1) * 2);
    _terrainQuadtree.Build(
        _context,
        _boundingBox,
        vec2<U32>(_terrainDimensions.x, _terrainDimensions.y),
        _chunkSize, this);

    // The terrain's final bounding box is the QuadTree's root bounding box
    _boundingBox.set(_terrainQuadtree.computeBoundingBox());

    const Material_ptr& mat = getMaterialTpl();

    const vec3<F32>& bbMin = _boundingBox.getMin();
    const vec3<F32>& bbExtent = _boundingBox.getExtent();

    for (RenderStagePass::PassIndex i = 0; i < RenderStagePass::count(); ++i) {
        const ShaderProgram_ptr& drawShader = mat->getShaderInfo(RenderStagePass::stagePass(i)).getProgram();

        drawShader->Uniform("bbox_min", bbMin);
        drawShader->Uniform("bbox_extent", bbExtent);

        U8 textureOffset = to_U8(ShaderProgram::TextureUsage::COUNT);
        U8 layerOffset = 0;
        stringImpl layerIndex;
        for (U8 k = 0; k < _terrainTextures.size(); ++k) {
            layerOffset = k * 3 + textureOffset;
            layerIndex = to_stringImpl(k);
            TerrainTextureLayer* textureLayer = _terrainTextures[k];

            drawShader->Uniform(("texBlend[" + layerIndex + "]").c_str(), layerOffset);
            drawShader->Uniform(("texTileMaps[" + layerIndex + "]").c_str(), layerOffset + 1);
            drawShader->Uniform(("texNormalMaps[" + layerIndex + "]").c_str(), layerOffset + 2);
            drawShader->Uniform(("diffuseScale[" + layerIndex + "]").c_str(), textureLayer->getDiffuseScales());
            drawShader->Uniform(("detailScale[" + layerIndex + "]").c_str(), textureLayer->getDetailScales());

            if (i == 0) {
                getMaterialTpl()->addCustomTexture(textureLayer->blendMap(), layerOffset);
                getMaterialTpl()->addCustomTexture(textureLayer->tileMaps(), layerOffset + 1);
                getMaterialTpl()->addCustomTexture(textureLayer->normalMaps(), layerOffset + 2);
            }
        }
    }
}

void Terrain::sceneUpdate(const U64 deltaTime,
                          SceneGraphNode& sgn,
                          SceneState& sceneState) {
    _waterHeight = sceneState.waterLevel();
    _terrainQuadtree.sceneUpdate(deltaTime, sgn, sceneState);
    Object3D::sceneUpdate(deltaTime, sgn, sceneState);
}

void Terrain::onCameraUpdate(SceneGraphNode& sgn,
                             const I64 cameraGUID,
                             const vec3<F32>& posOffset,
                             const mat4<F32>& rotationOffset) {
    ACKNOWLEDGE_UNUSED(sgn);
    ACKNOWLEDGE_UNUSED(cameraGUID);
    ACKNOWLEDGE_UNUSED(posOffset);
    ACKNOWLEDGE_UNUSED(rotationOffset);

    _cameraUpdated[to_base(_context.getRenderStage().stage())] = true;
}

void Terrain::initialiseDrawCommands(SceneGraphNode& sgn,
                                     const RenderStagePass& renderStagePass,
                                     GenericDrawCommands& drawCommandsInOut) {
   
    GenericDrawCommand cmd;
    cmd.primitiveType(PrimitiveType::PATCH);
    cmd.enableOption(GenericDrawCommand::RenderOptions::RENDER_TESSELLATED);
    cmd.sourceBuffer(getGeometryVB());
    cmd.patchVertexCount(4);
    cmd.cmd().indexCount = getGeometryVB()->getIndexCount();
    drawCommandsInOut.push_back(cmd);

    if (renderStagePass.stage() == RenderStage::DISPLAY) {

        PipelineDescriptor pipelineDescriptor;
        pipelineDescriptor._shaderProgram = 
            (renderStagePass.pass() == RenderPassType::DEPTH_PASS
                                     ? _planeDepthShader
                                     : _planeShader);

        //infinite plane
        GenericDrawCommand planeCmd;
        planeCmd.primitiveType(PrimitiveType::TRIANGLE_STRIP);
        planeCmd.cmd().firstIndex = 0;
        planeCmd.cmd().indexCount = _plane->getGeometryVB()->getIndexCount();
        planeCmd.LoD(0);
        planeCmd.sourceBuffer(_plane->getGeometryVB());
        planeCmd.pipeline(_context.newPipeline(pipelineDescriptor));
        drawCommandsInOut.push_back(planeCmd);

        //BoundingBoxes
        GenericDrawCommands commands;
        commands.reserve(getQuadtree().getChunkCount());

        _terrainQuadtree.drawBBox(_context, commands);
        for (const GenericDrawCommand& crtCmd : commands) {
            drawCommandsInOut.push_back(crtCmd);
        }
    }

    Object3D::initialiseDrawCommands(sgn, renderStagePass, drawCommandsInOut);

    _cameraUpdated[to_base(renderStagePass.stage())] = true;
}

void Terrain::updateDrawCommands(SceneGraphNode& sgn,
                                 const RenderStagePass& renderStagePass,
                                 const SceneRenderState& sceneRenderState,
                                 GenericDrawCommands& drawCommandsInOut) {
    _context.setClipPlane(ClipPlaneIndex::CLIP_PLANE_0, Plane<F32>(WORLD_Y_AXIS, _waterHeight));

    const U8 stageIndex = to_U8(renderStagePass.stage());
    bool& cameraUpdated = _cameraUpdated[stageIndex];
    TerrainTessellator& tessellator = _terrainTessellator[stageIndex];

    if (cameraUpdated) {
        const vec3<F32>& newEye = Camera::activeCamera()->getEye();
        if (tessellator.getEye() != newEye) {
            tessellator.createTree(Camera::activeCamera()->getEye(),
                                   vec3<F32>(0),
                                   _terrainDimensions);
            tessellator.updateRenderData();
        }
        
        cameraUpdated = false;
    }

    drawCommandsInOut.front().drawCount(tessellator.renderDepth());
    STUBBED("This may cause stalls. Profile! -Ionut");
#if 0
    _shaderData->updateData(0, depth, (bufferPtr)tessellator.renderData().data());
#else
    _shaderData->setData((bufferPtr)tessellator.renderData().data());
#endif

    if (renderStagePass.stage() == RenderStage::DISPLAY) {
        // draw infinite plane
        assert(drawCommandsInOut[1].drawCount() == 1);

        PipelineDescriptor descriptor = drawCommandsInOut[1].pipeline().toDescriptor();
        descriptor._shaderProgram = (renderStagePass.pass() == RenderPassType::DEPTH_PASS
                                                             ? _planeDepthShader
                                                             : _planeShader);

        drawCommandsInOut[1].pipeline(_context.newPipeline(descriptor));

        size_t i = 2;

        if (_drawBBoxes) {
            GenericDrawCommands commands;
            commands.reserve(getQuadtree().getChunkCount());
            _terrainQuadtree.drawBBox(_context, commands);

            for (const GenericDrawCommand& bbCmd : commands) {
                drawCommandsInOut[i++] = bbCmd;
            }

        } else {
            std::for_each(std::begin(drawCommandsInOut) + i,
                          std::end(drawCommandsInOut),
                          [](GenericDrawCommand& bbCmd) {
                                bbCmd.drawCount(0);
                          });
        }
    }

    Object3D::updateDrawCommands(sgn, renderStagePass, sceneRenderState, drawCommandsInOut);
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
    assert(!(x_clampf < .0f || z_clampf < .0f || x_clampf > 1.0f ||
             z_clampf > 1.0f));

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
    assert(!(x_clampf < .0f || z_clampf < .0f || x_clampf > 1.0f ||
             z_clampf > 1.0f));

    vec2<F32> posF(x_clampf * _terrainDimensions.x,
                   z_clampf * _terrainDimensions.y);
    vec2<I32> posI(to_I32(x_clampf * _terrainDimensions.x),
                   to_I32(z_clampf * _terrainDimensions.y));
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
    assert(!(x_clampf < .0f || z_clampf < .0f || x_clampf > 1.0f ||
             z_clampf > 1.0f));

    vec2<F32> posF(x_clampf * _terrainDimensions.x,
                   z_clampf * _terrainDimensions.y);
    vec2<I32> posI(to_I32(x_clampf * _terrainDimensions.x),
                   to_I32(z_clampf * _terrainDimensions.y));
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