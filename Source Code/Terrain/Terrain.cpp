#include "Terrain.h"
#include "Managers/TextureManager.h"
#include "Managers/ResourceManager.h"
#include "Managers/SceneManager.h"
#include "Utility/Headers/ParamHandler.h"
#include "Quadtree.h"
#include "QuadtreeNode.h"
#include "TerrainChunk.h"
#include "TerrainDescriptor.h"
#include "Hardware/Video/VertexBufferObject.h"
#include "Managers/CameraManager.h"
#include "Sky.h"
#include "Hardware/Video/GFXDevice.h"

#define COORD(x,y,w)	((y)*(w)+(x))

Terrain::Terrain() : SceneNode(){
	_alphaTexturePresent = false;
	_terrainWidth = 0;
	_terrainHeight = 0;
	_groundVBO = NULL;
	_terrainShader = ResourceManager::getInstance().LoadResource<Shader>(ResourceDescriptor("terrain_ground"));
	_terrainQuadtree = New Quadtree();
	_depthMapFBO[0] = GFXDevice::getInstance().newFBO();
	_depthMapFBO[1] = GFXDevice::getInstance().newFBO();
	_terrainMaterial = NULL;
	_loaded = false;
}


bool Terrain::unload(){
	_terrainWidth = 0;
	_terrainHeight = 0;

	if(_terrainQuadtree != NULL) {
		delete _terrainQuadtree;
		_terrainQuadtree = NULL;
	}

	if(_groundVBO != NULL) {
		delete _groundVBO;
		_groundVBO = NULL;		
	}
	for(U8 i = 0; i < _terrainTextures.size(); i++){
		ResourceManager::getInstance().remove(_terrainTextures[i]);
	}
	_terrainTextures.clear();
	ResourceManager::getInstance().remove(_terrainDiffuseMap);
	ResourceManager::getInstance().remove(_terrainShader);
	ResourceManager::getInstance().remove(_terrainMaterial);
	if(_veg != NULL){
		delete _veg;
		_veg = NULL;
	}
	if(!_groundVBO)
		return SceneNode::unload(); 
	else return false;
}

void Terrain::terrainSetParameters(const vec3& pos,const vec2& scale){
	_terrainHeightScaleFactor = scale.y;
	terrainScaleFactor = scale.x;
	//Terrain dimensions:
	//    |----------------------|        /\						      /\
	//    |          /\          |         |					         /  \
 	//    |          |           |         |					    /\__/    \
	//    |          |           | _terrainHeightScaleFactor  /\   /          \__/\___
	//    |<-terrainScaleFactor->|         |				 |  --/                   \
	//    |          |           |         |                /                          \
	//    |          |           |         |              |-                            \
	//    |          |           |         \/            /_______________________________\
	//    |_________\/___________|

	_boundingBox.setMin(vec3(-pos.x-300, pos.y, -pos.z-300));
	_boundingBox.setMax(vec3( pos.x+300, pos.y+40, pos.z+300));

	_boundingBox.Multiply(vec3(terrainScaleFactor,1,terrainScaleFactor));
	_boundingBox.MultiplyMax(vec3(1,_terrainHeightScaleFactor,1));
}

bool Terrain::load(const string& name){
	std::vector<TerrainDescriptor*>& terrains = SceneManager::getInstance().getActiveScene()->getTerrainInfoArray();
	Console::getInstance().printfn("Loading terrain: %s",name.c_str());
	TerrainDescriptor* terrain = NULL;
	for(U8 i = 0; i < terrains.size(); i++)
		if(name.compare(terrains[i]->getVariable("terrainName")) == 0){
			terrain = terrains[i];
			break;
		}
	if(!terrain) return false;
	terrainSetParameters(terrain->getPosition(),terrain->getScale());
	U32 chunkSize = 16;
	ResourceDescriptor textureNormalMap("Terrain Normal Map");
	textureNormalMap.setResourceLocation(terrain->getVariable("normalMap"));
	//textureNormalMap.setFlag(true);
	ResourceDescriptor textureRedTexture("Terrain Red Texture");
	textureRedTexture.setResourceLocation(terrain->getVariable("redTexture"));
	//textureRedTexture.setFlag(true);
	ResourceDescriptor textureGreenTexture("Terrain Green Texture");
	textureGreenTexture.setResourceLocation(terrain->getVariable("greenTexture"));
	//textureGreenTexture.setFlag(true);
	ResourceDescriptor textureBlueTexture("Terrain Blue Texture");
	textureBlueTexture.setResourceLocation(terrain->getVariable("blueTexture"));
	//textureBlueTexture.setFlag(true);
	//alphaTexture is a special case. See bellow -Ionut
	string alphaTextureFile = terrain->getVariable("alphaTexture");
	ResourceDescriptor textureWaterCaustics("Terrain Water Caustics");
	textureWaterCaustics.setResourceLocation(terrain->getVariable("waterCaustics"));
	//textureWaterCaustics.setFlag(true);
	ResourceDescriptor textureTextureMap("Terrain Texture Map");
	textureTextureMap.setResourceLocation(terrain->getVariable("textureMap"));
	//textureTextureMap.setFlag(true);

	_terrainDiffuseMap = ResourceManager::getInstance().LoadResource<Texture>(textureTextureMap);
	_terrainTextures.push_back(ResourceManager::getInstance().LoadResource<Texture>(textureNormalMap));
	_terrainTextures.push_back(ResourceManager::getInstance().LoadResource<Texture>(textureWaterCaustics));
	_terrainTextures.push_back(ResourceManager::getInstance().LoadResource<Texture>(textureRedTexture));
	_terrainTextures.push_back(ResourceManager::getInstance().LoadResource<Texture>(textureGreenTexture));
	_terrainTextures.push_back(ResourceManager::getInstance().LoadResource<Texture>(textureBlueTexture));
	if(alphaTextureFile.compare(ParamHandler::getInstance().getParam<string>("assetsLocation") + "/none") != 0){
		_alphaTexturePresent = true;
		ResourceDescriptor textureAlphaTexture("Terrain Alpha Texture");
		textureAlphaTexture.setResourceLocation(alphaTextureFile);
		//textureAlphaTexture.setFlag(true);
		_terrainTextures.push_back(ResourceManager::getInstance().LoadResource<Texture>(textureAlphaTexture));
	}
	
	_groundVBO = GFXDevice::getInstance().newVBO();
	std::vector<vec3>&		tPosition	= _groundVBO->getPosition();
	std::vector<vec3>&		tNormal		= _groundVBO->getNormal();
	std::vector<vec3>&		tTangent	= _groundVBO->getTangent();

	U8 d;
	U32 t;
	U8* data = ImageTools::OpenImage(terrain->getVariable("heightmap"), _terrainWidth, _terrainHeight, d, t);
	Console::getInstance().printfn("Terrain width: %d and height: %d",_terrainWidth, _terrainHeight);
	assert(data!=NULL);

	U32 nIMGWidth  = _terrainWidth;
	U32 nIMGHeight = _terrainHeight;

	if(_terrainWidth%2==0)	_terrainWidth++;
	if(_terrainHeight%2==0)	_terrainHeight++;

	tPosition.resize(_terrainWidth*_terrainHeight);
	tNormal.resize(_terrainWidth*_terrainHeight);
	tTangent.resize(_terrainWidth*_terrainHeight);

	for(U32 j=0; j < _terrainHeight; j++) {
		for(U32 i=0; i < _terrainWidth; i++) {

			U32 idxHM = COORD(i,j,_terrainWidth);
			tPosition[idxHM].x = _boundingBox.getMin().x + ((F32)i) * 
				                (_boundingBox.getMax().x - _boundingBox.getMin().x)/(_terrainWidth-1);
			tPosition[idxHM].z = _boundingBox.getMin().z + ((F32)j) * 
				                (_boundingBox.getMax().z - _boundingBox.getMin().z)/(_terrainHeight-1);
			U32 idxIMG = COORD(	i<nIMGWidth? i : i-1,
								j<nIMGHeight? j : j-1,
								nIMGWidth);

			F32 h = (F32)(data[idxIMG*d + 0] + data[idxIMG*d + 1] + data[idxIMG*d + 2])/3;

			tPosition[idxHM].y = (_boundingBox.getMin().y + ((F32)h) * 
				                 (_boundingBox.getMax().y - _boundingBox.getMin().y)/(255)) * _terrainHeightScaleFactor;

		}
	}
	delete[] data;
	data = NULL;
	U32 offset = 2;

	for(U32 j=offset; j < _terrainHeight-offset; j++) {
		for(U32 i=offset; i < _terrainWidth-offset; i++) {

			U32 idx = COORD(i,j,_terrainWidth);

			vec3 vU = tPosition[COORD(i+offset, j+0, _terrainWidth)] - tPosition[COORD(i-offset, j+0, _terrainWidth)];
			vec3 vV = tPosition[COORD(i+0, j+offset, _terrainWidth)] - tPosition[COORD(i+0, j-offset, _terrainWidth)];

			tNormal[idx].cross(vV, vU);
			tNormal[idx].normalize();
			tTangent[idx] = -vU;
			tTangent[idx].normalize();
		}
	}

	for(U32 j=0; j < offset; j++) {
		for(U32 i=0; i < _terrainWidth; i++) {
			U32 idx0 = COORD(i,	j,		_terrainWidth);
			U32 idx1 = COORD(i,	offset,	_terrainWidth);

			tNormal[idx0] = tNormal[idx1];
			tTangent[idx0] = tTangent[idx1];

			idx0 = COORD(i,	_terrainHeight-1-j,		_terrainWidth);
			idx1 = COORD(i,	_terrainHeight-1-offset,	_terrainWidth);

			tNormal[idx0] = tNormal[idx1];
			tTangent[idx0] = tTangent[idx1];
		}

	}

	for(U32 i=0; i < offset; i++) {
		for(U32 j=0; j < _terrainHeight; j++) {
			U32 idx0 = COORD(i,		    j,	_terrainWidth);
			U32 idx1 = COORD(offset,	j,	_terrainWidth);

			tNormal[idx0] = tNormal[idx1];
			tTangent[idx0] = tTangent[idx1];

			idx0 = COORD(_terrainWidth-1-i,		j,	_terrainWidth);
			idx1 = COORD(_terrainWidth-1-offset,	j,	_terrainWidth);

			tNormal[idx0] = tNormal[idx1];
			tTangent[idx0] = tTangent[idx1];
		}
	}

	_terrainQuadtree->Build(&_boundingBox, ivec2(_terrainWidth, _terrainHeight), chunkSize);
	_loaded = _groundVBO->Create();

	
	if(_loaded){
		Console::getInstance().printfn("Loading Terrain OK");
		setActive(terrain->getActive());
		computeBoundingBox();
		ResourceDescriptor terrainMaterial("terrainMaterial");
		_terrainMaterial = ResourceManager::getInstance().LoadResource<Material>(terrainMaterial);
		_terrainMaterial->setDiffuse(vec4(1.0f, 1.0f, 1.0f, 1.0f));
		_terrainMaterial->setSpecular(vec4(1.0f, 1.0f, 1.0f, 1.0f));
		_terrainMaterial->setShininess(50.0f);

		_terrainShader->bind();
			_terrainShader->Uniform("alphaTexture", _alphaTexturePresent);
			_terrainShader->Uniform("detail_scale", 200.0f);
			_terrainShader->Uniform("diffuse_scale", 200.0f);
			_terrainShader->Uniform("parallax_factor",0.2f);
			_terrainShader->Uniform("bbox_min", _boundingBox.getMin());
			_terrainShader->Uniform("bbox_max", _boundingBox.getMax());
			_terrainShader->Uniform("fog_color", vec3(0.7f, 0.7f, 0.9f));
			_terrainShader->Uniform("depth_map_size", 512);
		_terrainShader->unbind();
	}else{
		Console::getInstance().errorfn("Loading Terrain NOT OK");
		setActive(false);
	}
	return _loaded;
}

bool Terrain::computeBoundingBox(){
	_terrainQuadtree->computeBoundingBox(_groundVBO->getPosition());
	return true;
}

void Terrain::initializeVegetation(TerrainDescriptor* terrain) {	
	vector<Texture2D*> grass;
	ResourceDescriptor grassBillboard1("Grass Billboard 1");
	grassBillboard1.setResourceLocation(terrain->getVariable("grassBillboard1"));
	ResourceDescriptor grassBillboard2("Grass Billboard 2");
	grassBillboard2.setResourceLocation(terrain->getVariable("grassBillboard2"));
	ResourceDescriptor grassBillboard3("Grass Billboard 3");
	grassBillboard3.setResourceLocation(terrain->getVariable("grassBillboard3"));
	//ResourceDescriptor grassBillboard4("Grass Billboard 1");
	//grassBillboard4.setResourceLocation(terrain->getVariable("grassBillboard4"));
	grass.push_back(ResourceManager::getInstance().LoadResource<Texture2D>(grassBillboard1));
	grass.push_back(ResourceManager::getInstance().LoadResource<Texture2D>(grassBillboard2));
	grass.push_back(ResourceManager::getInstance().LoadResource<Texture2D>(grassBillboard3));
	//grass.push_back(ResourceManager::getInstance().LoadResource<Texture2D>(grassBillboard4));
	addVegetation(New Vegetation(3, terrain->getGrassDensity(),
			                        terrain->getGrassScale(),
								    terrain->getTreeDensity(),
									terrain->getTreeScale(),
									terrain->getVariable("grassMap"),
									grass),
				  "terrain_grass");
	toggleVegetation(true);
	 _veg->initialize(_grassShader,_name);
}

vec3 Terrain::getPosition(F32 x_clampf, F32 z_clampf) const{
	if(x_clampf<.0f || z_clampf<.0f || x_clampf>1.0f || z_clampf>1.0f) return vec3(0.0f, 0.0f, 0.0f);

	vec2  posF(	x_clampf * _terrainWidth, z_clampf * _terrainHeight );
	ivec2 posI(	(I32)(posF.x), (I32)(posF.y) );
	vec2  posD(	posF.x - posI.x, posF.y - posI.y );

	if(posI.x >= (I32)_terrainWidth-1)		posI.x = _terrainWidth-2;
	if(posI.y >= (I32)_terrainHeight-1)	posI.y = _terrainHeight-2;
	assert(posI.x>=0 && posI.x<(I32)_terrainWidth-1 && posI.y>=0 && posI.y<(I32)_terrainHeight-1);

	vec3 pos(_boundingBox.getMin().x + x_clampf * (_boundingBox.getMax().x - _boundingBox.getMin().x), 0.0f, _boundingBox.getMin().z + z_clampf * (_boundingBox.getMax().z - _boundingBox.getMin().z));
	pos.y =   (_groundVBO->getPosition()[ COORD(posI.x,  posI.y,  _terrainWidth) ].y)  * (1.0f-posD.x) * (1.0f-posD.y)
			+ (_groundVBO->getPosition()[ COORD(posI.x+1,posI.y,  _terrainWidth) ].y)  *       posD.x  * (1.0f-posD.y)
			+ (_groundVBO->getPosition()[ COORD(posI.x,  posI.y+1,_terrainWidth) ].y)  * (1.0f-posD.x) *       posD.y
			+ (_groundVBO->getPosition()[ COORD(posI.x+1,posI.y+1,_terrainWidth) ].y)  *       posD.x  *       posD.y;
	return pos;
}

vec3 Terrain::getNormal(F32 x_clampf, F32 z_clampf) const{
	if(x_clampf<.0f || z_clampf<.0f || x_clampf>1.0f || z_clampf>1.0f) return vec3(0.0f, 1.0f, 0.0f);

	vec2  posF(	x_clampf * _terrainWidth, z_clampf * _terrainHeight );
	ivec2 posI(	(I32)(x_clampf * _terrainWidth), (I32)(z_clampf * _terrainHeight) );
	vec2  posD(	posF.x - posI.x, posF.y - posI.y );

	if(posI.x >= (I32)_terrainWidth-1)		posI.x = _terrainWidth-2;
	if(posI.y >= (I32)_terrainHeight-1)	posI.y = _terrainHeight-2;
	assert(posI.x>=0 && posI.x<(I32)_terrainWidth-1 && posI.y>=0 && posI.y<(I32)_terrainHeight-1);

	return    (_groundVBO->getNormal()[ COORD(posI.x,  posI.y,  _terrainWidth) ])  * (1.0f-posD.x) * (1.0f-posD.y)
			+ (_groundVBO->getNormal()[ COORD(posI.x+1,posI.y,  _terrainWidth) ])  *       posD.x  * (1.0f-posD.y)
			+ (_groundVBO->getNormal()[ COORD(posI.x,  posI.y+1,_terrainWidth) ])  * (1.0f-posD.x) *       posD.y
			+ (_groundVBO->getNormal()[ COORD(posI.x+1,posI.y+1,_terrainWidth) ])  *       posD.x  *       posD.y;
}

vec3 Terrain::getTangent(F32 x_clampf, F32 z_clampf) const{
	if(x_clampf<.0f || z_clampf<.0f || x_clampf>1.0f || z_clampf>1.0f) return vec3(1.0f, 0.0f, 0.0f);

	vec2  posF(	x_clampf * _terrainWidth, z_clampf * _terrainHeight );
	ivec2 posI(	(I32)(x_clampf * _terrainWidth), (I32)(z_clampf * _terrainHeight) );
	vec2  posD(	posF.x - posI.x, posF.y - posI.y );

	if(posI.x >= (I32)_terrainWidth-1)		posI.x = _terrainWidth-2;
	if(posI.y >= (I32)_terrainHeight-1)	posI.y = _terrainHeight-2;
	assert(posI.x>=0 && posI.x<(I32)_terrainWidth-1 && posI.y>=0 && posI.y<(I32)_terrainHeight-1);

	return    (_groundVBO->getTangent()[ COORD(posI.x,  posI.y,  _terrainWidth) ])  * (1.0f-posD.x) * (1.0f-posD.y)
			+ (_groundVBO->getTangent()[ COORD(posI.x+1,posI.y,  _terrainWidth) ])  *       posD.x  * (1.0f-posD.y)
			+ (_groundVBO->getTangent()[ COORD(posI.x,  posI.y+1,_terrainWidth) ])  * (1.0f-posD.x) *       posD.y
			+ (_groundVBO->getTangent()[ COORD(posI.x+1,posI.y+1,_terrainWidth) ])  *       posD.x  *       posD.y;
}



void Terrain::render(){
	if(!_active) return;

	_veg->draw(_drawInReflection);

	GFXDevice::getInstance().setMaterial(_terrainMaterial);

	RenderState old = GFXDevice::getInstance().getActiveRenderState();
	RenderState s(true,true,true,true);
	GFXDevice::getInstance().setRenderState(s);

	GFXDevice::getInstance().setTextureMatrix(0,_sunModelviewProj); //Add sun projection matrix to the normal map for parallax rendering

	_terrainDiffuseMap->Bind(0);
	_depthMapFBO[0]->Bind(1);
	_depthMapFBO[1]->Bind(2);
	_terrainTextures[0]->Bind(3); //Normal Map
	_terrainTextures[1]->Bind(4); //Water Caustics
	_terrainTextures[2]->Bind(5); //AlphaMap: RED
	_terrainTextures[3]->Bind(6); //AlphaMap: GREEN
	_terrainTextures[4]->Bind(7); //AlphaMap: BLUE
	if(_alphaTexturePresent) _terrainTextures[5]->Bind(8); //AlphaMap: Alpha

	_terrainShader->bind();	

		_terrainShader->Uniform("water_height", SceneManager::getInstance().getActiveScene()->getWaterLevel());
		_terrainShader->Uniform("water_reflection_rendering", _drawInReflection);
		_terrainShader->Uniform("time", GETTIME());

		_terrainShader->UniformTexture("texDiffuseMap", 0);
		_terrainShader->UniformTexture("texDepthMapFromLight0", 1);
		_terrainShader->UniformTexture("texDepthMapFromLight1", 2);
		_terrainShader->UniformTexture("texNormalHeightMap", 3);
		_terrainShader->UniformTexture("texWaterCaustics", 4);
		_terrainShader->UniformTexture("texDiffuse0", 5);
		_terrainShader->UniformTexture("texDiffuse1", 6);
		_terrainShader->UniformTexture("texDiffuse2", 7);
		if(_alphaTexturePresent) _terrainShader->UniformTexture("texDiffuse3",8);
		
			drawGround(_drawInReflection);
			
	_terrainShader->unbind();

	if(_alphaTexturePresent) _terrainTextures[5]->Unbind(8);
	_terrainTextures[4]->Unbind(7);
	_terrainTextures[3]->Unbind(6);
	_terrainTextures[2]->Unbind(5);
	_terrainTextures[1]->Unbind(4);
	_terrainTextures[0]->Unbind(3);
	_depthMapFBO[1]->Unbind(2);
	_depthMapFBO[0]->Unbind(1);
	_terrainDiffuseMap->Unbind(0);

	GFXDevice::getInstance().restoreTextureMatrix(0);
	
	GFXDevice::getInstance().setRenderState(old);
}



void Terrain::drawGround(bool drawInReflection) const{
	assert(_groundVBO);

	_groundVBO->Enable();
		_terrainQuadtree->DrawGround(drawInReflection);
	_groundVBO->Disable();
}

