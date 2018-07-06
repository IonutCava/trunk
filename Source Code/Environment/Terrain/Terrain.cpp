#include "Headers/Terrain.h"
#include "Headers/TerrainChunk.h"
#include "Headers/TerrainDescriptor.h"

#include "Quadtree/Headers/QuadtreeNode.h"

#include "Core/Headers/PlatformContext.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Managers/Headers/SceneManager.h"

#include "Geometry/Material/Headers/Material.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

namespace Divide {


namespace {
    size_t g_PlaneCommandIndex = 0;
};

Terrain::Terrain(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name)
    : Object3D(context, parentCache, descriptorHash, name, ObjectType::TERRAIN, ObjectFlag::OBJECT_FLAG_NONE),
      _plane(nullptr),
      _drawBBoxes(false),
      _underwaterDiffuseScale(100.0f),
      _chunkSize(1),
      _waterHeight(0.0f)
{
    getGeometryVB()->useLargeIndices(true);  //<32bit indices
}

Terrain::~Terrain()
{
}

bool Terrain::unload() {
    MemoryManager::DELETE_VECTOR(_terrainTextures);
    return Object3D::unload();
}

void Terrain::postLoad(SceneGraphNode& sgn) {
    static const U32 normalMask = to_const_U32(SGNComponent::ComponentType::NAVIGATION) |
                                  to_const_U32(SGNComponent::ComponentType::PHYSICS) |
                                  to_const_U32(SGNComponent::ComponentType::BOUNDS) |
                                  to_const_U32(SGNComponent::ComponentType::RENDERING) |
                                  to_const_U32(SGNComponent::ComponentType::NETWORKING);

    SceneGraphNode_ptr planeSGN(sgn.addNode(_plane, normalMask, PhysicsGroup::GROUP_STATIC));
    planeSGN->setActive(false);
    /*for (TerrainChunk* chunk : _terrainChunks) {
        SceneGraphNode_ptr vegetation = sgn.addNode(Attorney::TerrainChunkTerrain::getVegetation(*chunk), normalMask);
        vegetation->lockVisibility(true);
    }*/
    // Skip Object3D::load() to avoid triangle list computation (extremely expensive)!!!

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

    for (U8 pass = 0; pass < to_const_U8(RenderPassType::COUNT); ++pass) {
        for (U32 i = 0; i < to_const_U32(RenderStage::COUNT); ++i) {
            RenderStage stage = static_cast<RenderStage>(i);

            const ShaderProgram_ptr& drawShader = mat->getShaderInfo(RenderStagePass(stage, static_cast<RenderPassType>(pass))).getProgram();

            drawShader->Uniform("bbox_min", bbMin);
            drawShader->Uniform("bbox_extent", bbExtent);
            drawShader->Uniform("underwaterDiffuseScale", _underwaterDiffuseScale);

            U8 textureOffset = to_const_U8(ShaderProgram::TextureUsage::COUNT);
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

                if (pass == 0 && i == 0 ) {
                    getMaterialTpl()->addCustomTexture(textureLayer->blendMap(), layerOffset);
                    getMaterialTpl()->addCustomTexture(textureLayer->tileMaps(), layerOffset + 1);
                    getMaterialTpl()->addCustomTexture(textureLayer->normalMaps(), layerOffset + 2);
                }
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

void Terrain::initialiseDrawCommands(SceneGraphNode& sgn,
                                     const RenderStagePass& renderStagePass,
                                     GenericDrawCommands& drawCommandsInOut) {

    GenericDrawCommand cmd;
    cmd.sourceBuffer(getGeometryVB());
    cmd.primitiveType(PrimitiveType::TRIANGLE_STRIP);
    drawCommandsInOut.push_back(cmd);

    size_t chunkCount = static_cast<size_t>(_terrainQuadtree.getChunkCount());
    for (U32 i = 0; i < chunkCount; ++i) {
        drawCommandsInOut.push_back(cmd);
    }

    g_PlaneCommandIndex = drawCommandsInOut.size();
    if (renderStagePass._stage == RenderStage::DISPLAY) {
        //infinite plane
        VertexBuffer* const vb = _plane->getGeometryVB();
        cmd.cmd().firstIndex = 0;
        cmd.cmd().indexCount = vb->getIndexCount();
        cmd.LoD(0);
        cmd.sourceBuffer(vb);
        drawCommandsInOut.push_back(cmd);

        //BoundingBoxes
        GenericDrawCommands commands;
        commands.reserve(chunkCount);
        _terrainQuadtree.drawBBox(_context, commands);
        for (const GenericDrawCommand& crtCmd : commands) {
            drawCommandsInOut.push_back(crtCmd);
        }
    }

    Object3D::initialiseDrawCommands(sgn, renderStagePass, drawCommandsInOut);
}

void Terrain::updateDrawCommands(SceneGraphNode& sgn,
                                 const RenderStagePass& renderStagePass,
                                 const SceneRenderState& sceneRenderState,
                                 GenericDrawCommands& drawCommandsInOut) {

    _context.setClipPlane(ClipPlaneIndex::CLIP_PLANE_0, Plane<F32>(WORLD_Y_AXIS, _waterHeight));

    drawCommandsInOut.front().shaderProgram()->Uniform("dvd_waterHeight", _waterHeight);
    vectorImpl<vec3<U32>> chunkData;
    size_t chunkCount = static_cast<size_t>(_terrainQuadtree.getChunkCount());
    chunkData.reserve(chunkCount);
    _terrainQuadtree.getChunkBufferData(renderStagePass._stage, sceneRenderState, chunkData);

    std::sort(std::begin(chunkData), std::end(chunkData),
                [](const vec3<U32>& a, const vec3<U32>& b) {
                    // LoD comparison
                    return a.z < b.z; 
                });

    size_t i = 0; size_t dataCount = chunkData.size();
    for (;i < chunkCount; ++i) {
        GenericDrawCommand& cmd = drawCommandsInOut[i + 1];
        if (i < dataCount) {
            const vec3<U32>& cmdData = chunkData[i];
            cmd.cmd().firstIndex = cmdData.x;
            cmd.cmd().indexCount = cmdData.y;
            cmd.LoD(to_I8(cmdData.z));
            cmd.drawCount(1);
        }  else {
            cmd.drawCount(0);
        }
    }

    if (renderStagePass._stage == RenderStage::DISPLAY) {
        // draw infinite plane
        assert(drawCommandsInOut[g_PlaneCommandIndex].drawCount() == 1);

        i = g_PlaneCommandIndex + 1;

        if (_drawBBoxes) {
            GenericDrawCommands commands;
            commands.reserve(chunkCount);
            _terrainQuadtree.drawBBox(_context, commands);

            for (const GenericDrawCommand& cmd : commands) {
                drawCommandsInOut[i++] = cmd;
            }

        } else {
            std::for_each(std::begin(drawCommandsInOut) + i,
                          std::end(drawCommandsInOut),
                          [](GenericDrawCommand& cmd) {
                                cmd.drawCount(0);
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


    const VertexBuffer* const vb = getGeometryVB();

    pos.y = (vb->getPosition(TER_COORD(posI.x, posI.y, to_I32(_terrainDimensions.x))).y) *
            (1.0f - posD.x) * (1.0f - posD.y) + 
            (vb->getPosition(TER_COORD(posI.x + 1, posI.y, to_I32(_terrainDimensions.x))).y) *
            posD.x * (1.0f - posD.y) +
            (vb->getPosition(TER_COORD(posI.x, posI.y + 1, to_I32(_terrainDimensions.x))).y) *
            (1.0f - posD.x) * posD.y +
            (vb->getPosition(TER_COORD(posI.x + 1, posI.y + 1,to_I32(_terrainDimensions.x))).y) *
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

    const VertexBuffer* const vb = getGeometryVB();
    vec3<F32> normals[4];

    vb->getNormal(TER_COORD(posI.x, posI.y, to_I32(_terrainDimensions.x)), normals[0]);
    vb->getNormal(TER_COORD(posI.x + 1, posI.y, to_I32(_terrainDimensions.x)), normals[1]);
    vb->getNormal(TER_COORD(posI.x, posI.y + 1, to_I32(_terrainDimensions.x)), normals[2]);
    vb->getNormal(TER_COORD(posI.x + 1, posI.y + 1, to_I32(_terrainDimensions.x)), normals[3]);

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

    const VertexBuffer* const vb = getGeometryVB();
    vec3<F32> tangents[4];


    vb->getTangent(TER_COORD(posI.x, posI.y, to_I32(_terrainDimensions.x)), tangents[0]);
    vb->getTangent(TER_COORD(posI.x + 1, posI.y, to_I32(_terrainDimensions.x)), tangents[1]);
    vb->getTangent(TER_COORD(posI.x, posI.y + 1, to_I32(_terrainDimensions.x)), tangents[2]);
    vb->getTangent(TER_COORD(posI.x + 1, posI.y + 1, to_I32(_terrainDimensions.x)), tangents[3]);

    return tangents[0] * (1.0f - posD.x) * (1.0f - posD.y) +
           tangents[1] * posD.x * (1.0f - posD.y) +
           tangents[2] * (1.0f - posD.x) * posD.y +
           tangents[3] * posD.x * posD.y;
}

TerrainTextureLayer::~TerrainTextureLayer()
{
}
};