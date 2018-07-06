/*
   Copyright (c) 2015 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
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

/// This class contains all the variables that define each scene's
/// "unique"-ness:
/// background music, wind information, visibility settings, camera movement,
/// BB and Skeleton visibility, fog info, etc

/// Fog information (fog is so game specific, that it belongs in SceneState not
/// SceneRenderState

namespace Divide {

struct FogDescriptor {
    F32 _fogDensity;
    vec3<F32> _fogColor;
};

class Scene;
namespace Attorney {
    class SceneRenderStateScene;
    class SceneStateScene;
};

/// Contains all the information needed to render the scene: camera position,
/// render state, etc
class SceneRenderState {
    friend class Attorney::SceneRenderStateScene;

   public:
    enum class GizmoState : U32 {
        NO_GIZMO = toBit(0),
        SCENE_GIZMO = toBit(1),
        SELECTED_GIZMO = toBit(2),
        ALL_GIZMO = SCENE_GIZMO | SELECTED_GIZMO
    };

    enum class ObjectRenderState : U32 {
        NO_DRAW = toBit(0),
        DRAW_OBJECT = toBit(1),
        DRAW_BOUNDING_BOX = toBit(2),
        DRAW_OBJECT_WITH_BOUNDING_BOX = DRAW_OBJECT | DRAW_BOUNDING_BOX
    };

    SceneRenderState();

    /// Render skeletons for animated geometry
    void toggleSkeletons();
    /// Show/hide bounding boxes and/or objects
    void toggleBoundingBoxes();
    /// Show/hide axis gizmos
    void toggleAxisLines();

    inline void drawSkeletons(bool visibility) {
        _drawSkeletons = visibility;
    }

    inline bool drawSkeletons() const {
        return _drawSkeletons;
    }

    inline void drawDebugLines(bool visibility) {
        _debugDrawLines = visibility;
    }

    inline bool drawDebugLines() const {
        return _debugDrawLines;
    }

    inline void drawDebugTargetLines(bool visibility) {
        _debugDrawTargetLines = visibility;
    }

    inline bool drawDebugTargetLines() const {
        return _debugDrawTargetLines;
    }

    inline void gizmoState(GizmoState newState) {
        _gizmoState = newState;
    }

    inline GizmoState gizmoState() const {
        return _gizmoState;
    }

    inline void objectState(ObjectRenderState newState) {
        _objectState = newState;
    }

    inline ObjectRenderState objectState() const {
        return _objectState;
    }

    inline CameraManager& getCameraMgr() {
        return *_cameraMgr;
    }

    inline Camera& getCamera() {
        return *_cameraMgr->getActiveCamera();
    }

    inline const Camera& getCameraConst() const {
        return *_cameraMgr->getActiveCamera();
    }

    inline const vec2<U16>& cachedResolution() const {
        return _cachedResolution;
    }

    inline bool playAnimations() const {
        return _playAnimations;
    }
   
   protected:

    inline void playAnimations(bool state) { 
        _playAnimations = state; 
    }

    inline void cachedResolution(const vec2<U16>& resolution) {
        return _cachedResolution.set(resolution);
    }

   protected:
    bool _drawBB;
    bool _drawObjects;
    bool _drawSkeletons;
    bool _debugDrawLines;
    bool _debugDrawTargetLines;
    bool _playAnimations;
    GizmoState _gizmoState;
    ObjectRenderState _objectState;
    CameraManager* _cameraMgr;
    /// cached resolution
    vec2<U16> _cachedResolution;
};

class SceneState {
    friend class Attorney::SceneStateScene;
   public:
    /// Background music map : trackName - track
    typedef hashMapImpl<stringImpl, AudioDescriptor*> MusicPlaylist;

    SceneState()
        : _cameraUnderwater(false), 
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
            RemoveResource(it.second);
        }
        _backgroundMusic.clear();
    }

    inline void resetMovement() {
        _moveFB = _moveLR = _angleUD = _angleLR = _roll= 0;
    }

    inline FogDescriptor& fogDescriptor()   { return _fog; }
    inline SceneRenderState& renderState()  { return _renderState; }
    inline MusicPlaylist& backgroundMusic() { return _backgroundMusic; }

    inline void windSpeed(F32 speed) { _windSpeed = speed; }
    inline F32  windSpeed()    const { return _windSpeed; }

    inline void windDirX(F32 factor) { _windDirX = factor; }
    inline F32  windDirX()     const { return _windDirX; }

    inline void windDirZ(F32 factor) { _windDirZ = factor; }
    inline F32  windDirZ()     const { return _windDirZ; }

    inline void grassVisibility(F32 distance) { _grassVisibility = distance; }
    inline F32  grassVisibility()       const { return _grassVisibility; }

    inline void treeVisibility(F32 distance) { _treeVisibility = distance; }
    inline F32  treeVisibility()       const { return _treeVisibility; }

    inline void generalVisibility(F32 distance) { _generalVisibility = distance; }
    inline F32  generalVisibility()       const { return _generalVisibility; }

    inline void waterLevel(F32 level) { _waterHeight = level; }
    inline F32  waterLevel()    const { return _waterHeight; }

    inline void waterDepth(F32 depth) { _waterDepth = depth; }
    inline F32  waterDepth()    const { return _waterDepth; }

    inline void runningState(bool state) { _isRunning = state; }
    inline bool runningState()     const { return _isRunning; }
    
    inline void cameraUnderwater(bool state) { _cameraUnderwater = state; }
    inline bool cameraUnderwater()     const { return _cameraUnderwater; }

    inline void cameraUpdated(bool state) { _cameraUpdated = state; }
    inline bool cameraUpdated()     const { return _cameraUpdated; }

    inline void moveFB(I32 factor) { _moveFB = factor; }
    inline I32  moveFB()     const { return _moveFB; }

    inline void moveLR(I32 factor) { _moveLR = factor; }
    inline I32  moveLR()     const { return _moveLR; }

    inline void angleUD(I32 factor) { _angleUD = factor; }
    inline I32  angleUD()     const { return _angleUD; }

    inline void angleLR(I32 factor) { _angleLR = factor; }
    inline I32  angleLR()     const { return _angleLR; }

    inline void roll(I32 factor) { _roll = factor; }
    inline I32  roll()     const { return _roll; }

    inline void mouseXDelta(F32 depth) { _mouseXDelta = depth; }
    inline F32  mouseXDelta()    const { return _mouseXDelta; }

    inline void mouseYDelta(F32 depth) { _mouseYDelta = depth; }
    inline F32  mouseYDelta()    const { return _mouseYDelta; }

protected:
    MusicPlaylist _backgroundMusic;

    F32 _mouseXDelta;
    F32 _mouseYDelta;
    I32 _moveFB;   ///< forward-back move change detected
    I32 _moveLR;   ///< left-right move change detected
    I32 _angleUD;  ///< up-down angle change detected
    I32 _angleLR;  ///< left-right angle change detected
    I32 _roll;     ///< roll left or right change detected

    F32 _waterHeight;
    F32 _waterDepth;
    bool _cameraUnderwater;
    // was the camera moved or rotated this frame
    bool _cameraUpdated;  

    FogDescriptor _fog;
    /// saves all the rendering information for the scene
    /// (camera position, light info, draw states)
    SceneRenderState _renderState;
    bool _isRunning;
    F32 _grassVisibility;
    F32 _treeVisibility;
    F32 _generalVisibility;
    F32 _windSpeed;
    F32 _windDirX;
    F32 _windDirZ;
};

namespace Attorney {
class SceneRenderStateScene {
   private:
    static void cachedResolution(SceneRenderState& sceneRenderState,
                                 const vec2<U16>& resolution) {
        sceneRenderState.cachedResolution(resolution);
    }
    static void playAnimations(SceneRenderState& sceneRenderState,
                               bool playAnimations) {
        sceneRenderState.playAnimations(playAnimations);
    }
    friend class Divide::Scene;
};

class SceneStateScene {
private:
    friend class Divide::Scene;
};

};  // namespace Attorney

};  // namespace Divide
#endif