/*“Copyright 2009-2011 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SCENE_H
#define _SCENE_H

#include "resource.h"
#include "EngineGraphs/SceneGraph.h"
#include "Hardware/Input/InputManager.h"
#include "Utility/Headers/BaseClasses.h"
#include "Utility/Headers/Event.h"
#include "Hardware/Video/GFXDevice.h"
#include "Hardware/Audio/SFXDevice.h"
#include "Importer/DVDConverter.h"
#include "Terrain/TerrainDescriptor.h"
#include "Hardware/Video/Light.h"
#include "Utility/Headers/ParamHandler.h"
#include "Managers/ResourceManager.h"

typedef std::tr1::shared_ptr<Event> Event_ptr;
typedef unordered_map<std::string, Object3D*> Name_Object_map;
class SceneGraph;
class Scene : public Resource
{

public:
	Scene() :  Resource(),
	  _GFX(GFXDevice::getInstance()),
	  _resManager(ResourceManager::getInstance()),
	  _paramHandler(ParamHandler::getInstance()),
	  _drawBB(false),
	  _drawObjects(true),
	  _lightTexture(NULL),
	  _deferredBuffer(NULL),
	  _deferredShader(NULL),
	  _inputManager(InputManagerInterface::getInstance())
	  {
		  _white = vec4(1.0f,1.0f,1.0f,1.0f);
		  _black = vec4(0.0f,0.0f,0.0f,0.0f);
		  _sceneGraph = New SceneGraph();
		  _depthMap[0] = NULL;
		  _depthMap[1] = NULL;
	  };

	virtual ~Scene() {
		if(_depthMap[0]){
			delete _depthMap[0];
			_depthMap[0] = NULL;
		}
		if(_depthMap[1]){
			delete _depthMap[1];
			_depthMap[1] = NULL;
		}
	};
	SceneGraphNode* addGeometry(Object3D* const object);
	void addModel(Mesh* const model);
	bool removeGeometry(SceneNode* node);

	virtual bool unload();
	virtual void createCopy(){} //tough one ...
	virtual void removeCopy(){}
	virtual void render() = 0;
	virtual void preRender() = 0;
	virtual bool load(const std::string& name);
	
	virtual void processInput() = 0;
	virtual void processEvents(F32 time) = 0;

	inline F32& getWindSpeed(){return _windSpeed;}
	inline F32& getWindDirX(){return _windDirX;}
	inline F32& getWindDirZ(){return _windDirZ;}
	inline F32&  getGrassVisibility()		    {return _grassVisibility;}
	inline F32&  getTreeVisibility()		    {return _treeVisibility;}
	inline F32&  getGeneralVisibility()	  	{return _generalVisibility;}
	inline F32&  getWaterLevel()               {return _waterHeight;}
	inline F32&  getWaterDepth()               {return _waterDepth;}

   
   inline U32 getNumberOfTerrains(){return TerrainInfoArray.size();}
   inline std::vector<TerrainDescriptor*>& getTerrainInfoArray() {return TerrainInfoArray;}
   inline Shader*                           getDeferredShaders() {return _deferredShader;}
   
   inline FrameBufferObject* getDepthMap(I32 index){return _depthMap[std::min(std::abs(index),2)];}

   inline std::vector<FileData>& getModelDataArray() {return ModelDataArray;}
   inline std::vector<FileData>& getVegetationDataArray() {return VegetationDataArray;}

   inline std::vector<Light*>& getLights() {return _lights;}
   inline std::vector<Event_ptr>& getEvents() {return _events;}

   inline SceneGraph* getSceneGraph()	{return _sceneGraph;}
   inline void   addLight(Light* lightItem) {_lights.push_back(lightItem);}
   inline void   addEvent(Event_ptr eventItem) {_events.push_back(eventItem);}

   inline void addModel(FileData& model) {ModelDataArray.push_back(model);}
   inline void addTerrain(TerrainDescriptor* ter) {TerrainInfoArray.push_back(ter);}
   void addPatch(std::vector<FileData>& data);
   bool clean();

   inline bool drawBBox() {return _drawBB;}
   inline void drawBBox(bool visibility) {_drawBB = visibility;}
   inline bool drawObjects() {return _drawObjects;}
   inline void drawObjects(bool visibility) {_drawObjects=visibility;}

protected:

	GFXDevice& _GFX;
	ResourceManager& _resManager;
	ParamHandler& _paramHandler;

	/*Name_Object_map GeometryArray;
	Name_Object_map::iterator GeometryIterator;*/

	//Datablocks for models,vegetation and terrains
	std::vector<FileData> ModelDataArray, PendingDataArray;
	std::vector<FileData> VegetationDataArray;
	std::vector<TerrainDescriptor*> TerrainInfoArray;
	
	std::vector<F32> _eventTimers;

	bool _drawBB,_drawObjects;
	boost::mutex _mutex;

	vec4 _white, _black;
	InputManagerInterface& _inputManager;

	//Deferred rendering
	FrameBufferObject* _deferredBuffer;
	PixelBufferObject* _lightTexture;
	Shader*			   _deferredShader;
	SceneGraph*        _sceneGraph;
	F32			       _grassVisibility,_treeVisibility,_generalVisibility,
				 	   _windSpeed,_windDirX, _windDirZ, _waterHeight, _waterDepth;
	FrameBufferObject *_depthMap[2];
private: 
	std::vector<Event_ptr> _events;
	std::vector<Light*> _lights;

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
