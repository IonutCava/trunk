#include "TerrainManager.h"
#include "Rendering/Camera.h"
#include "Managers/ResourceManager.h"
#include "Hardware/Video/GFXDevice.h"
#include "Rendering/common.h"
#include "TextureManager/Texture2D.h"

void TerrainManager::createTerrains(vector<TerrainInfo>& terrains)
{
	_loaded = false;
	//_thrd = new boost::thread(&TerrainManager::createThreadedTerrains,this,terrains);
	createThreadedTerrains(terrains);
}

void TerrainManager::createThreadedTerrains(vector<TerrainInfo>& terrains)
{
	vector<TerrainInfo>::iterator _terrainIter;
	ResourceManager& res = ResourceManager::getInstance();
	for(_terrainIter = terrains.begin(); _terrainIter != terrains.end(); ++_terrainIter)
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
	initialize();	
}

void TerrainManager::drawTerrains(bool drawInactive, bool drawInReflexion)
{
	if(!_loaded) return;
	if(_resDB.size() > 1)
	{
		for(_resDBiter = _resDB.begin(); _resDBiter != _resDB.end(); _resDBiter++)
		{
			_terrain = (Terrain*)_resDBiter->second;
			drawTerrain(drawInactive,drawInReflexion);
			
		}
	}
	else if(_resDB.size() == 1)
	{
		_terrain = (Terrain*)_resDB.begin()->second;
		drawTerrain(drawInactive,drawInReflexion);
	}
	else return;
}

void TerrainManager::drawTerrain(bool drawInactive, bool drawInReflexion)
{
	if(drawInactive) _terrain->setLoaded(true);
	else _terrain->restoreLoaded();

	_terrain->toggleReflexionRendering(drawInReflexion);
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
void TerrainManager::initialize()
{
	 ResourceManager& res = ResourceManager::getInstance();
	_waterShader = res.LoadResource<Shader>("water");
	_texWater = res.LoadResource<Texture2D>(ParamHandler::getInstance().getParam<string>("assetsLocation")+"/misc_images/terrain_water_NM.jpg");
	_computedMinHeight = false;
	_loaded = true;
}

void TerrainManager::drawInfinitePlane(const vec3& eye, F32 max_distance,FrameBufferObject& _fbo)
{
	if(!_loaded) return;
	ParamHandler&  par = ParamHandler::getInstance();
	if(!_computedMinHeight)
	{
		_minHeight = -1;_maxHeight = 1; F32 temp = 0;
		for(_resDBiter = _resDB.begin(); _resDBiter != _resDB.end(); _resDBiter++)
		{
			temp = ((Terrain*)_resDBiter->second)->getBoundingBox().min.y;
			if(temp < _minHeight)
			{
				_minHeight = temp;
				_maxHeight = ((Terrain*)_resDBiter->second)->getBoundingBox().max.y;
			}
		}
		par.setParam("minHeight",_minHeight);
		_computedMinHeight = true;
	}

	
	glPushAttrib(GL_ENABLE_BIT);

	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);

	_fbo.Bind(0);
	_texWater->Bind(1);
	_waterShader->bind();
		_waterShader->UniformTexture("texWaterReflection", 0);
		_waterShader->UniformTexture("texWaterNoiseNM", 1);
		_waterShader->Uniform("bbox_min",vec3(eye.x - max_distance,_minHeight,eye.z-max_distance));
		_waterShader->Uniform("bbox_max",vec3(eye.x + max_distance,_maxHeight,eye.z+max_distance));
		_waterShader->Uniform("win_width",  Engine::getInstance().getWindowWidth());
		_waterShader->Uniform("win_height", Engine::getInstance().getWindowHeight());
		_waterShader->Uniform("noise_tile", 10.0f);
		_waterShader->Uniform("noise_factor", 0.1f);
		_waterShader->Uniform("time", GETTIME());
		_waterShader->Uniform("water_shininess", 50.0f);
		glBegin(GL_QUADS);
			glNormal3f(0.0f, 1.0f, 0.0f);
			glTexCoord3f(1.0f, 0.0f, 0.0f);
			glVertex3f(eye.x - max_distance, _minHeight, eye.z - max_distance);
			glVertex3f(eye.x - max_distance, _minHeight, eye.z + max_distance);
			glVertex3f(eye.x + max_distance, _minHeight, eye.z + max_distance);
			glVertex3f(eye.x + max_distance, _minHeight, eye.z - max_distance);
		glEnd();
	_waterShader->unbind();
	_texWater->Unbind(1);
	_fbo.Unbind(0);

	glPopAttrib();
}