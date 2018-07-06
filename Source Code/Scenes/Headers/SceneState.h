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

#ifndef _SCENE_STATE_H_
#define _SCENE_STATE_H_

#include "core.h"
#include "Hardware/Audio/SFXDevice.h"
#include "Hardware/Audio/AudioDescriptor.h"
#include "Core/Resources/Headers/ResourceCache.h"
///This class contains all the variables that define each scene's "unique"-ness:
///background music, wind information, visibility settings, camera movement,
///BB and Skeleton visibility, fog info, etc
class SceneState{

public:
	SceneState() : 
	  _drawBB(false),
	  _drawSkeletons(false),
	  _drawObjects(true),
	  _moveFB(0.0f),
	  _moveLR(0.0f),
	  _angleUD(0.0f),
	  _angleLR(0.0f)
	{
		_white = vec4<F32>(1.0f,1.0f,1.0f,1.0f);
		_black = vec4<F32>(0.0f,0.0f,0.0f,0.0f);
		_fogColor = vec4<F32>(0.2f, 0.2f, 0.4f, 1.0f);
		_fogDensity = 0.01f;
	}

	virtual ~SceneState(){
		for_each(MusicPlaylist::value_type& it, _backgroundMusic){
			if(it.second){
				RemoveResource(it.second);
			}
		}
		_backgroundMusic.clear();
	}
	inline F32& getWindSpeed()                 {return _windSpeed;}
	inline F32& getWindDirX()                  {return _windDirX;}
	inline F32& getWindDirZ()                  {return _windDirZ;}
	inline F32& getGrassVisibility()           {return _grassVisibility;}
	inline F32& getTreeVisibility()	           {return _treeVisibility;}
	inline F32& getGeneralVisibility()         {return _generalVisibility;}
	inline F32& getWaterLevel()                {return _waterHeight;}
	inline F32& getWaterDepth()                {return _waterDepth;}

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

protected:

	F32 _grassVisibility;
	F32 _treeVisibility;
	F32 _generalVisibility;
	F32 _windSpeed;
	F32 _windDirX;
	F32 _windDirZ;
	F32 _waterHeight;
	F32 _waterDepth;
	F32 _moveFB;  ///< forward-back move change detected
	F32 _moveLR;  ///< left-right move change detected
	F32 _angleUD; ///< up-down angle change detected
	F32 _angleLR; ///< left-right angle change detected

	bool _drawBB;
	bool _drawObjects;
	bool _drawSkeletons;

	///White and black (duh :P)
	vec4<F32> _white;
	vec4<F32> _black;

	///Fog information
	vec4<F32> _fogColor;
	F32       _fogDensity;
	///Background music map
	typedef unordered_map<std::string /*trackName*/, AudioDescriptor* /*track*/> MusicPlaylist;
	MusicPlaylist _backgroundMusic;
};

#endif