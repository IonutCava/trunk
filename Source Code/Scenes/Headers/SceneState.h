/*
   Copyright (c) 2014 DIVIDE-Studio
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

#include "Managers/Headers/CameraManager.h"
#include "Platform/Audio/Headers/SFXDevice.h"
#include "Platform/Video/Headers/RenderAPIEnums.h"
#include "Platform/Audio/Headers/AudioDescriptor.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

///This class contains all the variables that define each scene's "unique"-ness:
///background music, wind information, visibility settings, camera movement,
///BB and Skeleton visibility, fog info, etc

///Fog information (fog is so game specific, that it belongs in SceneState not SceneRenderState

namespace Divide {

struct FogDescriptor{
    F32 _fogDensity;
    vec3<F32> _fogColor;
};

///Contains all the information needed to render the scene: camera position, render state, etc
class SceneRenderState {
public:
    enum GizmoState {
        NO_GIZMO       = toBit(0),
        SCENE_GIZMO    = toBit(1),
        SELECTED_GIZMO = toBit(2),
        ALL_GIZMO      = SCENE_GIZMO | SELECTED_GIZMO
    };
    
    enum ObjectRenderState {
        NO_DRAW           = toBit(0),
        DRAW_OBJECT       = toBit(1),
        DRAW_BOUNDING_BOX = toBit(2),
        DRAW_OBJECT_WITH_BOUNDING_BOX = DRAW_OBJECT | DRAW_BOUNDING_BOX
    };

    SceneRenderState(): _drawBB(false),
                        _drawSkeletons(false),
                        _drawObjects(true),
                        _debugDrawLines(false),
                        _debugDrawTargetLines(false),
                        _cameraMgr(nullptr)
    {
        _gizmoState = NO_GIZMO;
        _objectState = DRAW_OBJECT;
    }

    inline bool drawSkeletons()           const {return  _drawSkeletons;}
    inline void drawSkeletons(bool visibility)  {_drawSkeletons = visibility;}
    inline void drawDebugLines(bool visibility) {_debugDrawLines = visibility;}
    inline void drawDebugTargetLines(bool visibility) {_debugDrawTargetLines = visibility;}
    inline GizmoState gizmoState()      const   { return _gizmoState; }
    inline void gizmoState(GizmoState newState) { _gizmoState = newState; }
    inline ObjectRenderState objectState()       const      { return _objectState; }
    inline void objectState(ObjectRenderState newState)     { _objectState = newState; }
    /// Render skeletons for animated geometry
    inline void toggleSkeletons() {
        D_PRINT_FN(Locale::get("TOGGLE_SCENE_SKELETONS"));
        drawSkeletons(!drawSkeletons()); 
    }

    /// Show/hide bounding boxes and/or objects
    inline void toggleBoundingBoxes(){
        D_PRINT_FN(Locale::get("TOGGLE_SCENE_BOUNDING_BOXES"));
        if (objectState() == NO_DRAW) {
            objectState(DRAW_OBJECT);
        } else if (objectState() == DRAW_OBJECT) {
            objectState(DRAW_OBJECT_WITH_BOUNDING_BOX);
        } else if (objectState() == DRAW_OBJECT_WITH_BOUNDING_BOX) {
            objectState(DRAW_BOUNDING_BOX);
        } else {
            objectState(NO_DRAW);
        }
    }

    /// Show/hide axis gizmos
    inline void toggleAxisLines() {
        D_PRINT_FN(Locale::get("TOGGLE_SCENE_AXIS_GIZMO"));
        if (gizmoState() == NO_GIZMO) {
            gizmoState(SELECTED_GIZMO);
        } else if (gizmoState() == SELECTED_GIZMO) {
            gizmoState(ALL_GIZMO);
        } else if (gizmoState() == ALL_GIZMO) {
            gizmoState(SCENE_GIZMO);
        } else {
            gizmoState(NO_GIZMO);
        }
    }

    inline CameraManager& getCameraMgr()         { return *_cameraMgr;}
    inline       Camera&  getCamera()            { return *_cameraMgr->getActiveCamera(); }
    inline const Camera&  getCameraConst() const { return *_cameraMgr->getActiveCamera(); }
    inline vec2<U16>&     cachedResolution() {return _cachedResolution;}

protected:

    friend class Scene;
    bool _drawBB;
    bool _drawObjects;
    bool _drawSkeletons;
    bool _debugDrawLines;
    bool _debugDrawTargetLines;
    GizmoState _gizmoState;
    ObjectRenderState _objectState;
    CameraManager*  _cameraMgr;
    ///cached resolution
    vec2<U16> _cachedResolution;
};

class SceneState{
public:
    SceneState() :
      _cameraUnderwater(false),
      _cameraUpdated(false),
      _isRunning(false)
    {
        resetMovement();
        _fog._fogColor = vec3<F32>(0.2f, 0.2f, 0.2f);
        _fog._fogDensity = 0.01f;
    }

    virtual ~SceneState()
    {
		for (MusicPlaylist::value_type& it : _backgroundMusic) {
            RemoveResource( it.second );
        }
        _backgroundMusic.clear();
    }

    inline FogDescriptor&    getFogDesc()     {return _fog;}
    inline SceneRenderState& getRenderState() {return _renderState;}

    inline F32& getWindSpeed()                 {return _windSpeed;}
    inline F32& getWindDirX()                  {return _windDirX;}
    inline F32& getWindDirZ()                  {return _windDirZ;}
    inline F32& getGrassVisibility()           {return _grassVisibility;}
    inline F32& getTreeVisibility()	           {return _treeVisibility;}
    inline F32& getGeneralVisibility()         {return _generalVisibility;}
    inline F32& getWaterLevel()                {return _waterHeight;}
    inline F32& getWaterDepth()                {return _waterDepth;}
    inline bool getRunningState()        const {return _isRunning;}
    inline void toggleRunningState(bool state) {_isRunning = state;}

    inline void resetMovement() {
        _moveFB = 0;
        _moveLR = 0;
        _angleUD = 0;
        _angleLR = 0;
        _roll = 0;
    }
    F32 _mouseXDelta;
    F32 _mouseYDelta;
    I32 _moveFB;  ///< forward-back move change detected
    I32 _moveLR;  ///< left-right move change detected
    I32 _angleUD; ///< up-down angle change detected
    I32 _angleLR; ///< left-right angle change detected
    I32 _roll;    ///< roll left or right change detected

    F32 _waterHeight;
    F32 _waterDepth;
    bool _cameraUnderwater;
    bool _cameraUpdated; //was the camera moved or rotated this frame
    ///Background music map
    typedef hashMapImpl<stringImpl /*trackName*/, AudioDescriptor* /*track*/> MusicPlaylist;
    MusicPlaylist _backgroundMusic;

protected:
    friend class Scene;
    FogDescriptor _fog;
    ///saves all the rendering information for the scene (camera position, light info, draw states)
    SceneRenderState _renderState;
    bool _isRunning;
    F32  _grassVisibility;
    F32  _treeVisibility;
    F32  _generalVisibility;
    F32  _windSpeed;
    F32  _windDirX;
    F32  _windDirZ;
};

}; //namespace Divide
#endif