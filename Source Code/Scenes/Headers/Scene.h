/*“Copyright 2009-2012 DIVIDE-Studio”*/
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

#include "core.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Utility/Headers/Event.h"
#include "Hardware/Video/GFXDevice.h"
#include "Hardware/Audio/SFXDevice.h"
#include "Core/Headers/ParamHandler.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Managers/Headers/LightManager.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Hardware/Input/Headers/InputInterface.h"

class Light;
class SceneGraph;
class TerrainDescriptor;

class Scene : public Resource{

public:
	typedef std::tr1::shared_ptr<Event> Event_ptr;

	Scene() :  Resource(),
	  _GFX(GFX_DEVICE),
	  _paramHandler(ParamHandler::getInstance()),
	  _drawBB(false),
	  _drawSkeletons(false),
	  _drawObjects(true),
	  _lightTexture(NULL),
	  _deferredBuffer(NULL),
	  _deferredShader(NULL),
	  _camera(NULL),
	  _sceneGraph(New SceneGraph())
	  {
		  _white = vec4<F32>(1.0f,1.0f,1.0f,1.0f);
		  _black = vec4<F32>(0.0f,0.0f,0.0f,0.0f);
		  _moveFB = 0.0f;
	      _moveLR = 0.0f;
		  _angleUD = 0.0f;
		  _angleLR = 0.0f;
	  };

	virtual ~Scene() {
		
	};
	SceneGraphNode* addGeometry(Object3D* const object);
	bool removeGeometry(SceneNode* node);

	virtual bool unload();
	virtual void render() = 0;
	virtual void preRender() = 0;
	virtual bool load(const std::string& name);
	
	virtual void processInput() = 0;
	virtual void processEvents(F32 time) = 0;

	/// Update animations, network data, sounds, triggers etc.
	virtual void updateSceneState(D32 sceneTime);
	/// Update current camera (simple, fast, inlined poitner swap)
	inline void updateCamera(Camera* const camera) {_camera = camera;}

	inline F32&  getWindSpeed()         {return _windSpeed;}
	inline F32&  getWindDirX()          {return _windDirX;}
	inline F32&  getWindDirZ()          {return _windDirZ;}
	inline F32&  getGrassVisibility()	{return _grassVisibility;}
	inline F32&  getTreeVisibility()	{return _treeVisibility;}
	inline F32&  getGeneralVisibility()	{return _generalVisibility;}
	inline F32&  getWaterLevel()        {return _waterHeight;}
	inline F32&  getWaterDepth()        {return _waterDepth;}

   
   inline U32 getNumberOfTerrains(){return TerrainInfoArray.size();}
   inline std::vector<TerrainDescriptor*>& getTerrainInfoArray() {return TerrainInfoArray;}
   inline ShaderProgram*                   getDeferredShaders() {return _deferredShader;}
   
   inline std::vector<FileData>& getModelDataArray() {return ModelDataArray;}
   inline std::vector<FileData>& getVegetationDataArray() {return VegetationDataArray;}

   inline std::vector<Event_ptr>& getEvents() {return _events;}

   inline SceneGraph* getSceneGraph()	{return _sceneGraph;}
   inline void   addEvent(Event_ptr eventItem) {_events.push_back(eventItem);}

   inline void addModel(FileData& model) {ModelDataArray.push_back(model);}
   inline void addTerrain(TerrainDescriptor* ter) {TerrainInfoArray.push_back(ter);}
   void addPatch(std::vector<FileData>& data);
   void addLight(Light* const lightItem);
   bool clean();

   inline bool drawBBox() {return _drawBB;}
   inline void drawBBox(bool visibility) {_drawBB = visibility;}
   inline bool drawSkeletons() {return  _drawSkeletons;}
   inline void drawSkeletons(bool visibility) {_drawSkeletons = visibility;}
   inline bool drawObjects() {return _drawObjects;}
   inline void drawObjects(bool visibility) {_drawObjects=visibility;}

   inline void cacheResolution(const vec2<U16>& newResolution) {_cachedResolution = newResolution;}

protected:

	GFXDevice& _GFX;
	ParamHandler& _paramHandler;

	///Datablocks for models,vegetation and terrains
	std::vector<FileData>           ModelDataArray;
	std::vector<FileData>           VegetationDataArray;
	std::vector<FileData>           PendingDataArray;
	std::vector<TerrainDescriptor*> TerrainInfoArray;
	
	std::vector<F32> _eventTimers;

	bool _drawBB,_drawObjects, _drawSkeletons;
	boost::mutex _mutex;

	vec4<F32> _white, _black;

	///Deferred rendering
	FrameBufferObject* _deferredBuffer;
	PixelBufferObject* _lightTexture;
	ShaderProgram*	   _deferredShader;

	///Global info
	Camera*            _camera;
	SceneGraph*        _sceneGraph;
	F32			       _grassVisibility,_treeVisibility,_generalVisibility,
				 	   _windSpeed,_windDirX, _windDirZ, _waterHeight, _waterDepth;
	F32 _moveFB;  ///< forward-back move change detected
	F32 _moveLR;  ///< left-right move change detected
	F32 _angleUD; ///< up-down angle change detected
	F32 _angleLR; ///< left-right angle change detected
   ///cached resolution
   vec2<U16> _cachedResolution;

private: 
	std::vector<Event_ptr> _events;
	Event_ptr _aiEvent;

protected:

	friend class SceneManager;
	///Description in SceneManager
	virtual bool initializeAI(bool continueOnErrors)   {return true;}
	///Description in SceneManager
	virtual bool deinitializeAI(bool continueOnErrors) {return true;}
	///Description in SceneManager
	virtual bool loadResources(bool continueOnErrors)  {return true;}
	virtual bool loadEvents(bool continueOnErrors);
	virtual void setInitialData();
	void clearEvents();
	void clearObjects();
	void clearLights();
	bool loadModel(const FileData& data);
	bool loadGeometry(const FileData& data);
	Light* addDefaultLight();

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
