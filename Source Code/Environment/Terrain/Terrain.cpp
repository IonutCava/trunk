#include "Headers/Terrain.h"
#include "Headers/TerrainChunk.h"

#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/CameraManager.h"
#include "Quadtree/Headers/Quadtree.h"
#include "Quadtree/Headers/QuadtreeNode.h"
#include "Hardware/Video/VertexBufferObject.h"
#include "Environment/Sky/Headers/Sky.h"
#include "Hardware/Video/GFXDevice.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/RenderStateBlock.h"

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
	_veg(NULL),
	_planeTransform(NULL),
	_node(NULL),
	_planeSGN(NULL),
	_terrainRenderState(NULL)
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


void Terrain::prepareMaterial(SceneGraphNode const* const sgn){

	ShaderProgram* terrainShader = getMaterial()->getShaderProgram();
	SET_STATE_BLOCK(_terrainRenderState);
	GFX_DEVICE.setMaterial(getMaterial());

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

	if(_alphaTexturePresent){
		_terrainTextures[TERRAIN_TEXTURE_ALPHA]->Bind(6); //AlphaMap: Alpha
		terrainShader->UniformTexture("texDiffuse3",6);
	}
}

void Terrain::releaseMaterial(){

	if(_alphaTexturePresent) _terrainTextures[TERRAIN_TEXTURE_ALPHA]->Unbind(6);
	_terrainTextures[TERRAIN_TEXTURE_BLUE]->Unbind(5);
	_terrainTextures[TERRAIN_TEXTURE_GREEN]->Unbind(4);
	_terrainTextures[TERRAIN_TEXTURE_RED]->Unbind(3);
	_terrainTextures[TERRAIN_TEXTURE_CAUSTICS]->Unbind(2);
	_terrainTextures[TERRAIN_TEXTURE_NORMALMAP]->Unbind(1);
	_terrainTextures[TERRAIN_TEXTURE_DIFFUSE]->Unbind(0);
}

void Terrain::render(SceneGraphNode* const sgn){

	drawGround();
	drawInfinitePlain();
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

	const vec3<F32>& eyePos = CameraManager::getInstance().getActiveCamera()->getEye();
	_planeTransform->setPositionX(eyePos.x);
	_planeTransform->setPositionZ(eyePos.z);
	_planeSGN->getBoundingBox().Transform(_planeSGN->getInitialBoundingBox(),_planeTransform->getMatrix());

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

	return    (_groundVBO->getNormal()[ COORD(posI.x,  posI.y,  _terrainWidth) ])  * (1.0f-posD.x) * (1.0f-posD.y)
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