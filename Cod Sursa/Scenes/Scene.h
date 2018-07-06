#ifndef _SCENE_H
#define _SCENE_H

#include "resource.h"
#include "Hardware/Input/InputManager.h"
#include "Utility/Headers/BaseClasses.h"
#include "Utility/Headers/Event.h"
#include "Hardware/Video/GFXDevice.h"
#include "Importer/DVDConverter.h"
#include "Managers/TerrainManager.h"
#include "Hardware/Video/Light.h"


class Scene : public Resource
{

public:
	Scene() :
	  _GFX(GFXDevice::getInstance()),
	  _terMgr(new TerrainManager()),
	  _drawBB(false),
	  _drawObjects(true),
	  _inputManager(InputManagerInterface::getInstance())
	  {
		  _white = vec4(1.0f,1.0f,1.0f,1.0f);
		  _black = vec4(0.0f,0.0f,0.0f,0.0f);
	  };

	  ~Scene();
	void addGeometry(Object3D* const object);
	void addModel(DVDFile* const model);
	virtual bool unload();

	virtual void render() = 0;
	virtual void preRender() = 0;
	virtual bool load(const string& name) = 0;
	
	virtual void processInput() = 0;
	virtual void processEvents(F32 time) = 0;

   int getNumberOfObjects(){return ModelArray.size() + GeometryArray.size();}
   int getNumberOfTerrains(){return TerrainInfoArray.size();}

   TerrainManager* getTerrainManager() {return _terMgr;}

   inline unordered_map<string,DVDFile*>& getModelArray(){return ModelArray;}
   inline unordered_map<string,Object3D*>& getGeometryArray(){return GeometryArray;}

   inline vector<FileData>& getModelDataArray() {return ModelDataArray;}
   inline vector<FileData>& getVegetationDataArray() {return VegetationDataArray;}

   inline vector<Light*>& getLights() {return _lights;}
   inline vector<Event*>& getEvents() {return _events;}

   void addModel(FileData& model) {ModelDataArray.push_back(model);}
   void addTerrain(const TerrainInfo& ter) {TerrainInfoArray.push_back(ter);}
   void addPatch(vector<FileData>& data);
   void clean();

   bool drawBBox() {return _drawBB;}
   void drawBBox(bool visibility) {_drawBB = visibility;}
   bool drawObjects() {return _drawObjects;}
   void drawObjects(bool visibility) {_drawObjects=visibility;}

protected:

	GFXDevice& _GFX;
	TerrainManager* _terMgr;

	//Dynamic geometry
	unordered_map<string, DVDFile*> ModelArray;
	unordered_map<string, DVDFile*>::iterator ModelIterator;

	//Static geometry
	unordered_map<string, Object3D*> GeometryArray;
	unordered_map<string, Object3D*>::iterator GeometryIterator;

	//Datablocks for models,vegetation and terrains
	vector<FileData> ModelDataArray, PendingDataArray;
	vector<FileData> VegetationDataArray;
	vector<TerrainInfo> TerrainInfoArray;
	
	vector<Event*> _events;
	vector<Light*> _lights;

	Light* _defaultLight;

	bool _drawBB,_drawObjects;
	boost::mutex _mutex;

	vec4 _white, _black;
	InputManagerInterface& _inputManager;
	
protected:

	friend class SceneManager;
	virtual bool loadResources(bool continueOnErrors){return true;}
	virtual bool loadEvents(bool continueOnErrors){return true;}
	virtual void setInitialData();
	void clearEvents(){/*foreach(_events as event) event.end()*/_events.empty();}
	void clearObjects(){/*foreach(_ModelArray as model) model.unload();*/ GeometryArray.empty(); ModelArray.empty();}
	bool loadModel(const FileData& data);
	bool loadGeometry(const FileData& data);
	bool addDefaultLight();

public: //Input
	virtual void onKeyDown(const OIS::KeyEvent& key);
	virtual void onKeyUp(const OIS::KeyEvent& key);
	virtual void OnJoystickMoveAxis(const OIS::JoyStickEvent& key,I32 axis);
	virtual void OnJoystickMovePOV(const OIS::JoyStickEvent& key,I32 pov){}
	virtual void OnJoystickButtonDown(const OIS::JoyStickEvent& key,I32 button){}
	virtual void OnJoystickButtonUp(const OIS::JoyStickEvent& key, I32 button){}
	virtual void onMouseMove(const OIS::MouseEvent& key);
	virtual void onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button);
	virtual void onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button);
};

#endif