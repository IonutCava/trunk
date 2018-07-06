/*
   Copyright (c) 2016 DIVIDE-Studio
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
#include "Scenes/Headers/SceneComponent.h"
#include "Utility/Headers/Localization.h"

/// This class contains all the variables that define each scene's
/// "unique"-ness:
/// background music, wind information, visibility settings, camera movement,
/// BB and Skeleton visibility, fog info, etc

/// Fog information (fog is so game specific, that it belongs in SceneState not
/// SceneRenderState

namespace Divide {

enum class MusicType : U32 {
    TYPE_BACKGROUND = 0,
    TYPE_COMBAT,
    COUNT
};


struct FogDescriptor {
  public:
    FogDescriptor();

    inline bool dirty() const { return _dirty; }
    inline F32 density() const { return _density; }
    inline const vec3<F32>& colour() const { return _colour; }

    inline void set(const vec3<F32>& colour, F32 density) {
        _colour.set(colour);
        _density = density;
        _dirty = true;
    }


  protected:
    friend class SceneManager;
    inline void clean() {
        _dirty = false;
    }
    inline void active(const bool state) {
        _active = state;
    }
    inline bool active() const {
        return _active;
    }
  private:
    bool _dirty;
    bool _active;
    F32 _density;
    vec3<F32> _colour;
};

class Scene;
class LightPool;
class RenderPass;
namespace Attorney {
    class SceneRenderStateScene;
    class SceneStateScene;
    class SceneRenderStateLightPool;
    class SceneRenderStateRenderPass;
};

/// Contains all the information needed to render the scene: camera position,
/// render state, etc
class SceneRenderState : public SceneComponent {
    friend class Attorney::SceneRenderStateScene;
    friend class Attorney::SceneRenderStateLightPool;
    friend class Attorney::SceneRenderStateRenderPass;

   public:
    enum class GizmoState : U32 {
        NO_GIZMO = toBit(1),
        SCENE_GIZMO = toBit(2),
        SELECTED_GIZMO = toBit(3),
        ALL_GIZMO = SCENE_GIZMO | SELECTED_GIZMO,
        COUNT = 4
    };

    explicit SceneRenderState(Scene& parentScene);

    /// Render wireframe for all scene geometry
    void toggleWireframe();
    /// Render skeletons for animated geometry
    void toggleSkeletons();
    /// Show/hide bounding boxes and/or objects
    void toggleBoundingBoxes();
    /// Show/hide axis gizmos
    void toggleAxisLines();
    /// Show/hide debug lines
    void toggleDebugLines();
    /// Show/hide geometry
    void toggleGeometry();


    inline void drawGeometry(bool state) {
        _drawGeometry = state;
    }

    inline bool drawGeometry() const {
        return _drawGeometry;
    }

    inline void drawSkeletons(bool state) {
        _drawSkeletons = state;
    }

    inline bool drawSkeletons() const {
        return _drawSkeletons;
    }

    inline void drawBoundingBoxes(bool state) {
        _drawBoundingBoxes = state;
    }

    inline bool drawBoundingBoxes() const {
        return _drawBoundingBoxes;
    }

    inline void drawWireframe(bool state) {
        _drawWireframe = state;
    }

    inline bool drawWireframe() const {
        return _drawWireframe;
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

    inline void drawOctreeRegions(bool visibility) {
        _drawOctreeRegions = visibility;
    }

    inline bool drawOctreeRegions() const {
        return _drawOctreeRegions;
    }

    inline void gizmoState(GizmoState newState) {
        _gizmoState = newState;
    }

    inline I32 currentStagePass() {
        return _currentStagePass;
    }
    
    inline GizmoState gizmoState() const {
        return _gizmoState;
    }

    inline CameraManager& getCameraMgr() {
        return *_cameraMgr;
    }

    inline Camera& getCamera() {
        return _cameraMgr->getActiveCamera();
    }

    inline const Camera& getCameraConst() const {
        return _cameraMgr->getActiveCamera();
    }

    inline bool playAnimations() const {
        return _playAnimations;
    }

   protected:

    inline void playAnimations(bool state) { 
        _playAnimations = state; 
    }

    inline void currentStagePass(I32 idx) {
        _currentStagePass = idx;
    }

   protected:
    bool _drawBB;
    bool _debugDrawLines;
    bool _debugDrawTargetLines;
    bool _playAnimations;

    bool _drawGeometry;
    bool _drawSkeletons;
    bool _drawBoundingBoxes;
    bool _drawWireframe;
    bool _drawOctreeRegions;

    I32 _currentStagePass;

    GizmoState _gizmoState;
    CameraManager* _cameraMgr;
};

class SceneState : public SceneComponent {
    friend class Attorney::SceneStateScene;
   public:
       enum class MoveDirection : I32 {
           NONE = 0,
           NEGATIVE = -1,
           POSITIVE = 1
       };

   public:
    /// Background music map : trackName - track
    typedef hashMapImpl<ULL, AudioDescriptor_ptr> MusicPlaylist;

    SceneState(Scene& parentScene)
        : SceneComponent(parentScene),
          _renderState(parentScene),
          _cameraUnderwater(false), 
          _cameraUpdated(false),
          _cameraLockedToMouse(false),
          _saveLoadDisabled(false),
          _waterHeight(0.0f),
          _waterDepth(0.0f),
          _grassVisibility(1.0f),
          _treeVisibility(1.0f),
          _generalVisibility(1.0f),
          _windSpeed(1.0f),
          _windDirX(0.0f),
          _windDirZ(1.0f)
    {
        resetMovement();
        _fog.set(vec3<F32>(0.2f, 0.2f, 0.2f), 0.01f);
    }

    virtual ~SceneState()
    {
        for (MusicPlaylist& playlist : _music) {
            playlist.clear();
        }
    }

    inline void resetMovement() {
        _moveFB = _moveLR = _angleUD = _angleLR = _roll = MoveDirection::NONE;
        _mouseXDelta = _mouseYDelta = 0;
    }

    inline FogDescriptor& fogDescriptor()   { return _fog; }
    inline SceneRenderState& renderState()  { return _renderState; }
    inline MusicPlaylist& music(MusicType type) { return _music[to_uint(type)]; }

    inline const FogDescriptor& fogDescriptor() const { return _fog; }
    inline const SceneRenderState& renderState() const { return _renderState; }
    inline const MusicPlaylist& music(MusicType type) const { return _music[to_uint(type)]; }

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

    inline void cameraUnderwater(bool state) { _cameraUnderwater = state; }
    inline bool cameraUnderwater()     const { return _cameraUnderwater; }

    inline void cameraUpdated(bool state) { _cameraUpdated = state; }
    inline bool cameraUpdated()     const { return _cameraUpdated; }

    inline void moveFB(MoveDirection factor) { _moveFB = factor; }
    inline MoveDirection  moveFB()     const { return _moveFB; }

    inline void moveLR(MoveDirection factor) { _moveLR = factor; }
    inline MoveDirection  moveLR()     const { return _moveLR; }

    inline void angleUD(MoveDirection factor) { _angleUD = factor; }
    inline MoveDirection  angleUD()     const { return _angleUD; }

    inline void angleLR(MoveDirection factor) { _angleLR = factor; }
    inline MoveDirection  angleLR()     const { return _angleLR; }

    inline void roll(MoveDirection factor) { _roll = factor; }
    inline MoveDirection  roll()     const { return _roll; }

    inline void cameraLockedToMouse(bool state) { _cameraLockedToMouse = state; }
    inline bool cameraLockedToMouse()     const { return _cameraLockedToMouse; }

    inline void mouseXDelta(I32 depth) { _mouseXDelta = depth; }
    inline I32  mouseXDelta()    const { return _mouseXDelta; }

    inline void mouseYDelta(I32 depth) { _mouseYDelta = depth; }
    inline I32  mouseYDelta()    const { return _mouseYDelta; }

    inline void saveLoadDisabled(const bool state) { _saveLoadDisabled = state; }
    inline bool saveLoadDisabled()           const { return _saveLoadDisabled; }

protected:

    std::array<MusicPlaylist, to_const_uint(MusicType::COUNT)> _music;

    I32 _mouseXDelta;
    I32 _mouseYDelta;
    bool _cameraLockedToMouse;
    bool _saveLoadDisabled;

    MoveDirection _moveFB;   ///< forward-back move change detected
    MoveDirection _moveLR;   ///< left-right move change detected
    MoveDirection _angleUD;  ///< up-down angle change detected
    MoveDirection _angleLR;  ///< left-right angle change detected
    MoveDirection _roll;     ///< roll left or right change detected

    F32 _waterHeight;
    F32 _waterDepth;
    bool _cameraUnderwater;
    // was the camera moved or rotated this frame
    bool _cameraUpdated;  

    FogDescriptor _fog;
    /// saves all the rendering information for the scene
    /// (camera position, light info, draw states)
    SceneRenderState _renderState;
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
    static void playAnimations(SceneRenderState& sceneRenderState,
                               bool playAnimations) {
        sceneRenderState.playAnimations(playAnimations);
    }
    friend class Divide::Scene;
};

class SceneRenderStateLightPool {
   private:
    static void currentStagePass(SceneRenderState& sceneRenderState,
                                 I32 currentStagePass) {
        sceneRenderState.currentStagePass(currentStagePass);
    }

    friend class Divide::LightPool;
};

class SceneRenderStateRenderPass {
   private:
    static void currentStagePass(SceneRenderState& sceneRenderState,
                                 I32 currentStagePass) {
        sceneRenderState.currentStagePass(currentStagePass);
    }

    friend class Divide::RenderPass;
};

class SceneStateScene {
private:
    friend class Divide::Scene;
};

};  // namespace Attorney

};  // namespace Divide
#endif