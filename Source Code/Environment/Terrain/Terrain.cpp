#include "Headers/Terrain.h"
#include "Headers/TerrainChunk.h"

#include "Graphs/Headers/SceneGraphNode.h"
#include "Managers/Headers/SceneManager.h"
#include "Quadtree/Headers/Quadtree.h"
#include "Quadtree/Headers/QuadtreeNode.h"
#include "Hardware/Video/Buffers/VertexBufferObject/Headers/VertexBufferObject.h"
#include "Environment/Sky/Headers/Sky.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"

#define COORD(x,y,w)	((y)*(w)+(x))

Terrain::Terrain() : SceneNode(TYPE_TERRAIN), 
	_alphaTexturePresent(false),
	_terrainWidth(0),
	_terrainHeight(0),
	_groundVBO(NULL),
	_terrainQuadtree(New Quadtree()),
	_plane(NULL),
	_drawInReflection(false),
	_drawBBoxes(false),
	_shadowMapped(true),
	_veg(NULL),
	_planeTransform(NULL),
	_node(NULL),
	_planeSGN(NULL),
	_terrainRenderState(NULL),
	_stateRefreshIntervalBuffer(0),
	_stateRefreshInterval(500) ///<Every half a second
{
	
	_terrainTextures[TERRAIN_TEXTURE_DIFFUSE]   = NULL;
	_terrainTextures[TERRAIN_TEXTURE_NORMALMAP] = NULL;
	_terrainTextures[TERRAIN_TEXTURE_CAUSTICS]  = NULL;
	_terrainTextures[TERRAIN_TEXTURE_RED]       = NULL;
	_terrainTextures[TERRAIN_TEXTURE_GREEN]     = NULL;
	_terrainTextures[TERRAIN_TEXTURE_BLUE]      = NULL;
}

Terrain::~Terrain() 
{
}

void Terrain::sceneUpdate(D32 sceneTime){
	///Query shadow state every "_stateRefreshInterval" milliseconds

	if (sceneTime - _stateRefreshIntervalBuffer >= _stateRefreshInterval){
		_shadowMapped = ParamHandler::getInstance().getParam<bool>("rendering.enableShadows");

		_stateRefreshIntervalBuffer += _stateRefreshInterval;
	}
	_veg->sceneUpdate(sceneTime);
}

void Terrain::prepareMaterial(SceneGraphNode* const sgn){
	///ToDo: Add multiple local lights for terrain, such as torches, rockets, flashlights etc - Ionut 
	///Prepare the main light (directional light only, sun) for now.
	if(GFX_DEVICE.getRenderStage() != SHADOW_STAGE){
		///Find the most influental light for the terrain. Usually the Sun
		_lightCount = LightManager::getInstance().findLightsForSceneNode(sgn,LIGHT_TYPE_DIRECTIONAL);
		///Update lights for this node
		LightManager::getInstance().update();
		///Only 1 shadow map for terrains
	}
	CLAMP<U8>(_lightCount, 0, 1);
	if(GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE)){
		U8 offset = 9;
		for(U8 n = 0; n < _lightCount; n++, offset++){
			Light* l = LightManager::getInstance().getLightForCurrentNode(n);
			LightManager::getInstance().bindDepthMaps(l, n, offset);
		}
	}
	
	ShaderProgram* terrainShader = getMaterial()->getShaderProgram();
	SET_STATE_BLOCK(_terrainRenderState);
	GFX_DEVICE.setMaterial(getMaterial());
	vectorImpl<I32>& types = LightManager::getInstance().getLightTypesForCurrentNode();
	_terrainTextures[TERRAIN_TEXTURE_DIFFUSE]->Bind(0);
	_terrainTextures[TERRAIN_TEXTURE_NORMALMAP]->Bind(1);
	_terrainTextures[TERRAIN_TEXTURE_CAUSTICS]->Bind(2); //Water Caustics
	_terrainTextures[TERRAIN_TEXTURE_RED]->Bind(3);      //AlphaMap: RED
	_terrainTextures[TERRAIN_TEXTURE_GREEN]->Bind(4);    //AlphaMap: GREEN
	_terrainTextures[TERRAIN_TEXTURE_BLUE]->Bind(5);     //AlphaMap: BLUE

	terrainShader->bind();

	terrainShader->Uniform("water_reflection_rendering", _drawInReflection);
	terrainShader->UniformTexture("texDiffuseMap", 0);
	terrainShader->UniformTexture("texNormalHeightMap", 1);
	terrainShader->UniformTexture("texWaterCaustics", 2);
	terrainShader->UniformTexture("texDiffuse0", 3);
	terrainShader->UniformTexture("texDiffuse1", 4);
	terrainShader->UniformTexture("texDiffuse2", 5);
	terrainShader->Uniform("light_count", 1);

	if(_alphaTexturePresent){
		_terrainTextures[TERRAIN_TEXTURE_ALPHA]->Bind(6); //AlphaMap: Alpha
		terrainShader->UniformTexture("texDiffuse3",6);
	}
	terrainShader->Uniform("enable_shadow_mapping", _shadowMapped);
	terrainShader->Uniform("lightProjectionMatrices",LightManager::getInstance().getLightProjectionMatricesCache());
	terrainShader->Uniform("lightType",types);
}

void Terrain::releaseMaterial(){

	if(_alphaTexturePresent) _terrainTextures[TERRAIN_TEXTURE_ALPHA]->Unbind(6);
	_terrainTextures[TERRAIN_TEXTURE_BLUE]->Unbind(5);
	_terrainTextures[TERRAIN_TEXTURE_GREEN]->Unbind(4);
	_terrainTextures[TERRAIN_TEXTURE_RED]->Unbind(3);
	_terrainTextures[TERRAIN_TEXTURE_CAUSTICS]->Unbind(2);
	_terrainTextures[TERRAIN_TEXTURE_NORMALMAP]->Unbind(1);
	_terrainTextures[TERRAIN_TEXTURE_DIFFUSE]->Unbind(0);

	if(GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE)){
		U8 offset = (_lightCount - 1) + 9;
		for(I32 n = _lightCount - 1; n >= 0; n--,offset--){
			Light* l = LightManager::getInstance().getLightForCurrentNode(n);
			LightManager::getInstance().unbindDepthMaps(l, offset);
		}
	}
}

void Terrain::render(SceneGraphNode* const sgn){
	drawInfinitePlain();
	drawGround();
}

void Terrain::drawBoundingBox(SceneGraphNode* const sgn){
	if(_drawBBoxes){
		_terrainQuadtree->DrawBBox();
	}
}

void Terrain::postDraw(){
	_veg->draw(_drawInReflection);
}

void Terrain::drawInfinitePlain(){
	if(_drawInReflection) return;

	_planeTransform->setPositionX(_eyePos.x);
	_planeTransform->setPositionZ(_eyePos.z);
	_planeSGN->getBoundingBox().Transform(_planeSGN->getInitialBoundingBox(),
										  _planeTransform->getMatrix());

	GFX_DEVICE.setObjectState(_planeTransform);
	GFX_DEVICE.renderModel(_plane);
	GFX_DEVICE.releaseObjectState(_planeTransform);

}

void Terrain::drawGround() const{
	assert(_groundVBO);
	_groundVBO->Enable();
		_terrainQuadtree->DrawGround(_drawInReflection);
	_groundVBO->Disable();
}


vec3<F32> Terrain::getPosition(F32 x_clampf, F32 z_clampf) const{
	if(x_clampf<.0f || z_clampf<.0f || x_clampf>1.0f || z_clampf>1.0f) return vec3<F32>(0.0f, 0.0f, 0.0f);

	vec2<F32>  posF(	x_clampf * _terrainWidth, z_clampf * _terrainHeight );
	vec2<U32>  posI(	(I32)(posF.x), (I32)(posF.y) );
	vec2<F32>  posD(	posF.x - posI.x, posF.y - posI.y );

	if(posI.x >= (U32)_terrainWidth-1)	posI.x = _terrainWidth-2;
	if(posI.y >= (U32)_terrainHeight-1)	posI.y = _terrainHeight-2;
	assert(posI.x>=0 && posI.x<(U32)_terrainWidth-1 && posI.y>=0 && posI.y<(U32)_terrainHeight-1);

	vec3<F32> pos(_boundingBox.getMin().x + x_clampf * (_boundingBox.getMax().x - _boundingBox.getMin().x), 0.0f, _boundingBox.getMin().z + z_clampf * (_boundingBox.getMax().z - _boundingBox.getMin().z));
	pos.y =   (_groundVBO->getPosition()[ COORD(posI.x,  posI.y,  _terrainWidth) ].y)  * (1.0f-posD.x) * (1.0f-posD.y)
			+ (_groundVBO->getPosition()[ COORD(posI.x+1,posI.y,  _terrainWidth) ].y)  *       posD.x  * (1.0f-posD.y)
			+ (_groundVBO->getPosition()[ COORD(posI.x,  posI.y+1,_terrainWidth) ].y)  * (1.0f-posD.x) *       posD.y
			+ (_groundVBO->getPosition()[ COORD(posI.x+1,posI.y+1,_terrainWidth) ].y)  *       posD.x  *       posD.y;
	return pos;
}

vec3<F32> Terrain::getNormal(F32 x_clampf, F32 z_clampf) const{
	if(x_clampf<.0f || z_clampf<.0f || x_clampf>1.0f || z_clampf>1.0f) return vec3<F32>(0.0f, 1.0f, 0.0f);

	vec2<F32>  posF(	x_clampf * _terrainWidth, z_clampf * _terrainHeight );
	vec2<U32>  posI(	(I32)(x_clampf * _terrainWidth), (I32)(z_clampf * _terrainHeight) );
	vec2<F32>  posD(	posF.x - posI.x, posF.y - posI.y );

	if(posI.x >= (U32)_terrainWidth-1)		posI.x = _terrainWidth-2;
	if(posI.y >= (U32)_terrainHeight-1)	posI.y = _terrainHeight-2;
	assert(posI.x>=0 && posI.x<(U32)_terrainWidth-1 && posI.y>=0 && posI.y<(U32)_terrainHeight-1);

    return (_groundVBO->getNormal()[ COORD(posI.x,  posI.y,  _terrainWidth) ])  * (1.0f-posD.x) * (1.0f-posD.y)
		 + (_groundVBO->getNormal()[ COORD(posI.x+1,posI.y,  _terrainWidth) ])  *       posD.x  * (1.0f-posD.y)
		 + (_groundVBO->getNormal()[ COORD(posI.x,  posI.y+1,_terrainWidth) ])  * (1.0f-posD.x) *       posD.y
		 + (_groundVBO->getNormal()[ COORD(posI.x+1,posI.y+1,_terrainWidth) ])  *       posD.x  *       posD.y;
}

vec3<F32> Terrain::getTangent(F32 x_clampf, F32 z_clampf) const{
	if(x_clampf<.0f || z_clampf<.0f || x_clampf>1.0f || z_clampf>1.0f) return vec3<F32>(1.0f, 0.0f, 0.0f);

	vec2<F32> posF(	x_clampf * _terrainWidth, z_clampf * _terrainHeight );
	vec2<U32> posI(	(I32)(x_clampf * _terrainWidth), (I32)(z_clampf * _terrainHeight) );
	vec2<F32> posD(	posF.x - posI.x, posF.y - posI.y );

	if(posI.x >= (U32)_terrainWidth-1)		posI.x = _terrainWidth-2;
	if(posI.y >= (U32)_terrainHeight-1)	posI.y = _terrainHeight-2;
	assert(posI.x>=0 && posI.x<(U32)_terrainWidth-1 && posI.y>=0 && posI.y<(U32)_terrainHeight-1);

	return    (_groundVBO->getTangent()[ COORD(posI.x,  posI.y,  _terrainWidth) ])  * (1.0f-posD.x) * (1.0f-posD.y)
			+ (_groundVBO->getTangent()[ COORD(posI.x+1,posI.y,  _terrainWidth) ])  *       posD.x  * (1.0f-posD.y)
			+ (_groundVBO->getTangent()[ COORD(posI.x,  posI.y+1,_terrainWidth) ])  * (1.0f-posD.x) *       posD.y
			+ (_groundVBO->getTangent()[ COORD(posI.x+1,posI.y+1,_terrainWidth) ])  *       posD.x  *       posD.y;
}