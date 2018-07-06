#include "TerrainManager.h"
#include "Rendering/Camera.h"
#include "Managers/ResourceManager.h"
#include "Hardware/Video/GFXDevice.h"
#include "Rendering/common.h"

TerrainManager::TerrainManager()
{
	_loaded = false;
	_computedMinHeight = false;
}

TerrainManager::~TerrainManager()
{
	delete _water;
	for(_resDBiter = _resDB.begin(); _resDBiter != _resDB.end(); _resDBiter++)
	{
		delete ((Terrain*)_resDBiter->second);
		_resDB.erase(_resDBiter);
	}
}

void TerrainManager::createTerrains(vector<TerrainInfo>& terrains)
{
	
	//_thrd = new boost::thread(&TerrainManager::createThreadedTerrains,this,terrains);
	createThreadedTerrains(terrains);
}

void TerrainManager::createThreadedTerrains(vector<TerrainInfo>& terrains)
{
	vector<TerrainInfo>::iterator _terrainIter;
	ResourceManager& res = ResourceManager::getInstance();
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

void TerrainManager::drawTerrains(bool drawInactive, bool drawInReflexion,vec4& ambientColor)
{
	if(!_loaded) return;
	if(_resDB.size() > 1)
	{
		for(_resDBiter = _resDB.begin(); _resDBiter != _resDB.end(); _resDBiter++)
		{
			_terrain = (Terrain*)_resDBiter->second;
			drawTerrain(drawInactive,drawInReflexion,ambientColor);
			
		}
	}
	else if(_resDB.size() == 1)
	{
		_terrain = (Terrain*)_resDB.begin()->second;
		drawTerrain(drawInactive,drawInReflexion);
	}
	else return;
}

void TerrainManager::drawTerrain(bool drawInactive, bool drawInReflexion,vec4& ambientColor)
{
	if(drawInactive) _terrain->setLoaded(true);
	else _terrain->restoreLoaded();

	_terrain->toggleRenderingParams(drawInReflexion,ambientColor);
	_terrain->draw();
}

void TerrainManager::generateVegetation()
{
	for(_resDBiter = _resDB.begin(); _resDBiter != _resDB.end(); _resDBiter++)
	{
		((Terrain*)_resDBiter->second)->toggleVegetation(true);
		((Terrain*)_resDBiter->second)->initializeVegetation();
	}
}

void TerrainManager::generateVegetation(const string& name)
{
	if(_resDB.find(name) != _resDB.end())
	{
		((Terrain*)_resDB[name])->toggleVegetation(true);
		((Terrain*)_resDB[name])->initializeVegetation();
	}
}

void TerrainManager::drawInfinitePlane(F32 max_distance,FrameBufferObject& _fbo)
{
	
	if(!_loaded) return;
	ParamHandler&  par = ParamHandler::getInstance();
	if(!_computedMinHeight)
	{
		_minHeight = 1000;_maxHeight = 1; F32 temp = 0;
		for(_resDBiter = _resDB.begin(); _resDBiter != _resDB.end(); _resDBiter++)
		{
			temp = ((Terrain*)_resDBiter->second)->getBoundingBox().min.y;
			if(temp < _minHeight)
			{
				_minHeight = temp;
				_maxHeight = ((Terrain*)_resDBiter->second)->getBoundingBox().max.y;
			}
		}
		_minHeight -= 75;
		par.setParam("minHeight",_minHeight);
		_computedMinHeight = true;
	}
	const vec3& eye = Camera::getInstance().getEye();
	_water->getQuad()->_tl = vec3(eye.x - max_distance, _minHeight, eye.z - max_distance);
	_water->getQuad()->_tr = vec3(eye.x + max_distance, _minHeight, eye.z - max_distance);
	_water->getQuad()->_bl = vec3(eye.x - max_distance, _minHeight, eye.z + max_distance);
	_water->getQuad()->_br = vec3(eye.x + max_distance, _minHeight, eye.z + max_distance);
	_water->draw(_fbo);
}