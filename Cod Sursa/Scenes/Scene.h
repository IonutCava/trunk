#ifndef _SCENE_H
#define _SCENE_H

#include "resource.h"
#include "Hardware/Input/InputManager.h"
#include "Utility/Headers/BaseClasses.h"
#include "Utility/Headers/Event.h"
#include "Hardware/Video/GFXDevice.h"
#include "Hardware/Audio/SFXDevice.h"
#include "Importer/DVDConverter.h"
#include "Managers/TerrainManager.h"
#include "Hardware/Video/Light.h"

typedef std::tr1::shared_ptr<Event> Event_ptr;
typedef std::tr1::shared_ptr<Light> Light_ptr;
class Scene : public Resource
{

public:
	Scene() :
	  _GFX(GFXDevice::getInstance()),
	  _terMgr(new TerrainManager()),
	  _drawBB(false),
	  _drawObjects(true),
	  _lightTexture(NULL),
	  _deferredBuffer(NULL),
	  _deferredShader(NULL),
	  _inputManager(InputManagerInterface::getInstance())
	  {
		  _white = vec4(1.0f,1.0f,1.0f,1.0f);
		  _black = vec4(0.0f,0.0f,0.0f,0.0f);
	  };

	virtual ~Scene();
	void addGeometry(Object3D* const object);
	void addModel(Mesh* const model);
	bool removeGeometry(const std::string& name);
	bool removeModel(const std::string& name);

	virtual bool unload();

	virtual void render() = 0;
	virtual void preRender() = 0;
	virtual bool load(const std::string& name) = 0;
	
	virtual void processInput() = 0;
	virtual void processEvents(F32 time) = 0;

   U32 getNumberOfObjects(){return ModelArray.size() + GeometryArray.size();}
   U32 getNumberOfTerrains(){return TerrainInfoArray.size();}

   TerrainManager* getTerrainManager() {return _terMgr;}
   inline Shader*                                         getDeferredShaders() {return _deferredShader;}
   inline std::tr1::unordered_map<std::string,Mesh*>&     getModelArray(){return ModelArray;}
   inline std::tr1::unordered_map<std::string,Object3D*>& getGeometryArray(){return GeometryArray;}

   inline std::vector<FileData>& getModelDataArray() {return ModelDataArray;}
   inline std::vector<FileData>& getVegetationDataArray() {return VegetationDataArray;}

   inline std::vector<Light_ptr>& getLights() {return _lights;}
   inline std::vector<Event_ptr>& getEvents() {return _events;}

   void   addLight(Light_ptr lightItem) {_lights.push_back(lightItem);}
   void   addEvent(Event_ptr eventItem) {_events.push_back(eventItem);}

   void addModel(FileData& model) {ModelDataArray.push_back(model);}
   void addTerrain(const TerrainInfo& ter) {TerrainInfoArray.push_back(ter);}
   void addPatch(std::vector<FileData>& data);
   bool clean();

   bool drawBBox() {return _drawBB;}
   void drawBBox(bool visibility) {_drawBB = visibility;}
   bool drawObjects() {return _drawObjects;}
   void drawObjects(bool visibility) {_drawObjects=visibility;}

protected:

	GFXDevice& _GFX;
	TerrainManager* _terMgr;

	//Dynamic geometry
	std::tr1::unordered_map<std::string, Mesh*> ModelArray;
	std::tr1::unordered_map<std::string, Mesh*>::iterator ModelIterator;

	//Static geometry
	std::tr1::unordered_map<std::string, Object3D*> GeometryArray;
	std::tr1::unordered_map<std::string, Object3D*>::iterator GeometryIterator;

	//Datablocks for models,vegetation and terrains
	std::vector<FileData> ModelDataArray, PendingDataArray;
	std::vector<FileData> VegetationDataArray;
	std::vector<TerrainInfo> TerrainInfoArray;
	
	std::vector<F32> _eventTimers;

	bool _drawBB,_drawObjects;
	boost::mutex _mutex;

	vec4 _white, _black;
	InputManagerInterface& _inputManager;

	//Deferred rendering
	FrameBufferObject* _deferredBuffer;
	PixelBufferObject* _lightTexture;
	Shader*			   _deferredShader;

private: 
	std::vector<Event_ptr> _events;
	std::vector<Light_ptr> _lights;
protected:

	friend class SceneManager;
	virtual bool loadResources(bool continueOnErrors){return true;}
	virtual bool loadEvents(bool continueOnErrors){return true;}
	virtual void setInitialData();
	void clearEvents();
	void clearObjects();
	void clearLights();
	bool loadModel(const FileData& data);
	bool loadGeometry(const FileData& data);
	bool addDefaultLight();

public: //Input
	virtual void onKeyDown(const OIS::KeyEvent& key);
	virtual void onKeyUp(const OIS::KeyEvent& key);
	virtual void OnJoystickMoveAxis(const OIS::JoyStickEvent& key,I8 axis);
	virtual void OnJoystickMovePOV(const OIS::JoyStickEvent& key,I8 pov){}
	virtual void OnJoystickButtonDown(const OIS::JoyStickEvent& key,I8 button){}
	virtual void OnJoystickButtonUp(const OIS::JoyStickEvent& key, I8 button){}
	virtual void onMouseMove(const OIS::MouseEvent& key);
	virtual void onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button);
	virtual void onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button);
};

#endif
