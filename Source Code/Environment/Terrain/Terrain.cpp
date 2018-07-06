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

Terrain::Terrain() : SceneNode(TYPE_TERRAIN),
    _alphaTexturePresent(false),
    _terrainWidth(0),
    _terrainHeight(0),
    _groundVB(nullptr),
    _terrainQuadtree(New Quadtree()),
    _plane(nullptr),
    _drawBBoxes(false),
    _shadowMapped(true),
    _vegetation(nullptr),
    _vegetationGrassNode(nullptr),
    _planeTransform(nullptr),
    _terrainTransform(nullptr),
    _node(nullptr),
    _planeSGN(nullptr),
    _causticsTex(nullptr),
    _underwaterAlbedoTex(nullptr),
    _underwaterDetailTex(nullptr),
    _underwaterDiffuseScale(100.0f),
    _terrainRenderState(nullptr),
    _terrainDepthRenderState(nullptr),
    _terrainPrePassRenderState(nullptr),
    _terrainReflectionRenderState(nullptr),
    _stateRefreshIntervalBuffer(0ULL),
    _stateRefreshInterval(getSecToUs(0.5)) ///<Every half a second
{
    _groundVB = GFX_DEVICE.newVB(TRIANGLE_STRIP);
    _groundVB->useLargeIndices(true);//<32bit indices
}

Terrain::~Terrain()
{
    SAFE_DELETE(_groundVB);
}

bool Terrain::unload(){

    for (TerrainTextureLayer*& terrainTextures : _terrainTextures){
        SAFE_DELETE(terrainTextures);
    }
    _terrainTextures.clear();

    SAFE_DELETE(_terrainQuadtree);
    RemoveResource(_causticsTex);
    RemoveResource(_underwaterAlbedoTex);
    RemoveResource(_underwaterDetailTex);

    return SceneNode::unload();
}

void Terrain::postLoad(SceneGraphNode* const sgn){
    if (getState() != RES_LOADED) {
        sgn->setActive(false);
    }
    // Terrain can't be added to multiple SGNs (currently)
    if (_node)
        return;

    _node = sgn;

    ShaderProgram* s = getMaterial()->getShaderProgram();

    _groundVB->Create();
    _terrainQuadtree->Build(_boundingBox, vec2<U32>(_terrainWidth, _terrainHeight), _chunkSize, _groundVB, this);

    s->Uniform("dvd_lightCount", 1);
    s->Uniform("underwaterDiffuseScale", _underwaterDiffuseScale);

    s->UniformTexture("texWaterCaustics", 0);
    s->UniformTexture("texUnderwaterAlbedo", 1);
    s->UniformTexture("texUnderwaterDetail", 2);

    U8 textureOffset = 3;
    U8 layerOffset = 0;
    std::string layerIndex;
    for (U32 i = 0; i < _terrainTextures.size(); ++i){
        layerOffset = i * TerrainTextureLayer::TEXTURE_USAGE_PLACEHOLDER + textureOffset;
        layerIndex = Util::toString(i);
        TerrainTextureLayer* textureLayer = _terrainTextures[i];
        s->UniformTexture("texBlend["        + layerIndex + "]", layerOffset + TerrainTextureLayer::TEXTURE_BLEND_MAP);
        s->UniformTexture("texAlbedoMaps["   + layerIndex + "]", layerOffset + TerrainTextureLayer::TEXTURE_ALBEDO_MAPS);
        s->UniformTexture("texDetailMaps["   + layerIndex + "]", layerOffset + TerrainTextureLayer::TEXTURE_DETAIL_MAPS);
        
        s->Uniform("redDiffuseScale["   + layerIndex + "]", textureLayer->getTextureScale(TerrainTextureLayer::TEXTURE_ALBEDO_MAPS, TerrainTextureLayer::TEXTURE_RED_CHANNEL));
        s->Uniform("greenDiffuseScale[" + layerIndex + "]", textureLayer->getTextureScale(TerrainTextureLayer::TEXTURE_ALBEDO_MAPS, TerrainTextureLayer::TEXTURE_GREEN_CHANNEL));
        s->Uniform("blueDiffuseScale["  + layerIndex + "]", textureLayer->getTextureScale(TerrainTextureLayer::TEXTURE_ALBEDO_MAPS, TerrainTextureLayer::TEXTURE_BLUE_CHANNEL));
        s->Uniform("alphaDiffuseScale[" + layerIndex + "]", textureLayer->getTextureScale(TerrainTextureLayer::TEXTURE_ALBEDO_MAPS, TerrainTextureLayer::TEXTURE_ALPHA_CHANNEL));

        s->Uniform("redDetailScale["   + layerIndex + "]", textureLayer->getTextureScale(TerrainTextureLayer::TEXTURE_DETAIL_MAPS, TerrainTextureLayer::TEXTURE_RED_CHANNEL));
        s->Uniform("greenDetailScale[" + layerIndex + "]", textureLayer->getTextureScale(TerrainTextureLayer::TEXTURE_DETAIL_MAPS, TerrainTextureLayer::TEXTURE_GREEN_CHANNEL));
        s->Uniform("blueDetailScale["  + layerIndex + "]", textureLayer->getTextureScale(TerrainTextureLayer::TEXTURE_DETAIL_MAPS, TerrainTextureLayer::TEXTURE_BLUE_CHANNEL));
        s->Uniform("alphaDetailScale[" + layerIndex + "]", textureLayer->getTextureScale(TerrainTextureLayer::TEXTURE_DETAIL_MAPS, TerrainTextureLayer::TEXTURE_ALPHA_CHANNEL));
    }
    _groundVB->setShaderProgram(s);

    ResourceDescriptor infinitePlane("infinitePlane");
    infinitePlane.setFlag(true); //No default material
    _plane = CreateResource<Quad3D>(infinitePlane);
    F32 depth = GET_ACTIVE_SCENE()->state().getWaterDepth();
    F32 height = GET_ACTIVE_SCENE()->state().getWaterLevel() - depth;
    _farPlane = 2.0f * GET_ACTIVE_SCENE()->state().getRenderState().getCameraConst().getZPlanes().y;
    _plane->setCorner(Quad3D::TOP_LEFT,     vec3<F32>(-_farPlane * 1.5f, height, -_farPlane * 1.5f));
    _plane->setCorner(Quad3D::TOP_RIGHT,    vec3<F32>( _farPlane * 1.5f, height, -_farPlane * 1.5f));
    _plane->setCorner(Quad3D::BOTTOM_LEFT,  vec3<F32>(-_farPlane * 1.5f, height,  _farPlane * 1.5f));
    _plane->setCorner(Quad3D::BOTTOM_RIGHT, vec3<F32>( _farPlane * 1.5f, height,  _farPlane * 1.5f));
    _planeSGN = _node->addNode(_plane);
    _plane->computeBoundingBox(_planeSGN);
    _plane->renderInstance()->preDraw(true);
    _planeSGN->setActive(false);
    _planeTransform = _planeSGN->getTransform();
    _plane->renderInstance()->transform(_planeTransform);
    computeBoundingBox(_node);

    if (_vegetation){
        _vegetationGrassNode = _node->addNode(_vegetation);
        _vegetation->initialize(this, _node);
        toggleVegetation(true);
    }

    SceneNode::postLoad(sgn);
}

bool Terrain::computeBoundingBox(SceneGraphNode* const sgn){
    //The terrain's final bounding box is the QuadTree's root bounding box
    //Compute the QuadTree boundingboxes and get the root BB
    _boundingBox = _terrainQuadtree->computeBoundingBox();
    //Inform the scenegraph of our new BB
    sgn->getBoundingBox() = _boundingBox;
    return  SceneNode::computeBoundingBox(sgn);
}

void Terrain::sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState){
    //Query shadow state every "_stateRefreshInterval" microseconds
    if (_stateRefreshIntervalBuffer >= _stateRefreshInterval){
        _shadowMapped = ParamHandler::getInstance().getParam<bool>("rendering.enableShadows") && sgn->getReceivesShadows();
        _stateRefreshIntervalBuffer -= _stateRefreshInterval;
    }
    _terrainTransform = sgn->getTransform();
    _terrainQuadtree->sceneUpdate(deltaTime, sgn, sceneState);
    _stateRefreshIntervalBuffer += deltaTime;
    _terrainTransform = sgn->getTransform();

    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}

bool Terrain::prepareMaterial(SceneGraphNode* const sgn){
    STUBBED("ToDo: Add multiple local lights for terrain, such as torches, rockets, flashlights etc - Ionut")
    if (!GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE | REFLECTION_STAGE))
        return true;

    //Prepare the main light (directional light only, sun) for now.
    //Find the most influential light for the terrain. Usually the Sun
    _lightCount = LightManager::getInstance().findLightsForSceneNode(sgn,LIGHT_TYPE_DIRECTIONAL);
    //Update lights for this node
    LightManager::getInstance().update();
    //Only 1 shadow map for terrains
    
    CLAMP<U8>(_lightCount, 0, 1);
    if(GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE)){
        U8 offset = Config::MAX_TEXTURE_STORAGE;
        for(U8 n = 0; n < _lightCount; n++, offset++){
            Light* l = LightManager::getInstance().getLightForCurrentNode(n);
            LightManager::getInstance().bindDepthMaps(l, n, offset);
        }
    }

    ShaderProgram* terrainShader = getMaterial()->getShaderProgram();
    _groundVB->setShaderProgram(terrainShader);
    _plane->setCustomShader(terrainShader);

    const vectorImpl<I32>& indices = LightManager::getInstance().getLightIndicesForCurrentNode();
    const vectorImpl<I32>& types = LightManager::getInstance().getLightTypesForCurrentNode();
    const vectorImpl<I32>& lightShadowCast = LightManager::getInstance().getShadowCastingLightsForCurrentNode();

    _causticsTex->Bind(0);
    _underwaterAlbedoTex->Bind(1);
    _underwaterDetailTex->Bind(2);
    for (U8 i = 0; i < _terrainTextures.size(); ++i){
        _terrainTextures[i]->bindTextures(i * TerrainTextureLayer::TEXTURE_USAGE_PLACEHOLDER + 3);
    }

    SET_STATE_BLOCK(*(GFX_DEVICE.isCurrentRenderStage(REFLECTION_STAGE) ? _terrainReflectionRenderState : _terrainRenderState));

    terrainShader->Uniform("material", getMaterial()->getMaterialMatrix());
    terrainShader->Uniform("dvd_enableShadowMapping", _shadowMapped);
    terrainShader->Uniform("dvd_lightType", types);
    terrainShader->Uniform("dvd_lightIndex", indices);
    terrainShader->Uniform("dvd_lightCastsShadows", lightShadowCast);
    terrainShader->Uniform("dvd_waterHeight", GET_ACTIVE_SCENE()->state().getWaterLevel());
    terrainShader->Uniform("bbox_min", _boundingBox.getMin());
    terrainShader->Uniform("bbox_extent", _boundingBox.getExtent());

    return terrainShader->bind();
}

bool Terrain::releaseMaterial(){
    if (!GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE | REFLECTION_STAGE))
        return true;

    for (U8 i = 0; i < _terrainTextures.size(); ++i){
        _terrainTextures[i]->unbindTextures();
    }
    _underwaterDetailTex->Unbind(2);
    _underwaterAlbedoTex->Unbind(1);
    _causticsTex->Unbind(0);

     U8 offset = (_lightCount - 1) + Config::MAX_TEXTURE_STORAGE;
     for(I32 n = _lightCount - 1; n >= 0; n--,offset--){
        Light* l = LightManager::getInstance().getLightForCurrentNode(n);
        LightManager::getInstance().unbindDepthMaps(l, offset);
    }

    return true;
}

bool Terrain::prepareDepthMaterial(SceneGraphNode* const sgn){
    bool depthPrePass = GFX_DEVICE.isDepthPrePass();

    SET_STATE_BLOCK(*(depthPrePass ? _terrainPrePassRenderState : _terrainDepthRenderState));
    ShaderProgram* terrainShader = getMaterial()->getShaderProgram(depthPrePass ? Z_PRE_PASS_STAGE : SHADOW_STAGE);

    _groundVB->setShaderProgram(terrainShader);
    _plane->setCustomShader(terrainShader);

    return terrainShader->bind();
}

bool Terrain::releaseDepthMaterial(){
    return true;
}

void Terrain::renderChunkCallback(U8 lod){
    _groundVB->currentShader()->SetLOD(lod);
    _groundVB->DrawRange(true);
}

void Terrain::render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState){
    drawGround(sceneRenderState);
    drawInfinitePlain(sceneRenderState);
}

void Terrain::drawBoundingBox(SceneGraphNode* const sgn) const {
    if(_drawBBoxes)	_terrainQuadtree->DrawBBox();
}

bool Terrain::onDraw(SceneGraphNode* const sgn, const RenderStage& currentStage){
     return true;
}

void Terrain::postDraw(SceneGraphNode* const sgn, const RenderStage& currentStage){
}

void Terrain::drawInfinitePlain(const SceneRenderState& sceneRenderState) const {
    if(!GFX_DEVICE.isCurrentRenderStage(FINAL_STAGE | Z_PRE_PASS_STAGE)) return;
    GFX_DEVICE.renderInstance(_plane->renderInstance());
}

void Terrain::drawGround(const SceneRenderState& sceneRenderState) const {
    GFX_DEVICE.updateStates();
    GFX_DEVICE.pushWorldMatrix(_terrainTransform->getGlobalMatrix(), _terrainTransform->isUniformScaled());
    if (_groundVB->SetActive()){
        _terrainQuadtree->DrawGround(sceneRenderState);
    }
    GFX_DEVICE.popWorldMatrix();
}

Vegetation* const Terrain::getVegetation() const {
    assert(_vegetationGrassNode != nullptr);
    return _vegetationGrassNode->getNode<Vegetation>();
}

void Terrain::toggleVegetation(bool state){ 
    assert(_vegetationGrassNode != nullptr);
    _vegetationGrassNode->getNode<Vegetation>()->toggleRendering(state);
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

    const vectorImpl<vec3<F32> >& position = _groundVB->getPosition();

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

    const vectorImpl<vec3<F32> >& normals = _groundVB->getNormal();

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

    const vectorImpl<vec3<F32> >& tangents = _groundVB->getTangent();

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
    for (U8 i = 0; i < TEXTURE_USAGE_PLACEHOLDER; ++i)
        RemoveResource(_texture[i]);
}

void TerrainTextureLayer::bindTextures(U32 offset){
    assert(_texture[TEXTURE_BLEND_MAP] && _texture[TEXTURE_ALBEDO_MAPS] && _texture[TEXTURE_DETAIL_MAPS]);

    _lastOffset = offset;
    if (_texture[TEXTURE_BLEND_MAP])   _texture[TEXTURE_BLEND_MAP]->Bind(_lastOffset   + TEXTURE_BLEND_MAP);
    if (_texture[TEXTURE_ALBEDO_MAPS]) _texture[TEXTURE_ALBEDO_MAPS]->Bind(_lastOffset + TEXTURE_ALBEDO_MAPS);
    if (_texture[TEXTURE_DETAIL_MAPS]) _texture[TEXTURE_DETAIL_MAPS]->Bind(_lastOffset + TEXTURE_DETAIL_MAPS);
    
}

void TerrainTextureLayer::unbindTextures(){
    if (_texture[TEXTURE_BLEND_MAP])   _texture[TEXTURE_BLEND_MAP]->Unbind(_lastOffset + TEXTURE_BLEND_MAP);
    if (_texture[TEXTURE_ALBEDO_MAPS]) _texture[TEXTURE_ALBEDO_MAPS]->Unbind(_lastOffset + TEXTURE_ALBEDO_MAPS);
    if (_texture[TEXTURE_DETAIL_MAPS]) _texture[TEXTURE_DETAIL_MAPS]->Unbind(_lastOffset + TEXTURE_DETAIL_MAPS);
}