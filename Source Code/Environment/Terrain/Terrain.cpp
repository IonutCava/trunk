#include "Headers/Terrain.h"
#include "Headers/TerrainChunk.h"
#include "Headers/TerrainDescriptor.h"

#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/CameraManager.h"
#include "Quadtree/Headers/Quadtree.h"
#include "Quadtree/Headers/QuadtreeNode.h"
#include "Hardware/Video/VertexBufferObject.h"
#include "Environment/Sky/Headers/Sky.h"
#include "Hardware/Video/GFXDevice.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Graphs/Headers/RenderQueue.h"
#include "Core/Headers/ParamHandler.h"

#define COORD(x,y,w)	((y)*(w)+(x))

Terrain::Terrain() : SceneNode(), 
	_alphaTexturePresent(false),
	_terrainWidth(0),
	_terrainHeight(0),
	_groundVBO(NULL),
	_terrainQuadtree(New Quadtree()),
	_loaded(false),
	_plane(NULL),
	_drawInReflection(false),
	_terrainDiffuseMap(NULL),
	_veg(NULL),
	_planeTransform(NULL),
	_node(NULL),
	_planeSGN(NULL)
{}


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
		RemoveResource(_terrainTextures[i]);
	}
	_terrainTextures.clear();
	RemoveResource(_terrainDiffuseMap);

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
	///Terrain dimensions:
	///    |----------------------|        /\						      /\
	///    |          /\          |         |					         /  \
 	///    |          |           |         |					    /\__/    \
	///    |          |           | _terrainHeightScaleFactor  /\   /          \__/\___
	///    |<-terrainScaleFactor->|         |				 |  --/                   \
	///    |          |           |         |                /                          \
	///    |          |           |         |              |-                            \
	///    |          |           |         \/            /_______________________________\
	///    |_________\/___________|

	_boundingBox.set(vec3(-pos.x-300, pos.y, -pos.z-300),vec3( pos.x+300, pos.y+40, pos.z+300));
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
	ResourceDescriptor textureRedTexture("Terrain Red Texture");
	textureRedTexture.setResourceLocation(terrain->getVariable("redTexture"));
	ResourceDescriptor textureGreenTexture("Terrain Green Texture");
	textureGreenTexture.setResourceLocation(terrain->getVariable("greenTexture"));
	ResourceDescriptor textureBlueTexture("Terrain Blue Texture");
	textureBlueTexture.setResourceLocation(terrain->getVariable("blueTexture"));
	string alphaTextureFile = terrain->getVariable("alphaTexture");
	ResourceDescriptor textureWaterCaustics("Terrain Water Caustics");
	textureWaterCaustics.setResourceLocation(terrain->getVariable("waterCaustics"));
	ResourceDescriptor textureTextureMap("Terrain Texture Map");
	textureTextureMap.setResourceLocation(terrain->getVariable("textureMap"));
  	_terrainDiffuseMap = ResourceManager::getInstance().loadResource<Texture>(textureTextureMap);
	_terrainDiffuseMap->setTextureWrap(true,true);
	_terrainTextures.push_back(ResourceManager::getInstance().loadResource<Texture>(textureNormalMap));
	_terrainTextures.push_back(ResourceManager::getInstance().loadResource<Texture>(textureWaterCaustics));
	_terrainTextures.push_back(ResourceManager::getInstance().loadResource<Texture>(textureRedTexture));
	_terrainTextures.push_back(ResourceManager::getInstance().loadResource<Texture>(textureGreenTexture));
	_terrainTextures.push_back(ResourceManager::getInstance().loadResource<Texture>(textureBlueTexture));
	if(alphaTextureFile.compare(ParamHandler::getInstance().getParam<string>("assetsLocation") + "/none") != 0){
		_alphaTexturePresent = true;
		ResourceDescriptor textureAlphaTexture("Terrain Alpha Texture");
		textureAlphaTexture.setResourceLocation(alphaTextureFile);
		_terrainTextures.push_back(ResourceManager::getInstance().loadResource<Texture>(textureAlphaTexture));
	}
	
	_groundVBO = GFXDevice::getInstance().newVBO();
	std::vector<vec3>&		tPosition	= _groundVBO->getPosition();
	std::vector<vec3>&		tNormal		= _groundVBO->getNormal();
	std::vector<vec3>&		tTangent	= _groundVBO->getTangent();

	U8 d;
	U32 t, id;
	U8* data = ImageTools::OpenImage(terrain->getVariable("heightmap"), _terrainWidth, _terrainHeight, d, t, id);
	Console::getInstance().d_printfn("Terrain width: %d and height: %d",_terrainWidth, _terrainHeight);
	assert(data);

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

			F32 h = (F32)(data[idxIMG*d + 0] + data[idxIMG*d + 1] + data[idxIMG*d + 2])/3.0f;

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

			idx0 = COORD(i,	_terrainHeight-1-j,		 _terrainWidth);
			idx1 = COORD(i,	_terrainHeight-1-offset, _terrainWidth);
	
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
		ResourceDescriptor infinitePlane("infinitePlane");
		infinitePlane.setFlag(true); //No default material
		_farPlane = 2.0f * ParamHandler::getInstance().getParam<F32>("zFar");
		_plane = ResourceManager::getInstance().loadResource<Quad3D>(infinitePlane);
		F32 depth = SceneManager::getInstance().getActiveScene()->getWaterDepth();
		F32 height = SceneManager::getInstance().getActiveScene()->getWaterLevel()- depth;
		_farPlane = 2.0f * ParamHandler::getInstance().getParam<F32>("zFar");
		const vec3& eyePos = CameraManager::getInstance().getActiveCamera()->getEye();

		_plane->setCorner(Quad3D::TOP_LEFT, vec3(eyePos.x - _farPlane, height, eyePos.z - _farPlane));
		_plane->setCorner(Quad3D::TOP_RIGHT, vec3(eyePos.x + _farPlane, height, eyePos.z - _farPlane));
		_plane->setCorner(Quad3D::BOTTOM_LEFT, vec3(eyePos.x - _farPlane, height, eyePos.z + _farPlane));
		_plane->setCorner(Quad3D::BOTTOM_RIGHT, vec3(eyePos.x + _farPlane, height, eyePos.z + _farPlane));
		ResourceDescriptor terrainMaterial("terrainMaterial");
		setMaterial(ResourceManager::getInstance().loadResource<Material>(terrainMaterial));
		getMaterial()->setDiffuse(vec4(1.0f, 1.0f, 1.0f, 1.0f));
		getMaterial()->setSpecular(vec4(1.0f, 1.0f, 1.0f, 1.0f));
		getMaterial()->setShininess(50.0f);
		getMaterial()->setShaderProgram("terrain");
		Console::getInstance().printfn("Loading Terrain OK");
	}else{
		Console::getInstance().errorfn("Loading Terrain NOT OK");
	}
	return _loaded;
}

void Terrain::postLoad(SceneGraphNode* const node){
	
	if(!_loaded) node->setActive(false);
	_planeSGN = node->addNode(_plane);
	_plane->computeBoundingBox(_planeSGN);
	_plane->setRenderState(false);
	_planeSGN->setActive(false);
	_planeTransform = _planeSGN->getTransform();
	computeBoundingBox(node);
	ShaderProgram* s = getMaterial()->getShaderProgram();
	s->bind();
		s->Uniform("alphaTexture", _alphaTexturePresent);
		s->Uniform("detail_scale", 120.0f);
		s->Uniform("diffuse_scale", 70.0f);
		s->Uniform("bbox_min", _boundingBox.getMin());
		s->Uniform("bbox_max", _boundingBox.getMax());
		s->Uniform("water_height", SceneManager::getInstance().getActiveScene()->getWaterLevel());
		s->Uniform("enable_shadow_mapping", ParamHandler::getInstance().getParam<bool>("enableShadows"));
	s->unbind();
	_node = node;
	
}

bool Terrain::computeBoundingBox(SceneGraphNode* const node){
	_terrainQuadtree->computeBoundingBox(_groundVBO->getPosition());
	node->getBoundingBox() = _boundingBox;
	return  SceneNode::computeBoundingBox(node);
}

void Terrain::initializeVegetation(TerrainDescriptor* terrain) {	
	vector<Texture2D*> grass;
	ResourceDescriptor grassBillboard1("Grass Billboard 1");
	grassBillboard1.setResourceLocation(terrain->getVariable("grassBillboard1"));
	ResourceDescriptor grassBillboard2("Grass Billboard 2");
	grassBillboard2.setResourceLocation(terrain->getVariable("grassBillboard2"));
	ResourceDescriptor grassBillboard3("Grass Billboard 3");
	grassBillboard3.setResourceLocation(terrain->getVariable("grassBillboard3"));
	//ResourceDescriptor grassBillboard4("Grass Billboard 4");
	//grassBillboard4.setResourceLocation(terrain->getVariable("grassBillboard4"));
	grass.push_back(ResourceManager::getInstance().loadResource<Texture2D>(grassBillboard1));
	grass.push_back(ResourceManager::getInstance().loadResource<Texture2D>(grassBillboard2));
	grass.push_back(ResourceManager::getInstance().loadResource<Texture2D>(grassBillboard3));
	//grass.push_back(ResourceManager::getInstance().loadResource<Texture2D>(grassBillboard4));
	addVegetation(New Vegetation(3, terrain->getGrassDensity(),
			                        terrain->getGrassScale(),
								    terrain->getTreeDensity(),
									terrain->getTreeScale(),
									terrain->getVariable("grassMap"),
									grass),
				  "grass");
	toggleVegetation(true);
	 _veg->initialize(_grassShader,_name);
}

void Terrain::prepareMaterial(SceneGraphNode* const sgn){
	if(GFXDevice::getInstance().getRenderStage() == SHADOW_STAGE){
		return;
	}
	ShaderProgram* terrainShader = getMaterial()->getShaderProgram();
		

	_terrainDiffuseMap->Bind(0);
	_terrainTextures[0]->Bind(1); //Normal Map
	_terrainTextures[1]->Bind(2); //Water Caustics
	_terrainTextures[2]->Bind(3); //AlphaMap: RED
	_terrainTextures[3]->Bind(4); //AlphaMap: GREEN
	_terrainTextures[4]->Bind(5); //AlphaMap: BLUE

	terrainShader->bind();

	terrainShader->Uniform("water_reflection_rendering", _drawInReflection);
	terrainShader->UniformTexture("texDiffuseMap", 0);
	terrainShader->UniformTexture("texNormalHeightMap", 1);
	terrainShader->UniformTexture("texWaterCaustics", 2);
	terrainShader->UniformTexture("texDiffuse0", 3);
	terrainShader->UniformTexture("texDiffuse1", 4);
	terrainShader->UniformTexture("texDiffuse2", 5);
	if(_alphaTexturePresent){
		_terrainTextures[5]->Bind(6); //AlphaMap: Alpha
		terrainShader->UniformTexture("texDiffuse3",6);
	}
}

void Terrain::releaseMaterial(){
	if(GFXDevice::getInstance().getRenderStage() == SHADOW_STAGE){
		return;
	}
	//getMaterial()->getShaderProgram()->unbind();
	if(_alphaTexturePresent) _terrainTextures[5]->Unbind(6);
	_terrainTextures[4]->Unbind(5);
	_terrainTextures[3]->Unbind(4);
	_terrainTextures[2]->Unbind(3);
	_terrainTextures[1]->Unbind(2);
	_terrainTextures[0]->Unbind(1);
	_terrainDiffuseMap->Unbind(0);
}

void Terrain::render(SceneGraphNode* const node){

	if(GFXDevice::getInstance().getRenderStage() != SHADOW_STAGE){
		drawGround();
		drawInfinitePlain();
	}
}

void Terrain::postDraw(){
	_veg->draw(_drawInReflection);
}

void Terrain::drawInfinitePlain(){
	if(_drawInReflection) return;
	const vec3& eyePos = CameraManager::getInstance().getActiveCamera()->getEye();
	_planeTransform->setPositionX(eyePos.x);
	_planeTransform->setPositionZ(eyePos.z);
	_planeSGN->getBoundingBox().Transform(_planeSGN->getInitialBoundingBox(),_planeTransform->getMatrix());

	GFXDevice::getInstance().setObjectState(_planeTransform);
	GFXDevice::getInstance().renderModel(_plane);
	GFXDevice::getInstance().releaseObjectState(_planeTransform);

}

void Terrain::drawGround() const{
	assert(_groundVBO);
	_groundVBO->Enable();
		_terrainQuadtree->DrawGround(_drawInReflection);
	_groundVBO->Disable();
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