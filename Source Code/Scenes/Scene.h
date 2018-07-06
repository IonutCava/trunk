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

typedef std::tr1::shared_ptr<Event> Event_ptr;
typedef std::tr1::unordered_map<std::string, Object3D*> Name_Object_map;
class SceneGraph;
class Scene : public Resource
{

public:
	Scene() :  Resource(),
	  _GFX(GFXDevice::getInstance()),
	  _drawBB(false),
	  _drawObjects(true),
	  _lightTexture(NULL),
	  _deferredBuffer(NULL),
	  _deferredShader(NULL),
	  _inputManager(InputManagerInterface::getInstance())
	  {
		  _white = vec4(1.0f,1.0f,1.0f,1.0f);
		  _black = vec4(0.0f,0.0f,0.0f,0.0f);
		  _sceneGraph = new SceneGraph();
	  };

	virtual ~Scene() {};
	void addGeometry(Object3D* const object);
	void addModel(Mesh* const model);
	bool removeGeometry(const std::string& name);

	virtual bool unload();

	virtual void render() = 0;
	virtual void preRender() = 0;
	virtual bool load(const std::string& name);
	
	virtual void processInput() = 0;
	virtual void processEvents(F32 time) = 0;

	F32& getWindSpeed(){return _windSpeed;}
	F32& getWindDirX(){return _windDirX;}
	F32& getWindDirZ(){return _windDirZ;}
	F32&  getGrassVisibility()		    {return _grassVisibility;}
	F32&  getTreeVisibility()		    {return _treeVisibility;}
	F32&  getGeneralVisibility()	  	{return _generalVisibility;}
	F32&  getWaterLevel()               {return _waterHeight;}
	F32&  getWaterDepth()               {return _waterDepth;}

   //U32 getNumberOfObjects(){return GeometryArray.size();}
   U32 getNumberOfTerrains(){return TerrainInfoArray.size();}
   std::vector<TerrainDescriptor*>& getTerrainInfoArray(){return TerrainInfoArray;}
   inline Shader*                                         getDeferredShaders() {return _deferredShader;}
   //inline std::tr1::unordered_map<std::string,Object3D*>& getGeometryArray(){return GeometryArray;}

   inline std::vector<FileData>& getModelDataArray() {return ModelDataArray;}
   inline std::vector<FileData>& getVegetationDataArray() {return VegetationDataArray;}

   inline std::vector<Light*>& getLights() {return _lights;}
   inline std::vector<Event_ptr>& getEvents() {return _events;}

   inline SceneGraph* getSceneGraph()	{return _sceneGraph;}
   void   addLight(Light* lightItem) {_lights.push_back(lightItem);}
   void   addEvent(Event_ptr eventItem) {_events.push_back(eventItem);}

   void addModel(FileData& model) {ModelDataArray.push_back(model);}
   void addTerrain(TerrainDescriptor* ter) {TerrainInfoArray.push_back(ter);}
   void addPatch(std::vector<FileData>& data);
   bool clean();

   bool drawBBox() {return _drawBB;}
   void drawBBox(bool visibility) {_drawBB = visibility;}
   bool drawObjects() {return _drawObjects;}
   void drawObjects(bool visibility) {_drawObjects=visibility;}

protected:

	GFXDevice& _GFX;

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
