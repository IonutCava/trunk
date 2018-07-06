#include "Headers/Terrain.h"
#include "Headers/TerrainChunk.h"
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

#define COORD(x,y,w)	((y)*(w)+(x))

Terrain::Terrain() : SceneNode(TYPE_TERRAIN),
    _alphaTexturePresent(false),
    _terrainWidth(0),
    _terrainHeight(0),
    _groundVB(nullptr),
    _terrainQuadtree(New Quadtree()),
    _plane(nullptr),
    _drawBBoxes(false),
    _shadowMapped(true),
    _drawReflected(false),
    _vegetationGrassNode(nullptr),
    _planeTransform(nullptr),
    _terrainTransform(nullptr),
    _node(nullptr),
    _planeSGN(nullptr),
    _terrainRenderState(nullptr),
    _terrainDepthRenderState(nullptr),
    _terrainReflectionRenderState(nullptr),
    _stateRefreshIntervalBuffer(0ULL),
    _diffuseUVScale(1.0f),
    _normalMapUVScale(1.0f),
    _stateRefreshInterval(getSecToUs(0.5)) ///<Every half a second
{
    _terrainTextures[TERRAIN_TEXTURE_DIFFUSE]   = nullptr;
    _terrainTextures[TERRAIN_TEXTURE_NORMALMAP] = nullptr;
    _terrainTextures[TERRAIN_TEXTURE_CAUSTICS]  = nullptr;
    _terrainTextures[TERRAIN_TEXTURE_RED]       = nullptr;
    _terrainTextures[TERRAIN_TEXTURE_GREEN]     = nullptr;
    _terrainTextures[TERRAIN_TEXTURE_BLUE]      = nullptr;
}

Terrain::~Terrain()
{
}

void Terrain::sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState){
    //Query shadow state every "_stateRefreshInterval" microseconds
    if (_stateRefreshIntervalBuffer >= _stateRefreshInterval){
        _shadowMapped = ParamHandler::getInstance().getParam<bool>("rendering.enableShadows") && sgn->getReceivesShadows();
        _stateRefreshIntervalBuffer -= _stateRefreshInterval;
    }
    _stateRefreshIntervalBuffer += deltaTime;

    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}

bool Terrain::prepareMaterial(SceneGraphNode* const sgn){
    STUBBED("ToDo: Add multiple local lights for terrain, such as torches, rockets, flashlights etc - Ionut")
    //Transform the Object (Rot, Trans, Scale)
    if(!GFX_DEVICE.excludeFromStateChange(getType())){ ///< only if the node is not in the exclusion mask
        _terrainTransform = sgn->getTransform();
    }

    //Prepare the main light (directional light only, sun) for now.
    if(!GFX_DEVICE.isCurrentRenderStage(SHADOW_STAGE)){
        //Find the most influential light for the terrain. Usually the Sun
        _lightCount = LightManager::getInstance().findLightsForSceneNode(sgn,LIGHT_TYPE_DIRECTIONAL);
        //Update lights for this node
        LightManager::getInstance().update();
        //Only 1 shadow map for terrains
    }
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

    _terrainTextures[TERRAIN_TEXTURE_DIFFUSE]->Bind(0);
    _terrainTextures[TERRAIN_TEXTURE_NORMALMAP]->Bind(1);
    _terrainTextures[TERRAIN_TEXTURE_CAUSTICS]->Bind(2); //Water Caustics
    _terrainTextures[TERRAIN_TEXTURE_RED]->Bind(3);      //AlphaMap: RED
    _terrainTextures[TERRAIN_TEXTURE_GREEN]->Bind(4);    //AlphaMap: GREEN
    _terrainTextures[TERRAIN_TEXTURE_BLUE]->Bind(5);     //AlphaMap: BLUE
    if(_alphaTexturePresent){
        _terrainTextures[TERRAIN_TEXTURE_ALPHA]->Bind(6); //AlphaMap: Alpha
    }

    if(!terrainShader->bind())
        return false;

    terrainShader->Uniform("material",getMaterial()->getMaterialMatrix());
    terrainShader->Uniform("dvd_enableShadowMapping", _shadowMapped);
    terrainShader->Uniform("dvd_lightType",types);
    terrainShader->Uniform("dvd_lightIndex", indices);
    terrainShader->Uniform("dvd_lightCastsShadows",lightShadowCast);

    if(GFX_DEVICE.isCurrentRenderStage(REFLECTION_STAGE)){
        SET_STATE_BLOCK(_terrainReflectionRenderState);
        terrainShader->Uniform("bbox_min", _boundingBox.getMax());
        terrainShader->Uniform("dvd_isReflection", true);
    }else{
        SET_STATE_BLOCK(_terrainRenderState);
        terrainShader->Uniform("bbox_min", _boundingBox.getMin());
        terrainShader->Uniform("dvd_isReflection", false);
    }

    return true;
}

bool Terrain::releaseMaterial(){
    if(_alphaTexturePresent) _terrainTextures[TERRAIN_TEXTURE_ALPHA]->Unbind(6);
    _terrainTextures[TERRAIN_TEXTURE_BLUE]->Unbind(5);
    _terrainTextures[TERRAIN_TEXTURE_GREEN]->Unbind(4);
    _terrainTextures[TERRAIN_TEXTURE_RED]->Unbind(3);
    _terrainTextures[TERRAIN_TEXTURE_CAUSTICS]->Unbind(2);
    _terrainTextures[TERRAIN_TEXTURE_NORMALMAP]->Unbind(1);
    _terrainTextures[TERRAIN_TEXTURE_DIFFUSE]->Unbind(0);

    if(GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE)){
        U8 offset = (_lightCount - 1) + Config::MAX_TEXTURE_STORAGE;
        for(I32 n = _lightCount - 1; n >= 0; n--,offset--){
            Light* l = LightManager::getInstance().getLightForCurrentNode(n);
            LightManager::getInstance().unbindDepthMaps(l, offset);
        }
    }
    return true;
}

bool Terrain::prepareDepthMaterial(SceneGraphNode* const sgn){
    if(!GFX_DEVICE.excludeFromStateChange(getType())){
        _terrainTransform = sgn->getTransform();
    }
    SET_STATE_BLOCK(_terrainDepthRenderState);
    ShaderProgram* terrainShader = getMaterial()->getShaderProgram(SHADOW_STAGE);
    _groundVB->setShaderProgram(terrainShader);
    _plane->setCustomShader(terrainShader);
    if(!terrainShader->bind())
        return false;

    terrainShader->Uniform("dvd_enableShadowMapping", false);

    return true;
}

bool Terrain::releaseDepthMaterial(){
    return true;
}

void Terrain::render(SceneGraphNode* const sgn){
    drawGround();
    drawInfinitePlain();
}

void Terrain::drawBoundingBox(SceneGraphNode* const sgn){
    if(_drawBBoxes)	_terrainQuadtree->DrawBBox();
}

bool Terrain::onDraw(const RenderStage& currentStage){
    _eyePos = GET_ACTIVE_SCENE()->renderState().getCamera().getEye();
     return true;
}

void Terrain::postDraw(const RenderStage& currentStage){
}

void Terrain::drawInfinitePlain(){
    if(GFX_DEVICE.isCurrentRenderStage(REFLECTION_STAGE | DEPTH_STAGE)) return;

    SET_STATE_BLOCK(_terrainDepthRenderState);
    _planeTransform->setPosition(vec3<F32>(_eyePos.x,_planeTransform->getPosition().y,_eyePos.z));
    _planeSGN->getBoundingBox().Transform(_planeSGN->getInitialBoundingBox(),
                                          _planeTransform->getGlobalMatrix());

    GFX_DEVICE.renderInstance(_plane->renderInstance());
}

void Terrain::drawGround() const{
    assert(_groundVB);
    _terrainQuadtree->DrawGround(_drawReflected);
}

Vegetation* const Terrain::getVegetation() const {
    assert(_vegetationGrassNode != nullptr);
    return _vegetationGrassNode->getNode<Vegetation>();
}

void Terrain::addVegetation(SceneGraphNode* const sgn, Vegetation* veg, std::string grassShader){
    _vegetationGrassNode = sgn->addNode(veg);
    _grassShader = grassShader;
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
    if(x_clampf<.0f || z_clampf<.0f || x_clampf>1.0f || z_clampf>1.0f)
        return VECTOR3_ZERO;

    vec2<F32>  posF(	x_clampf * _terrainWidth, z_clampf * _terrainHeight );
    vec2<U32>  posI(	(I32)(posF.x), (I32)(posF.y) );
    vec2<F32>  posD(	posF.x - posI.x, posF.y - posI.y );

    if(posI.x >= (U32)_terrainWidth-1)	posI.x = _terrainWidth-2;
    if(posI.y >= (U32)_terrainHeight-1)	posI.y = _terrainHeight-2;
    assert(posI.x>=0 && posI.x<(U32)_terrainWidth-1 && posI.y>=0 && posI.y<(U32)_terrainHeight-1);

    vec3<F32> pos(_boundingBox.getMin().x + x_clampf * (_boundingBox.getMax().x - _boundingBox.getMin().x), 0.0f, _boundingBox.getMin().z + z_clampf * (_boundingBox.getMax().z - _boundingBox.getMin().z));
    pos.y =   (_groundVB->getPosition()[ COORD(posI.x,  posI.y,  _terrainWidth) ].y)  * (1.0f-posD.x) * (1.0f-posD.y)
            + (_groundVB->getPosition()[ COORD(posI.x+1,posI.y,  _terrainWidth) ].y)  *       posD.x  * (1.0f-posD.y)
            + (_groundVB->getPosition()[ COORD(posI.x,  posI.y+1,_terrainWidth) ].y)  * (1.0f-posD.x) *       posD.y
            + (_groundVB->getPosition()[ COORD(posI.x+1,posI.y+1,_terrainWidth) ].y)  *       posD.x  *       posD.y;
    return pos;
}

vec3<F32> Terrain::getNormal(F32 x_clampf, F32 z_clampf) const{
    if(x_clampf<.0f || z_clampf<.0f || x_clampf>1.0f || z_clampf>1.0f) return WORLD_Y_AXIS;

    vec2<F32>  posF(	x_clampf * _terrainWidth, z_clampf * _terrainHeight );
    vec2<U32>  posI(	(I32)(x_clampf * _terrainWidth), (I32)(z_clampf * _terrainHeight) );
    vec2<F32>  posD(	posF.x - posI.x, posF.y - posI.y );

    if(posI.x >= (U32)_terrainWidth-1)		posI.x = _terrainWidth-2;
    if(posI.y >= (U32)_terrainHeight-1)	posI.y = _terrainHeight-2;
    assert(posI.x>=0 && posI.x<(U32)_terrainWidth-1 && posI.y>=0 && posI.y<(U32)_terrainHeight-1);

    return (_groundVB->getNormal()[ COORD(posI.x,  posI.y,  _terrainWidth) ])  * (1.0f-posD.x) * (1.0f-posD.y)
         + (_groundVB->getNormal()[ COORD(posI.x+1,posI.y,  _terrainWidth) ])  *       posD.x  * (1.0f-posD.y)
         + (_groundVB->getNormal()[ COORD(posI.x,  posI.y+1,_terrainWidth) ])  * (1.0f-posD.x) *       posD.y
         + (_groundVB->getNormal()[ COORD(posI.x+1,posI.y+1,_terrainWidth) ])  *       posD.x  *       posD.y;
}

vec3<F32> Terrain::getTangent(F32 x_clampf, F32 z_clampf) const{
    if(x_clampf<.0f || z_clampf<.0f || x_clampf>1.0f || z_clampf>1.0f) return vec3<F32>(1.0f, 0.0f, 0.0f);

    vec2<F32> posF(	x_clampf * _terrainWidth, z_clampf * _terrainHeight );
    vec2<U32> posI(	(I32)(x_clampf * _terrainWidth), (I32)(z_clampf * _terrainHeight) );
    vec2<F32> posD(	posF.x - posI.x, posF.y - posI.y );

    if(posI.x >= (U32)_terrainWidth-1)		posI.x = _terrainWidth-2;
    if(posI.y >= (U32)_terrainHeight-1)	posI.y = _terrainHeight-2;
    assert(posI.x>=0 && posI.x<(U32)_terrainWidth-1 && posI.y>=0 && posI.y<(U32)_terrainHeight-1);

    return    (_groundVB->getTangent()[ COORD(posI.x,  posI.y,  _terrainWidth) ])  * (1.0f-posD.x) * (1.0f-posD.y)
            + (_groundVB->getTangent()[ COORD(posI.x+1,posI.y,  _terrainWidth) ])  *       posD.x  * (1.0f-posD.y)
            + (_groundVB->getTangent()[ COORD(posI.x,  posI.y+1,_terrainWidth) ])  * (1.0f-posD.x) *       posD.y
            + (_groundVB->getTangent()[ COORD(posI.x+1,posI.y+1,_terrainWidth) ])  *       posD.x  *       posD.y;
}