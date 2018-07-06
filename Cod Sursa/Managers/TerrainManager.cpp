#include "TerrainManager.h"
#include "Managers/CameraManager.h"
#include "Managers/ResourceManager.h"
#include "Hardware/Video/GFXDevice.h"
#include "Terrain/Water.h"
#include "Terrain/Terrain.h"
#include "Rendering/common.h"
using namespace std;

TerrainManager::TerrainManager(){
	_loaded = false;
	_water = NULL;
	_computedMinHeight = false;
	_sunModelviewProj.identity();
}

TerrainManager::~TerrainManager(){
    Destroy();
	if(_water) {
		delete _water;
		_water = NULL;
	}

}

Terrain* TerrainManager::getTerrain(U8 index) {
	return dynamic_cast<Terrain*>(_resDB.begin()->second);
}

void TerrainManager::createTerrains(vector<TerrainInfo>& terrains){
	
	//_thrd = new boost::thread(&TerrainManager::createThreadedTerrains,this,terrains);
	createThreadedTerrains(terrains);
}

void TerrainManager::createThreadedTerrains(vector<TerrainInfo>& terrains){
	vector<TerrainInfo>::iterator _terrainIter;
	ResourceManager& res = ResourceManager::getInstance();
	if(terrains.empty()) return;
	for(_terrainIter = terrains.begin(); _terrainIter != terrains.end(); _terrainIter++)
	{
		_terrain = New Terrain((*_terrainIter).position,(*_terrainIter).scale);
		_terrain->load((*_terrainIter).variables["heightmap"]);
		_terrain->addTexture(res.LoadResource<Texture2D>((*_terrainIter).variables["normalMap"]));
		_terrain->addTexture(res.LoadResource<Texture2D>((*_terrainIter).variables["redTexture"]));
		_terrain->addTexture(res.LoadResource<Texture2D>((*_terrainIter).variables["greenTexture"]));
		_terrain->addTexture(res.LoadResource<Texture2D>((*_terrainIter).variables["blueTexture"]));
		//_terrain.addTexture(res.LoadResource<Texture2D>((*_terrainIter).variables["alphaTexture"]));
		_terrain->addTexture(res.LoadResource<Texture2D>((*_terrainIter).variables["waterCaustics"]));
		_terrain->setDiffuse(res.LoadResource<Texture2D>((*_terrainIter).variables["textureMap"]));
		_terrain->setShader(res.LoadResource<Shader>("terrain_ground"));
		_terrain->postLoad();
		_terrain->setLoaded((*_terrainIter).active);
		ImageTools::ImageData img;
		ImageTools::OpenImage((*_terrainIter).variables["grassMap"],img);
		vector<Texture2D*> grass;
		grass.push_back(res.LoadResource<Texture2D>((*_terrainIter).variables["grassBillboard1"]));
		grass.push_back(res.LoadResource<Texture2D>((*_terrainIter).variables["grassBillboard2"]));
		grass.push_back(res.LoadResource<Texture2D>((*_terrainIter).variables["grassBillboard3"]));
		//grass.push_back(res.LoadResource<Texture2D>((*_terrainIter).variables["grassBillboard4"]));
		_terrain->addVegetation(New Vegetation(*_terrain,3,
											 (*_terrainIter).grassDensity,
			                                 (*_terrainIter).grassScale,
											 (*_terrainIter).treeDensity,
											 (*_terrainIter).treeScale,
											 img,
											 grass),
								             "terrain_grass");
		_terrain->toggleVegetation(true);
		_terrain->initializeVegetation();
		_terrain->computeBoundingBox();
		add((*_terrainIter).variables["terrainName"],_terrain);
	}
	terrains.clear();
	//joinThread();
	_water = new WaterPlane();
	_loaded = true;
}

void TerrainManager::setDepthMap(U8 index, FrameBufferObject* depthMap)
{
	if(!_loaded) return;
	if(_resDB.size() > 0)
	{
		for(_resDBiter = _resDB.begin(); _resDBiter != _resDB.end(); _resDBiter++)
		{
			_terrain = dynamic_cast<Terrain*>(_resDBiter->second);
			_terrain->_depthMapFBO[index] = depthMap;
		}	
	}
	else return;
}

void TerrainManager::drawVegetation(bool drawInReflexion)
{
	if(_resDB.size() > 0)
		for(_resDBiter = _resDB.begin(); _resDBiter != _resDB.end(); _resDBiter++)
		{
			_terrain = dynamic_cast<Terrain*>(_resDBiter->second);
			_terrain->getVegetation()->draw(drawInReflexion);
		}
}


void TerrainManager::drawTerrains(mat4& sunModelviewProj, bool drawInactive, bool drawInReflexion, vec4& ambientColor)
{
	_sunModelviewProj = sunModelviewProj;
	if(!_loaded) return;
	if(_resDB.size() > 1)
	{
		for(_resDBiter = _resDB.begin(); _resDBiter != _resDB.end(); _resDBiter++)
		{
			_terrain = dynamic_cast<Terrain*>(_resDBiter->second);
			drawTerrain(drawInactive,drawInReflexion,ambientColor);
			
		}
	}
	else if(_resDB.size() == 1)
	{
		_terrain = dynamic_cast<Terrain*>(_resDB.begin()->second);
		drawTerrain(drawInactive,drawInReflexion,ambientColor);
	}
	else return;
}

void TerrainManager::drawTerrain(bool drawInactive, bool drawInReflexion,vec4& ambientColor)
{
	if(drawInactive) _terrain->setLoaded(true);
	else _terrain->restoreLoaded();

	_terrain->toggleRenderingParams(drawInReflexion,ambientColor,_sunModelviewProj);
	_terrain->draw();
}

void TerrainManager::generateVegetation()
{
	for(_resDBiter = _resDB.begin(); _resDBiter != _resDB.end(); _resDBiter++)
	{
		dynamic_cast<Terrain*>(_resDBiter->second)->toggleVegetation(true);
		dynamic_cast<Terrain*>(_resDBiter->second)->initializeVegetation();
	}
}

void TerrainManager::generateVegetation(const string& name)
{
	if(_resDB.find(name) != _resDB.end())
	{
		dynamic_cast<Terrain*>(_resDB[name])->toggleVegetation(true);
		dynamic_cast<Terrain*>(_resDB[name])->initializeVegetation();
	}
}

void TerrainManager::drawInfinitePlane(F32 max_distance,FrameBufferObject* fbo[])
{
	
	if(!_loaded) return;
	ParamHandler&  par = ParamHandler::getInstance();
	if(!_computedMinHeight)
	{
		_minHeight = 1000; F32 temp = 0;
		for(_resDBiter = _resDB.begin(); _resDBiter != _resDB.end(); _resDBiter++)
		{
			temp = dynamic_cast<Terrain*>(_resDBiter->second)->getBoundingBox().getMin().y;
			if(temp < _minHeight)
				_minHeight = temp;
		}
		_minHeight -= 75;
		par.setParam("minHeight",_minHeight);
		_computedMinHeight = true;
	}

	const vec3& eye = CameraManager::getInstance().getActiveCamera()->getEye();
	_water->getQuad()->getCorner(Quad3D::TOP_LEFT)     = vec3(eye.x - max_distance, _minHeight, eye.z - max_distance);
	_water->getQuad()->getCorner(Quad3D::TOP_RIGHT)    = vec3(eye.x + max_distance, _minHeight, eye.z - max_distance);
	_water->getQuad()->getCorner(Quad3D::BOTTOM_LEFT)  = vec3(eye.x - max_distance, _minHeight, eye.z + max_distance);
	_water->getQuad()->getCorner(Quad3D::BOTTOM_RIGHT) = vec3(eye.x + max_distance, _minHeight, eye.z + max_distance);
	_water->draw(fbo);
}