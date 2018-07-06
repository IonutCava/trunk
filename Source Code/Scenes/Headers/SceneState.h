/*
   Copyright (c) 2013 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.
   
   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _SCENE_STATE_H_
#define _SCENE_STATE_H_

#include "core.h"
#include "Hardware/Audio/Headers/SFXDevice.h"
#include "Hardware/Video/Headers/RenderAPIEnums.h"
#include "Hardware/Audio/Headers/AudioDescriptor.h"
#include "Core/Resources/Headers/ResourceCache.h"

///This class contains all the variables that define each scene's "unique"-ness:
///background music, wind information, visibility settings, camera movement,
///BB and Skeleton visibility, fog info, etc

///Fog information (fog is so game specific, that it belongs in SceneState not SceneRenderState
struct FogDescriptor{
    FogMode _fogMode;
    F32 _fogDensity;
    F32 _fogStartDist;
    F32 _fogEndDist;
    vec3<F32> _fogColor;
};

class SceneState{
public:
	SceneState() :
	  _moveFB(0.0f),
	  _moveLR(0.0f),
	  _angleUD(0.0f),
	  _angleLR(0.0f),
	  _isRunning(false)
	{
		_fog._fogColor = vec3<F32>(0.2f, 0.2f, 0.2f);
		_fog._fogDensity = 0.01f;
        _fog._fogMode = FOG_EXP2;
        _fog._fogStartDist = 10;
        _fog._fogEndDist = 1000;
	}

	virtual ~SceneState(){
		for_each(MusicPlaylist::value_type& it, _backgroundMusic){
			if(it.second){
				RemoveResource(it.second);
			}
		}
		_backgroundMusic.clear();
	}
    inline FogDescriptor& getFogDesc()         {return _fog;}
	inline F32& getWindSpeed()                 {return _windSpeed;}
	inline F32& getWindDirX()                  {return _windDirX;}
	inline F32& getWindDirZ()                  {return _windDirZ;}
	inline F32& getGrassVisibility()           {return _grassVisibility;}
	inline F32& getTreeVisibility()	           {return _treeVisibility;}
	inline F32& getGeneralVisibility()         {return _generalVisibility;}
	inline F32& getWaterLevel()                {return _waterHeight;}
	inline F32& getWaterDepth()                {return _waterDepth;}
	inline bool getRunningState()              {return _isRunning;}
	inline void toggleRunningState(bool state) {_isRunning = state;}

	F32 _moveFB;  ///< forward-back move change detected
	F32 _moveLR;  ///< left-right move change detected
	F32 _angleUD; ///< up-down angle change detected
	F32 _angleLR; ///< left-right angle change detected

	F32 _waterHeight;
	F32 _waterDepth;
	///Background music map
	typedef Unordered_map<std::string /*trackName*/, AudioDescriptor* /*track*/> MusicPlaylist;
	MusicPlaylist _backgroundMusic;

protected:
	friend class Scene;
    FogDescriptor _fog;
	bool _isRunning;
	F32  _grassVisibility;
	F32  _treeVisibility;
	F32  _generalVisibility;
	F32  _windSpeed;
	F32  _windDirX;
	F32  _windDirZ;
};

class Camera;
///Contains all the information needed to render the scene:
///camera position, render state, etc
class SceneRenderState{
public:
	SceneRenderState(): _drawBB(false),
						_drawSkeletons(false),
	                    _drawObjects(true),
						_camera(NULL),
						_shadowMapResolutionFactor(1)
	{
	}

	inline bool drawBBox()                     {return _drawBB;}
	inline bool drawSkeletons()                {return  _drawSkeletons;}
	inline bool drawObjects()                  {return _drawObjects;}
	inline void drawBBox(bool visibility)      {_drawBB = visibility;}
	inline void drawSkeletons(bool visibility) {_drawSkeletons = visibility;}
	inline void drawObjects(bool visibility)   {_drawObjects=visibility;}
	///Render skeletons for animated geometry
	inline void toggleSkeletons() { drawSkeletons(!drawSkeletons()); }
	///Show/hide bounding boxes
	inline void toggleBoundingBoxes(){
		if(!drawBBox() && drawObjects())	{
			drawBBox(true);
			drawObjects(true);
		}else if (drawBBox() && drawObjects()){
			drawBBox(true);
			drawObjects(false);
		}else if (drawBBox() && !drawObjects()){
			drawBBox(false);
			drawObjects(false);
		}else{
			drawBBox(false);
			drawObjects(true);
		}
	}
	inline Camera* getCamera() {return _camera;}
	/// Update current camera (simple, fast, inlined poitner swap)
	inline void updateCamera(Camera* const camera) {_camera = camera;}
	inline vec2<U16>& cachedResolution() {return _cachedResolution;}
	///This can be dinamically controlled in case scene rendering needs it
	inline F32& shadowMapResolutionFactor() {return _shadowMapResolutionFactor;}
protected:
	friend class Scene;
	bool _drawBB;
	bool _drawObjects;
	bool _drawSkeletons;
	Camera*  _camera;
	///cached resolution
    vec2<U16> _cachedResolution;
	///cached shadowmap resolution factor
	F32       _shadowMapResolutionFactor;
};
#endif