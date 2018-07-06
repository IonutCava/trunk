#include "Headers/Terrain.h"
#include "Quadtree/Headers/Quadtree.h"
#include "Hardware/Video/VertexBufferObject.h"
#include "Hardware/Video/RenderStateBlock.h"
#include "Headers/TerrainDescriptor.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Managers/Headers/CameraManager.h"

using namespace std;

#define COORD(x,y,w)	((y)*(w)+(x))

bool Terrain::unload(){
	_terrainWidth = 0;
	_terrainHeight = 0;

	SAFE_DELETE(_terrainQuadtree);
	SAFE_DELETE(_groundVBO);
	SAFE_DELETE(_terrainRenderState);
	SAFE_DELETE(_veg);

	for(U8 i = 0; i < _terrainTextures.size(); i++){
		RemoveResource(_terrainTextures[i]);
	}
	_terrainTextures.clear();
	RemoveResource(_terrainDiffuseMap);

	if(!_groundVBO)
		return SceneNode::unload(); 
	else return false;
}

bool Terrain::load(const string& name){
	_name = name;
	std::vector<TerrainDescriptor*>& terrains = SceneManager::getInstance().getActiveScene()->getTerrainInfoArray();
	PRINT_FN("Loading terrain [ %s ]",name.c_str());

	TerrainDescriptor* terrain = NULL;
	for(U8 i = 0; i < terrains.size(); i++)
		if(name.compare(terrains[i]->getVariable("terrainName")) == 0){
			terrain = terrains[i];
			break;
		}
	if(!terrain) return false;

	bool state = false;

	state = loadVisualResources(terrain);
	if(state){
		state =	loadThreadedResources(terrain);
	}

	if(state){
		PRINT_FN("Loading Terrain [ %s ] OK", _name.c_str());
		return true;
	}
	
	ERROR_FN("Error loading terrain [ %s ]", _name.c_str());
	return false;
}

/// Visual resources must be loaded in the rendering thread to gain acces to the current graphic context
bool Terrain::loadVisualResources(TerrainDescriptor* const terrain){
	ResourceDescriptor terrainMaterial("terrainMaterial");
	setMaterial(CreateResource<Material>(terrainMaterial));
	getMaterial()->setDiffuse(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	getMaterial()->setSpecular(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	getMaterial()->setShininess(50.0f);
	getMaterial()->setShaderProgram("terrain");

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
  	_terrainDiffuseMap = CreateResource<Texture>(textureTextureMap);
	_terrainDiffuseMap->setTextureWrap(true,true);
	_terrainTextures.push_back(CreateResource<Texture>(textureNormalMap));
	_terrainTextures.push_back(CreateResource<Texture>(textureWaterCaustics));
	_terrainTextures.push_back(CreateResource<Texture>(textureRedTexture));
	_terrainTextures.push_back(CreateResource<Texture>(textureGreenTexture));
	_terrainTextures.push_back(CreateResource<Texture>(textureBlueTexture));
	if(alphaTextureFile.compare(ParamHandler::getInstance().getParam<string>("assetsLocation") + "/none") != 0){
		_alphaTexturePresent = true;
		ResourceDescriptor textureAlphaTexture("Terrain Alpha Texture");
		textureAlphaTexture.setResourceLocation(alphaTextureFile);
		_terrainTextures.push_back(CreateResource<Texture>(textureAlphaTexture));
	}

	///Generate a render state
	RenderStateBlockDescriptor terrainDesc;
	terrainDesc.setCullMode(CULL_MODE_CW);
	terrainDesc._fixedLighting = true;
	_terrainRenderState = GFX_DEVICE.createStateBlock(terrainDesc);

	///For now, terrain doesn't cast shadows
	addToDrawExclusionMask(SHADOW_STAGE);

	if(_terrainRenderState){
		return true;
	}
	return false;
}

bool Terrain::loadThreadedResources(TerrainDescriptor* const terrain){
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

	_terrainHeightScaleFactor = terrain->getScale().y;

	_boundingBox.set(vec3(-300,  0.0f, -300),
					 vec3( 300, 40.0f,  300));
	_boundingBox.Translate(terrain->getPosition());
	_boundingBox.Multiply(vec3(terrain->getScale().x,1,terrain->getScale().x));
	_boundingBox.MultiplyMax(vec3(1,_terrainHeightScaleFactor,1));

	_groundVBO = GFX_DEVICE.newVBO();
	std::vector<vec3>&		vertexData	= _groundVBO->getPosition();
	std::vector<vec3>&		normalData	= _groundVBO->getNormal();
	std::vector<vec3>&		tangentData	= _groundVBO->getTangent();

	U8 d; U32 t, id;
	bool alpha = false;
	U8* data = ImageTools::OpenImage(terrain->getVariable("heightmap"), 
									_terrainWidth,
									_terrainHeight,
									 d, t, id,alpha);

	D_PRINT_FN("Terrain width: %d and height: %d",_terrainWidth, _terrainHeight);
	assert(data);
	U32 heightmapWidth  = _terrainWidth;
	U32 heightmapHeight = _terrainHeight;

	if(_terrainWidth%2==0)	_terrainWidth++;
	if(_terrainHeight%2==0)	_terrainHeight++;

	vertexData.resize(_terrainWidth*_terrainHeight);
	normalData.resize(_terrainWidth*_terrainHeight);
	tangentData.resize(_terrainWidth*_terrainHeight);

	for(U32 j=0; j < _terrainHeight; j++) {
		for(U32 i=0; i < _terrainWidth; i++) {

			U32 idxHM = COORD(i,j,_terrainWidth);
			vertexData[idxHM].x = _boundingBox.getMin().x + ((F32)i) * 
				                (_boundingBox.getMax().x - _boundingBox.getMin().x)/(_terrainWidth-1);
			vertexData[idxHM].z = _boundingBox.getMin().z + ((F32)j) * 
				                (_boundingBox.getMax().z - _boundingBox.getMin().z)/(_terrainHeight-1);
			U32 idxIMG = COORD(	i<heightmapWidth? i : i-1,
								j<heightmapHeight? j : j-1,
								heightmapWidth);

			F32 h = (F32)(data[idxIMG*d + 0] + data[idxIMG*d + 1] + data[idxIMG*d + 2])/3.0f;

			vertexData[idxHM].y = (_boundingBox.getMin().y + ((F32)h) * 
				                 (_boundingBox.getMax().y - _boundingBox.getMin().y)/(255)) * _terrainHeightScaleFactor;

		}
	}
	SAFE_DELETE_ARRAY(data);

	U32 offset = 2;


			
	for(U32 j=offset; j < _terrainHeight-offset; j++) {
		for(U32 i=offset; i < _terrainWidth-offset; i++) {

			U32 idx = COORD(i,j,_terrainWidth);
			
			vec3 vU = vertexData[COORD(i+offset, j+0, _terrainWidth)] - vertexData[COORD(i-offset, j+0, _terrainWidth)];
			vec3 vV = vertexData[COORD(i+0, j+offset, _terrainWidth)] - vertexData[COORD(i+0, j-offset, _terrainWidth)];

			normalData[idx].cross(vV, vU);
			normalData[idx].normalize();
			tangentData[idx] = -vU;
			tangentData[idx].normalize();
		}
	}

	for(U32 j=0; j < offset; j++) {
		for(U32 i=0; i < _terrainWidth; i++) {
			U32 idx0 = COORD(i,	j,		_terrainWidth);
			U32 idx1 = COORD(i,	offset,	_terrainWidth);

			normalData[idx0] = normalData[idx1];
		    tangentData[idx0] = tangentData[idx1];

			idx0 = COORD(i,	_terrainHeight-1-j,		 _terrainWidth);
			idx1 = COORD(i,	_terrainHeight-1-offset, _terrainWidth);
	
			normalData[idx0] = normalData[idx1];
			tangentData[idx0] = tangentData[idx1];
		}

	}

	for(U32 i=0; i < offset; i++) {
		for(U32 j=0; j < _terrainHeight; j++) {
			U32 idx0 = COORD(i,		    j,	_terrainWidth);
			U32 idx1 = COORD(offset,	j,	_terrainWidth);

			normalData[idx0] = normalData[idx1];
			tangentData[idx0] = tangentData[idx1];

			idx0 = COORD(_terrainWidth-1-i,		j,	_terrainWidth);
			idx1 = COORD(_terrainWidth-1-offset,	j,	_terrainWidth);

			normalData[idx0] = normalData[idx1];
			tangentData[idx0] = tangentData[idx1];
		}
	}

	U32 chunkSize = 16;
	_terrainQuadtree->setParentShaderProgram(getMaterial()->getShaderProgram());
	_terrainQuadtree->Build(_boundingBox, ivec2(_terrainWidth, _terrainHeight), chunkSize);

	
	ResourceDescriptor infinitePlane("infinitePlane");
	infinitePlane.setFlag(true); //No default material
	_plane = CreateResource<Quad3D>(infinitePlane);

	F32 depth = SceneManager::getInstance().getActiveScene()->getWaterDepth();
	F32 height = SceneManager::getInstance().getActiveScene()->getWaterLevel()- depth;
	const vec3& eyePos = CameraManager::getInstance().getActiveCamera()->getEye();
	_farPlane = 2.0f * ParamHandler::getInstance().getParam<F32>("zFar");
	_plane->setCorner(Quad3D::TOP_LEFT, vec3(eyePos.x - _farPlane, height, eyePos.z - _farPlane));
	_plane->setCorner(Quad3D::TOP_RIGHT, vec3(eyePos.x + _farPlane, height, eyePos.z - _farPlane));
	_plane->setCorner(Quad3D::BOTTOM_LEFT, vec3(eyePos.x - _farPlane, height, eyePos.z + _farPlane));
	_plane->setCorner(Quad3D::BOTTOM_RIGHT, vec3(eyePos.x + _farPlane, height, eyePos.z + _farPlane));

	return true;
}

void Terrain::postLoad(SceneGraphNode* const sgn){
	
	if(!_loaded) {
		sgn->setActive(false);
	}
	_node = sgn;

	_groundVBO->Create();
	_planeSGN = _node->addNode(_plane);
	_plane->computeBoundingBox(_planeSGN);
	_plane->setDrawState(false);
	_planeSGN->setActive(false);
	_planeTransform = _planeSGN->getTransform();
	computeBoundingBox(_node);
	ShaderProgram* s = getMaterial()->getShaderProgram();
	s->bind();
		s->Uniform("alphaTexture", _alphaTexturePresent);
		s->Uniform("detail_scale", 100.0f);
		s->Uniform("diffuse_scale", 100.0f);
		s->Uniform("bbox_min", _boundingBox.getMin());
		s->Uniform("bbox_max", _boundingBox.getMax());
		s->Uniform("water_height", SceneManager::getInstance().getActiveScene()->getWaterLevel());
		s->Uniform("enable_shadow_mapping", ParamHandler::getInstance().getParam<bool>("enableShadows"));
	s->unbind();
	
}

bool Terrain::computeBoundingBox(SceneGraphNode* const sgn){
	///The terrain's final bounding box is the QuadTree's root bounding box
	///Compute the QuadTree boundingboxes and get the root BB
	_boundingBox = _terrainQuadtree->computeBoundingBox(_groundVBO->getPosition());
	///Inform the scenegraph of our new BB
	sgn->getBoundingBox() = _boundingBox;
	return  SceneNode::computeBoundingBox(sgn);
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
	grass.push_back(CreateResource<Texture2D>(grassBillboard1));
	grass.push_back(CreateResource<Texture2D>(grassBillboard2));
	grass.push_back(CreateResource<Texture2D>(grassBillboard3));
	//grass.push_back(CreateResource<Texture2D>(grassBillboard4));
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