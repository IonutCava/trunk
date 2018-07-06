#include "Headers/Terrain.h"
#include "Headers/TerrainChunk.h"
#include "Headers/TerrainDescriptor.h"

#include "Quadtree/Headers/Quadtree.h"
#include "Quadtree/Headers/QuadtreeNode.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Managers/Headers/SceneManager.h"

#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Hardware/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

Terrain::Terrain() : Object3D(TERRAIN, TRIANGLE_STRIP),
    _alphaTexturePresent(false),
    _terrainWidth(0),
    _terrainHeight(0),
    _terrainQuadtree(New Quadtree()),
    _plane(nullptr),
    _drawBBoxes(false),
    _vegetationGrassNode(nullptr),
    _causticsTex(nullptr),
    _underwaterAlbedoTex(nullptr),
    _underwaterDetailTex(nullptr),
    _underwaterDiffuseScale(100.0f),
    _terrainRenderStateHash(0),
    _terrainDepthRenderStateHash(0),
    _terrainReflectionRenderStateHash(0)
{
    _geometry->useLargeIndices(true);//<32bit indices
}

Terrain::~Terrain()
{
}

bool Terrain::unload(){
    SAFE_DELETE(_terrainQuadtree);

    for (TerrainTextureLayer*& terrainTextures : _terrainTextures){
        SAFE_DELETE(terrainTextures);
    }
    _terrainTextures.clear();

    RemoveResource(_causticsTex);
    RemoveResource(_underwaterAlbedoTex);
    RemoveResource(_underwaterDetailTex);
    RemoveResource(_vegDetails.grassBillboards);
    return SceneNode::unload();
}

void Terrain::postLoad(SceneGraphNode* const sgn){
    assert(getState() == RES_LOADED);

    _geometry->Create();
    _geometry->computeTriangles(false);
    _geometry->reserveTriangleCount((_terrainWidth - 1) * (_terrainHeight - 1) * 2);
    _terrainQuadtree->Build(_boundingBox, vec2<U32>(_terrainWidth, _terrainHeight), _chunkSize, sgn);

    _drawShader = getMaterial()->getShaderInfo().getProgram();
    _drawShader->Uniform("dvd_waterHeight", GET_ACTIVE_SCENE()->state().getWaterLevel());
    _drawShader->Uniform("bbox_min", _boundingBox.getMin());
    _drawShader->Uniform("bbox_extent", _boundingBox.getExtent());
    _drawShader->UniformTexture("texWaterCaustics", 0);
    _drawShader->UniformTexture("texUnderwaterAlbedo", 1);
    _drawShader->UniformTexture("texUnderwaterDetail", 2);
    _drawShader->Uniform("underwaterDiffuseScale", _underwaterDiffuseScale);
    U8 textureOffset = 3;
    U8 layerOffset = 0;
    std::string layerIndex;
    for (U32 i = 0; i < _terrainTextures.size(); ++i){
        layerOffset = i * 2 + textureOffset;
        layerIndex = Util::toString(i);
        TerrainTextureLayer* textureLayer = _terrainTextures[i];
        _drawShader->UniformTexture("texBlend[" + layerIndex + "]", layerOffset);
        _drawShader->UniformTexture("texTileMaps[" + layerIndex + "]", layerOffset + 1);
        
        _drawShader->Uniform("diffuseScale[" + layerIndex + "]", textureLayer->getDiffuseScales());
        _drawShader->Uniform("detailScale[" + layerIndex + "]", textureLayer->getDetailScales());

    }

    ResourceDescriptor infinitePlane("infinitePlane");
    infinitePlane.setFlag(true); //No default material
    _plane = CreateResource<Quad3D>(infinitePlane);
    // our bottom plane is placed at the bottom of our water entity
    F32 height = GET_ACTIVE_SCENE()->state().getWaterLevel() - GET_ACTIVE_SCENE()->state().getWaterDepth();
    _farPlane = 2.0f * GET_ACTIVE_SCENE()->state().getRenderState().getCameraConst().getZPlanes().y;
    _plane->setCorner(Quad3D::TOP_LEFT,     vec3<F32>(-_farPlane * 1.5f, height, -_farPlane * 1.5f));
    _plane->setCorner(Quad3D::TOP_RIGHT,    vec3<F32>( _farPlane * 1.5f, height, -_farPlane * 1.5f));
    _plane->setCorner(Quad3D::BOTTOM_LEFT,  vec3<F32>(-_farPlane * 1.5f, height,  _farPlane * 1.5f));
    _plane->setCorner(Quad3D::BOTTOM_RIGHT, vec3<F32>( _farPlane * 1.5f, height,  _farPlane * 1.5f));
    SceneGraphNode* planeSGN = sgn->addNode(_plane);
    planeSGN->setActive(false);
    _plane->computeBoundingBox(planeSGN);
    _plane->renderInstance()->preDraw(true);
    _plane->renderInstance()->transform(planeSGN->getTransform());
    computeBoundingBox(sgn);

    SceneNode::postLoad(sgn);
}

bool Terrain::computeBoundingBox(SceneGraphNode* const sgn){
    //The terrain's final bounding box is the QuadTree's root bounding box
    _boundingBox = _terrainQuadtree->computeBoundingBox();
    //Inform the scenegraph of our new BB
    sgn->getBoundingBox() = _boundingBox;
    return  SceneNode::computeBoundingBox(sgn);
}

void Terrain::sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState){
    _terrainQuadtree->sceneUpdate(deltaTime, sgn, sceneState);
    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}

bool Terrain::prepareMaterial(SceneGraphNode* const sgn){
    if (!GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE | REFLECTION_STAGE))
        return false;

    LightManager& lightMgr = LightManager::getInstance();

    _causticsTex->Bind(0);
    _underwaterAlbedoTex->Bind(1);
    _underwaterDetailTex->Bind(2);
    U8 textureOffset = 3;
    for (U8 i = 0; i < _terrainTextures.size(); ++i){
        _terrainTextures[i]->bindTextures((i * 2) + textureOffset);
    }

    SET_STATE_BLOCK(GFX_DEVICE.isCurrentRenderStage(REFLECTION_STAGE) ? _terrainReflectionRenderStateHash : _terrainRenderStateHash);

    Material::ShaderInfo& shaderInfo = getMaterial()->getShaderInfo();
    getMaterial()->UploadToShader(shaderInfo);
    bool temp = lightMgr.shadowMappingEnabled() && sgn->getReceivesShadows();
    if(shaderInfo.getTrackedBool(1) != temp){
        shaderInfo.setTrackedBool(1, temp);
        _drawShader->Uniform("dvd_enableShadowMapping", temp);
    }        

    _plane->setCustomShader(_drawShader);

    return _drawShader->bind();
}

bool Terrain::prepareDepthMaterial(SceneGraphNode* const sgn){
    bool depthPrePass = GFX_DEVICE.isDepthPrePass();

    SET_STATE_BLOCK(depthPrePass ? _terrainRenderStateHash : _terrainDepthRenderStateHash);
     _plane->setCustomShader(_drawShader);

    return _drawShader->bind();
}

void Terrain::render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState){
    // draw ground
    _terrainQuadtree->CreateDrawCommands(sceneRenderState);

    for(VertexBuffer::DeferredDrawCommand& cmd : _drawCommands[0]){
        cmd._lodIndex = 0;
        _renderInstance->addDeferredDrawCommand(cmd);
    }
    _drawCommands[0].resize(0);
    for(VertexBuffer::DeferredDrawCommand& cmd : _drawCommands[1]){
        cmd._lodIndex = 1;
        _renderInstance->addDeferredDrawCommand(cmd);
    }
    _drawCommands[1].resize(0);

    Object3D::render(sgn, sceneRenderState);

    // draw infinite plane
    if (GFX_DEVICE.isCurrentRenderStage(FINAL_STAGE | Z_PRE_PASS_STAGE))
        GFX_DEVICE.renderInstance(_plane->renderInstance());
}

void Terrain::drawBoundingBox(SceneGraphNode* const sgn) const {
    if(_drawBBoxes)	_terrainQuadtree->DrawBBox();
}

vec3<F32> Terrain::getPositionFromGlobal(F32 x, F32 z) const {
    x -= _boundingBox.getCenter().x;
    z -= _boundingBox.getCenter().z;
    F32 xClamp = (0.5f * _terrainWidth) + x;
    F32 zClamp = (0.5f * _terrainHeight) - z;
    xClamp /= _terrainWidth;
    zClamp /= _terrainHeight;
    zClamp = 1 - zClamp;
    vec3<F32> temp = getPosition(xClamp,zClamp);

    return temp;
}

vec3<F32> Terrain::getPosition(F32 x_clampf, F32 z_clampf) const{
    assert(!(x_clampf<.0f || z_clampf<.0f || x_clampf>1.0f || z_clampf>1.0f));

    vec2<F32>  posF(x_clampf * _terrainWidth, z_clampf * _terrainHeight );
    vec2<I32>  posI((I32)(posF.x),   (I32)(posF.y) );
    vec2<F32>  posD(posF.x - posI.x, posF.y - posI.y );

    if(posI.x >= (I32)_terrainWidth - 1)	posI.x = _terrainWidth  - 2;
    if(posI.y >= (I32)_terrainHeight - 1)	posI.y = _terrainHeight - 2;

    assert(posI.x >= 0 && posI.x < (I32)(_terrainWidth)-1 && posI.y >= 0 && posI.y < (I32)(_terrainHeight) - 1);

    vec3<F32> pos(_boundingBox.getMin().x + x_clampf * (_boundingBox.getMax().x - _boundingBox.getMin().x), 0.0f,
                  _boundingBox.getMin().z + z_clampf * (_boundingBox.getMax().z - _boundingBox.getMin().z));

    const vectorImpl<vec3<F32> >& position = _geometry->getPosition();

    pos.y = (position[TER_COORD(posI.x,     posI.y,     (I32)_terrainWidth)].y) * (1.0f - posD.x) * (1.0f - posD.y)
          + (position[TER_COORD(posI.x + 1, posI.y,     (I32)_terrainWidth)].y) *         posD.x  * (1.0f - posD.y)
          + (position[TER_COORD(posI.x,     posI.y + 1, (I32)_terrainWidth)].y) * (1.0f - posD.x) *         posD.y
          + (position[TER_COORD(posI.x + 1, posI.y + 1, (I32)_terrainWidth)].y) *         posD.x  *         posD.y;

    return pos;
}

vec3<F32> Terrain::getNormal(F32 x_clampf, F32 z_clampf) const{
    assert(!(x_clampf<.0f || z_clampf<.0f || x_clampf>1.0f || z_clampf>1.0f));

    vec2<F32>  posF(	x_clampf * _terrainWidth, z_clampf * _terrainHeight );
    vec2<I32>  posI(	(I32)(x_clampf * _terrainWidth), (I32)(z_clampf * _terrainHeight) );
    vec2<F32>  posD(	posF.x - posI.x, posF.y - posI.y );

    if(posI.x >= (I32)(_terrainWidth)-1)    posI.x = (I32)(_terrainWidth) - 2;
    if(posI.y >= (I32)(_terrainHeight)-1)	posI.y = (I32)(_terrainHeight) - 2;

    assert(posI.x >= 0 && posI.x < (I32)(_terrainWidth)-1 && posI.y >= 0 && posI.y < (I32)(_terrainHeight)-1);

    const vectorImpl<vec3<F32> >& normals = _geometry->getNormal();

    return (normals[TER_COORD(posI.x,     posI.y,     (I32)_terrainWidth)]) * (1.0f - posD.x) * (1.0f - posD.y)
         + (normals[TER_COORD(posI.x + 1, posI.y,     (I32)_terrainWidth)]) *         posD.x  * (1.0f - posD.y)
         + (normals[TER_COORD(posI.x,     posI.y + 1, (I32)_terrainWidth)]) * (1.0f - posD.x) *         posD.y
         + (normals[TER_COORD(posI.x + 1, posI.y + 1, (I32)_terrainWidth)]) *         posD.x  *         posD.y;
}

vec3<F32> Terrain::getTangent(F32 x_clampf, F32 z_clampf) const{
    assert(!(x_clampf<.0f || z_clampf<.0f || x_clampf>1.0f || z_clampf>1.0f));

    vec2<F32> posF(	x_clampf * _terrainWidth, z_clampf * _terrainHeight );
    vec2<I32> posI(	(I32)(x_clampf * _terrainWidth), (I32)(z_clampf * _terrainHeight) );
    vec2<F32> posD(	posF.x - posI.x, posF.y - posI.y );

    if (posI.x >= (I32)(_terrainWidth)-1)    posI.x = (I32)(_terrainWidth)-2;
    if (posI.y >= (I32)(_terrainHeight)-1)	posI.y = (I32)(_terrainHeight)-2;

    assert(posI.x >= 0 && posI.x < (I32)(_terrainWidth)-1 && posI.y >= 0 && posI.y < (I32)(_terrainHeight)-1);

    const vectorImpl<vec3<F32> >& tangents = _geometry->getTangent();

    return (tangents[TER_COORD(posI.x,     posI.y,     (I32)_terrainWidth)]) * (1.0f - posD.x) * (1.0f - posD.y)
         + (tangents[TER_COORD(posI.x + 1, posI.y,     (I32)_terrainWidth)]) *         posD.x  * (1.0f - posD.y)
         + (tangents[TER_COORD(posI.x,     posI.y + 1, (I32)_terrainWidth)]) * (1.0f - posD.x) *         posD.y
         + (tangents[TER_COORD(posI.x + 1, posI.y + 1, (I32)_terrainWidth)]) *         posD.x  *         posD.y;
}

void Terrain::setCausticsTex(Texture* causticTexture) {
    RemoveResource(_causticsTex);
    _causticsTex = causticTexture;
}

void Terrain::setUnderwaterAlbedoTex(Texture* underwaterAlbedoTexture) {
    RemoveResource(_underwaterAlbedoTex);
    _underwaterAlbedoTex = underwaterAlbedoTexture;
}

void Terrain::setUnderwaterDetailTex(Texture* underwaterDetailTexture) {
    RemoveResource(_underwaterDetailTex);
    _underwaterDetailTex = underwaterDetailTexture;
}

TerrainTextureLayer::~TerrainTextureLayer()
{
    RemoveResource(_blendMap);
    RemoveResource(_tileMaps);
}

void TerrainTextureLayer::bindTextures(U32 offset){
    _lastOffset = offset;
    if (_blendMap)  _blendMap->Bind(_lastOffset);
    if (_tileMaps)  _tileMaps->Bind(_lastOffset + 1);
}