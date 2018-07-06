#ifndef _SCENE_H
#define _SCENE_H

#include "resource.h"
#include <unordered_map>
#include "Utility/Headers/BaseClasses.h"
#include "Utility/Headers/Event.h"
#include "Hardware/Video/GFXDevice.h"
#include "Importer/DVDConverter.h"
#include "Managers/TerrainManager.h"
#include "Hardware/Video/Light.h"

#include "Geometry/Predefined/Box3D.h"
#include "Geometry/Predefined/Text3D.h"
#include "Geometry/Predefined/Sphere3D.h"
#include "Geometry/Predefined/Quad3D.h"

class Scene : public Resource
{

public:
	Scene() :
	  _GFX(GFXDevice::getInstance()),
	  _terMgr(new TerrainManager()),
	  _drawBB(false),
	  _drawObjects(true)
	  {
	  };
	virtual void render() = 0;
	virtual void preRender() = 0;
	virtual bool load(const string& name) = 0;
	virtual bool unload() = 0;
	virtual void processInput() = 0;
	virtual void processEvents(F32 time) = 0;

   int getNumberOfObjects(){return ModelArray.size();}
   int getNumberOfTerrains(){return TerrainInfoArray.size();}

   TerrainManager* getTerrainManager() {return _terMgr;}

   inline vector<DVDFile*>& getModelArray(){return ModelArray;}

   inline vector<FileData>& getModelDataArray() {return ModelDataArray;}
   inline vector<FileData>& getVegetationDataArray() {return VegetationDataArray;}

   void addModel(FileData& model) {ModelDataArray.push_back(model);}
   void addTerrain(const TerrainInfo& ter) {TerrainInfoArray.push_back(ter);}
   void addPatch(vector<FileData>& data);
   void clean();
protected:
	GFXDevice& _GFX;
	TerrainManager* _terMgr;

	vector<DVDFile*> ModelArray;
	vector<DVDFile*>::iterator ModelIterator;

	//Datablocks for models,vegetation and terrains
	vector<FileData> ModelDataArray;
	vector<FileData> VegetationDataArray;
	vector<TerrainInfo> TerrainInfoArray;
	
	vector<Event*> _events;
	vector<Light*> _lights;
	bool& drawObjects() {return _drawObjects;}
	bool& drawBBox() {return _drawBB;}
	
protected:
	bool _drawBB,_drawObjects;
	boost::mutex _mutex;
	friend class SceneManager;

	
	virtual bool loadResources(bool continueOnErrors){return true;}
	virtual bool loadEvents(bool continueOnErrors){return true;}
	virtual void setInitialData();
	void clearEvents(){/*foreach(_events as event) event.end()*/_events.empty();}
	void clearObjects(){/*foreach(_ModelArray as model) model.unload();*/ ModelArray.empty();}

	void loadModel(FileData& data);

};

#endif