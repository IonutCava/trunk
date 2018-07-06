#include "Headers/Terrain.h"
#include "Headers/TerrainChunk.h"
#include "Headers/TerrainDescriptor.h"

#include "Quadtree/Headers/QuadtreeNode.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Managers/Headers/SceneManager.h"

#include "Geometry/Material/Headers/Material.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

namespace Divide {

Terrain::Terrain()
    : Object3D(TERRAIN),
      _alphaTexturePresent(false),
      _plane(nullptr),
      _drawBBoxes(false),
      _vegetationGrassNode(nullptr),
      _underwaterDiffuseScale(100.0f),
      _terrainRenderStateHash(0),
      _terrainDepthRenderStateHash(0),
      _terrainReflectionRenderStateHash(0),
      _terrainInView(false),
      _planeInView(false) {
    getGeometryVB()->useLargeIndices(true);  //<32bit indices

    _albedoSampler = MemoryManager_NEW SamplerDescriptor();
    _albedoSampler->setWrapMode(TEXTURE_REPEAT);
    _albedoSampler->setAnisotropy(8);
    _albedoSampler->toggleMipMaps(true);
    _albedoSampler->toggleSRGBColorSpace(true);

    _normalSampler = MemoryManager_NEW SamplerDescriptor();
    _normalSampler->setWrapMode(TEXTURE_REPEAT);
    _normalSampler->setAnisotropy(8);
    _normalSampler->toggleMipMaps(true);
}

Terrain::~Terrain() {
    MemoryManager::DELETE(_albedoSampler);
    MemoryManager::DELETE(_normalSampler);
}

bool Terrain::unload() {
    MemoryManager::DELETE_VECTOR(_terrainTextures);

    RemoveResource(_vegDetails.grassBillboards);
    return SceneNode::unload();
}

void Terrain::postLoad(SceneGraphNode* const sgn) {
    SceneGraphNode* planeSGN = sgn->addNode(_plane);
    planeSGN->setActive(false);
    _plane->computeBoundingBox(planeSGN);
    computeBoundingBox(sgn);
    for (TerrainChunk* chunk : _terrainChunks) {
        sgn->addNode(TerrainChunkTerrainAttorney::getVegetation(*chunk));
    }
    SceneNode::postLoad(sgn);
}

void Terrain::buildQuadtree() {
    reserveTriangleCount((_terrainDimensions.x - 1) *
                         (_terrainDimensions.y - 1) * 2);
    _terrainQuadtree.Build(
        _boundingBox, vec2<U32>(_terrainDimensions.x, _terrainDimensions.y),
        _chunkSize, this);
    // The terrain's final bounding box is the QuadTree's root bounding box
    _boundingBox = _terrainQuadtree.computeBoundingBox();

    Material* mat = getMaterialTpl();
    for (U8 i = 0; i < 3; ++i) {
        ShaderProgram* const drawShader =
            mat->getShaderInfo(i == 0
                                   ? FINAL_STAGE
                                   : (i == 1 ? SHADOW_STAGE : Z_PRE_PASS_STAGE))
                .getProgram();
        drawShader->Uniform("dvd_waterHeight",
                            GET_ACTIVE_SCENE()->state().getWaterLevel());
        drawShader->Uniform("bbox_min", _boundingBox.getMin());
        drawShader->Uniform("bbox_extent", _boundingBox.getExtent());
        drawShader->UniformTexture("texWaterCaustics",
                                   ShaderProgram::TEXTURE_UNIT0);
        drawShader->UniformTexture("texUnderwaterAlbedo",
                                   ShaderProgram::TEXTURE_UNIT1);
        drawShader->UniformTexture("texUnderwaterDetail",
                                   ShaderProgram::TEXTURE_NORMALMAP);
        drawShader->Uniform("underwaterDiffuseScale", _underwaterDiffuseScale);

        U8 textureOffset = ShaderProgram::TEXTURE_NORMALMAP + 1;
        U8 layerOffset = 0;
        stringImpl layerIndex;
        for (U32 k = 0; k < _terrainTextures.size(); ++k) {
            layerOffset = k * 3 + textureOffset;
            layerIndex = Util::toString(k);
            TerrainTextureLayer* textureLayer = _terrainTextures[k];
            drawShader->UniformTexture("texBlend[" + layerIndex + "]",
                                       layerOffset);
            drawShader->UniformTexture("texTileMaps[" + layerIndex + "]",
                                       layerOffset + 1);
            drawShader->UniformTexture("texNormalMaps[" + layerIndex + "]",
                                       layerOffset + 2);

            getMaterialTpl()->addCustomTexture(textureLayer->blendMap(),
                                               layerOffset);
            getMaterialTpl()->addCustomTexture(textureLayer->tileMaps(),
                                               layerOffset + 1);
            getMaterialTpl()->addCustomTexture(textureLayer->normalMaps(),
                                               layerOffset + 2);

            drawShader->Uniform("diffuseScale[" + layerIndex + "]",
                                textureLayer->getDiffuseScales());
            drawShader->Uniform("detailScale[" + layerIndex + "]",
                                textureLayer->getDetailScales());
        }
    }
}

bool Terrain::computeBoundingBox(SceneGraphNode* const sgn) {
    // Inform the scenegraph of our new BB
    sgn->getBoundingBox() = _boundingBox;
    return SceneNode::computeBoundingBox(sgn);
}

bool Terrain::isInView(const SceneRenderState& sceneRenderState,
                       SceneGraphNode* const sgn, const bool distanceCheck) {
    _terrainInView = SceneNode::isInView(sceneRenderState, sgn, distanceCheck);
    _planeInView = _terrainInView
                       ? false
                       : _plane->isInView(sceneRenderState,
                                          sgn->getChildren()[0], distanceCheck);

    return _terrainInView || _planeInView;
}

void Terrain::sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn,
                          SceneState& sceneState) {
    _terrainQuadtree.sceneUpdate(deltaTime, sgn, sceneState);
    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}

void Terrain::getDrawCommands(SceneGraphNode* const sgn,
                              const RenderStage& currentRenderStage,
                              SceneRenderState& sceneRenderState,
                              vectorImpl<GenericDrawCommand>& drawCommandsOut) {
    size_t drawStateHash = 0;

    if (bitCompare(currentRenderStage, DEPTH_STAGE)) {
        drawStateHash = bitCompare(currentRenderStage, Z_PRE_PASS_STAGE)
                            ? _terrainRenderStateHash
                            : _terrainDepthRenderStateHash;
    } else {
        drawStateHash = bitCompare(currentRenderStage, REFLECTION_STAGE)
                            ? _terrainReflectionRenderStateHash
                            : _terrainRenderStateHash;
    }

    RenderingComponent* const renderable =
        sgn->getComponent<RenderingComponent>();
    assert(renderable != nullptr);

    ShaderProgram* drawShader = renderable->getDrawShader(
        bitCompare(currentRenderStage, REFLECTION_STAGE) ? FINAL_STAGE
                                                         : currentRenderStage);

    if (_terrainInView) {
        vectorImpl<GenericDrawCommand> tempCommands;
        tempCommands.reserve(_terrainQuadtree.getChunkCount());

        // draw ground
        _terrainQuadtree.createDrawCommands(sceneRenderState, tempCommands);

        std::sort(std::begin(tempCommands), std::end(tempCommands),
                  [](const GenericDrawCommand& a, const GenericDrawCommand& b) {
                      return a.LoD() < b.LoD();
                  });

        for (GenericDrawCommand& cmd : tempCommands) {
            cmd.renderWireframe(renderable->renderWireframe());
            cmd.stateHash(drawStateHash);
            cmd.shaderProgram(drawShader);
            cmd.sourceBuffer(getGeometryVB());
            drawCommandsOut.push_back(cmd);
        }
    }

    // draw infinite plane
    if (GFX_DEVICE.isCurrentRenderStage(FINAL_STAGE | Z_PRE_PASS_STAGE) &&
        _planeInView) {
        GenericDrawCommand cmd(TRIANGLE_STRIP, 0, 1);
        cmd.renderWireframe(renderable->renderWireframe());
        cmd.stateHash(drawStateHash);
        cmd.shaderProgram(drawShader);
        cmd.sourceBuffer(_plane->getGeometryVB());
        drawCommandsOut.push_back(cmd);
    }
}

void Terrain::postDrawBoundingBox(SceneGraphNode* const sgn) const {
    if (_drawBBoxes) {
        _terrainQuadtree.drawBBox();
    }
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
    vec2<I32> posI(static_cast<I32>(posF.x), static_cast<I32>(posF.y));
    vec2<F32> posD(posF.x - posI.x, posF.y - posI.y);

    if (posI.x >= (I32)_terrainDimensions.x - 1)
        posI.x = _terrainDimensions.x - 2;
    if (posI.y >= (I32)_terrainDimensions.y - 1)
        posI.y = _terrainDimensions.y - 2;

    assert(posI.x >= 0 && posI.x < static_cast<I32>(_terrainDimensions.x) - 1 &&
           posI.y >= 0 && posI.y < static_cast<I32>(_terrainDimensions.y) - 1);

    vec3<F32> pos(
        _boundingBox.getMin().x +
            x_clampf * (_boundingBox.getMax().x - _boundingBox.getMin().x),
        0.0f,
        _boundingBox.getMin().z +
            z_clampf * (_boundingBox.getMax().z - _boundingBox.getMin().z));

    const vectorImpl<vec3<F32> >& position = getGeometryVB()->getPosition();

    pos.y = (position[TER_COORD(posI.x, posI.y,
                                static_cast<I32>(_terrainDimensions.x))].y) *
                (1.0f - posD.x) * (1.0f - posD.y) +
            (position[TER_COORD(posI.x + 1, posI.y,
                                static_cast<I32>(_terrainDimensions.x))].y) *
                posD.x * (1.0f - posD.y) +
            (position[TER_COORD(posI.x, posI.y + 1,
                                static_cast<I32>(_terrainDimensions.x))].y) *
                (1.0f - posD.x) * posD.y +
            (position[TER_COORD(posI.x + 1, posI.y + 1,
                                static_cast<I32>(_terrainDimensions.x))].y) *
                posD.x * posD.y;

    return pos;
}

vec3<F32> Terrain::getNormal(F32 x_clampf, F32 z_clampf) const {
    assert(!(x_clampf < .0f || z_clampf < .0f || x_clampf > 1.0f ||
             z_clampf > 1.0f));

    vec2<F32> posF(x_clampf * _terrainDimensions.x,
                   z_clampf * _terrainDimensions.y);
    vec2<I32> posI(static_cast<I32>(x_clampf * _terrainDimensions.x),
                   static_cast<I32>(z_clampf * _terrainDimensions.y));
    vec2<F32> posD(posF.x - posI.x, posF.y - posI.y);

    if (posI.x >= static_cast<I32>(_terrainDimensions.x) - 1) {
        posI.x = static_cast<I32>(_terrainDimensions.x) - 2;
    }
    if (posI.y >= static_cast<I32>(_terrainDimensions.y) - 1) {
        posI.y = static_cast<I32>(_terrainDimensions.y) - 2;
    }
    assert(posI.x >= 0 && posI.x < static_cast<I32>(_terrainDimensions.x) - 1 &&
           posI.y >= 0 && posI.y < static_cast<I32>(_terrainDimensions.y) - 1);

    const vectorImpl<vec3<F32> >& normals = getGeometryVB()->getNormal();

    return (normals[TER_COORD(posI.x, posI.y,
                              static_cast<I32>(_terrainDimensions.x))]) *
               (1.0f - posD.x) * (1.0f - posD.y) +
           (normals[TER_COORD(posI.x + 1, posI.y,
                              static_cast<I32>(_terrainDimensions.x))]) *
               posD.x * (1.0f - posD.y) +
           (normals[TER_COORD(posI.x, posI.y + 1,
                              static_cast<I32>(_terrainDimensions.x))]) *
               (1.0f - posD.x) * posD.y +
           (normals[TER_COORD(posI.x + 1, posI.y + 1,
                              static_cast<I32>(_terrainDimensions.x))]) *
               posD.x * posD.y;
}

vec3<F32> Terrain::getTangent(F32 x_clampf, F32 z_clampf) const {
    assert(!(x_clampf < .0f || z_clampf < .0f || x_clampf > 1.0f ||
             z_clampf > 1.0f));

    vec2<F32> posF(x_clampf * _terrainDimensions.x,
                   z_clampf * _terrainDimensions.y);
    vec2<I32> posI(static_cast<I32>(x_clampf * _terrainDimensions.x),
                   static_cast<I32>(z_clampf * _terrainDimensions.y));
    vec2<F32> posD(posF.x - posI.x, posF.y - posI.y);

    if (posI.x >= static_cast<I32>(_terrainDimensions.x) - 1) {
        posI.x = static_cast<I32>(_terrainDimensions.x) - 2;
    }
    if (posI.y >= static_cast<I32>(_terrainDimensions.y) - 1) {
        posI.y = static_cast<I32>(_terrainDimensions.y) - 2;
    }
    assert(posI.x >= 0 && posI.x < static_cast<I32>(_terrainDimensions.x) - 1 &&
           posI.y >= 0 && posI.y < static_cast<I32>(_terrainDimensions.y) - 1);

    const vectorImpl<vec3<F32> >& tangents = getGeometryVB()->getTangent();

    return (tangents[TER_COORD(posI.x, posI.y,
                               static_cast<I32>(_terrainDimensions.x))]) *
               (1.0f - posD.x) * (1.0f - posD.y) +
           (tangents[TER_COORD(posI.x + 1, posI.y,
                               static_cast<I32>(_terrainDimensions.x))]) *
               posD.x * (1.0f - posD.y) +
           (tangents[TER_COORD(posI.x, posI.y + 1,
                               static_cast<I32>(_terrainDimensions.x))]) *
               (1.0f - posD.x) * posD.y +
           (tangents[TER_COORD(posI.x + 1, posI.y + 1,
                               static_cast<I32>(_terrainDimensions.x))]) *
               posD.x * posD.y;
}

TerrainTextureLayer::~TerrainTextureLayer() {
    RemoveResource(_blendMap);
    RemoveResource(_tileMaps);
    RemoveResource(_normalMaps);
}
};