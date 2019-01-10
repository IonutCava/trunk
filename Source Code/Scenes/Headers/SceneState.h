/*
   Copyright (c) 2018 DIVIDE-Studio
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

#include "Platform/Audio/Headers/SFXDevice.h"
#include "Platform/Video/Headers/RenderAPIEnums.h"
#include "Platform/Audio/Headers/AudioDescriptor.h"
#include "Platform/Video/Headers/ClipPlanes.h"
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

enum class MusicType : U8 {
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
};

/// Contains all the information needed to render the scene: camera position,
/// render state, etc
class SceneRenderState : public SceneComponent {
    friend class Attorney::SceneRenderStateScene;

   public:
    enum class GizmoState : U8 {
        NO_GIZMO = toBit(1),
        SCENE_GIZMO = toBit(2),
        SELECTED_GIZMO = toBit(3),
        ALL_GIZMO = SCENE_GIZMO | SELECTED_GIZMO,
        COUNT = 4
    };

    enum class RenderOptions : U16 {
        /// Show/hide bounding boxes and/or objects
        RENDER_AABB = toBit(1),
        /// Show/hide debug lines
        RENDER_DEBUG_LINES = toBit(2),
        RENDER_DEBUG_TARGET_LINES = toBit(3),
        /// Show/hide geometry
        RENDER_GEOMETRY = toBit(4),
        /// Render skeletons for animated geometry
        RENDER_SKELETONS = toBit(5),
        /// Render wireframe for all scene geometry
        RENDER_WIREFRAME = toBit(6),
        RENDER_OCTREE_REGIONS = toBit(7),
        PLAY_ANIMATIONS = toBit(8),
        COUNT = 8
    };

    explicit SceneRenderState(Scene& parentScene);

    void renderMask(U32 mask);
    bool isEnabledOption(RenderOptions option) const;
    void enableOption(RenderOptions option);
    void disableOption(RenderOptions option);
    void toggleOption(RenderOptions option);
    void toggleOption(RenderOptions option, const bool state);

    /// Show/hide axis gizmos
    void toggleAxisLines();
    
    inline void gizmoState(GizmoState newState) {
        _gizmoState = newState;
    }
    
    inline GizmoState gizmoState() const {
        return _gizmoState;
    }

    inline void generalVisibility(F32 distance) { _generalVisibility = distance; }
    inline F32  generalVisibility()       const { return _generalVisibility; }

    inline void grassVisibility(F32 distance) { _grassVisibility = distance; }
    inline F32  grassVisibility()       const { return _grassVisibility; }

    inline void treeVisibility(F32 distance) { _treeVisibility = distance; }
    inline F32  treeVisibility()       const { return _treeVisibility; }

    inline void renderPass(U8 renderPass) { _renderPass = renderPass; }
    inline U8   renderPass()      const { return _renderPass; }

    inline vec4<U16>& lodThresholds() { return _lod; }
    inline FogDescriptor& fogDescriptor() { return _fog; }
    inline const vec4<U16>& lodThresholds() const { return _lod; }
    inline const FogDescriptor& fogDescriptor() const { return _fog; }

   protected:
       
    vec4<U16> _lod;
    FogDescriptor _fog;
    U8 _renderPass;
    U32 _stateMask;
    GizmoState _gizmoState;
    F32 _grassVisibility;
    F32 _treeVisibility;
    F32 _generalVisibility;
};

class Camera;

enum class MoveDirection : I8 {
    NONE = 0,
    NEGATIVE = -1,
    POSITIVE = 1
};

class SceneStatePerPlayer {
  public:
    SceneStatePerPlayer() noexcept
      : _headHeight(1.82f)
    {
        resetAll();
    }

    inline void resetMovement() {
        _moveFB = _moveLR = _moveUD = _angleUD = _angleLR = _roll = _zoom = MoveDirection::NONE;
    }

    inline void resetAll() {
        resetMovement();
        _cameraUnderwater = false;
        _cameraUpdated = false;
        _overrideCamera = nullptr;
        _cameraLockedToMouse = false;
    }

    inline void cameraUnderwater(bool state) { _cameraUnderwater = state; }
    inline bool cameraUnderwater()     const { return _cameraUnderwater; }

    inline void cameraUpdated(bool state) { _cameraUpdated = state; }
    inline bool cameraUpdated()     const { return _cameraUpdated; }

    inline void moveFB(MoveDirection factor) { _moveFB = factor; }
    inline MoveDirection  moveFB()     const { return _moveFB; }

    inline void moveLR(MoveDirection factor) { _moveLR = factor; }
    inline MoveDirection  moveLR()     const { return _moveLR; }

    inline void moveUD(MoveDirection factor) { _moveUD = factor; }
    inline MoveDirection  moveUD()     const { return _moveUD; }

    inline void angleUD(MoveDirection factor) { _angleUD = factor; }
    inline MoveDirection  angleUD()     const { return _angleUD; }

    inline void angleLR(MoveDirection factor) { _angleLR = factor; }
    inline MoveDirection  angleLR()     const { return _angleLR; }

    inline void roll(MoveDirection factor) { _roll = factor; }
    inline MoveDirection  roll()     const { return _roll; }

    inline void zoom(MoveDirection factor) { _zoom = factor; }
    inline MoveDirection  zoom()     const { return _zoom; }

    inline void cameraLockedToMouse(bool state) { _cameraLockedToMouse = state; }
    inline bool cameraLockedToMouse()     const { return _cameraLockedToMouse; }

    inline void    overrideCamera(Camera* camera) { _overrideCamera = camera; }
    inline Camera* overrideCamera()         const { return _overrideCamera; }

    inline F32 headHeight() const { return _headHeight; }

private:
    const F32 _headHeight = 0.5f;
    bool _cameraLockedToMouse;
    MoveDirection _moveFB;   ///< forward-back move change detected
    MoveDirection _moveLR;   ///< left-right move change detected
    MoveDirection _moveUD;   ///< up-down move change detected
    MoveDirection _angleUD;  ///< up-down angle change detected
    MoveDirection _angleLR;  ///< left-right angle change detected
    MoveDirection _roll;     ///< roll left or right change detected
    MoveDirection _zoom;     ///< zoom in or out detected
    bool _cameraUnderwater;
    // was the camera moved or rotated this frame
    bool _cameraUpdated;
    Camera* _overrideCamera;
};

struct WaterDetails {
    F32 _heightOffset = 0.0f;
    F32 _depth = 0.0f;
    vec3<F32> _normal = WORLD_Y_AXIS;
};

class SceneState : public SceneComponent {
   public:
    /// Background music map : trackName - track
    typedef hashMap<U64, AudioDescriptor_ptr> MusicPlaylist;

    SceneState(Scene& parentScene)
        : SceneComponent(parentScene),
          _renderState(parentScene),
          _saveLoadDisabled(false),
          _playerPass(0),
          _windSpeed(1.0f),
          _windDirX(0.0f),
          _windDirZ(1.0f)
    {
    }

    virtual ~SceneState()
    {
        for (MusicPlaylist& playlist : _music) {
            playlist.clear();
        }
    }

    inline void onPlayerAdd(U8 index) {
        // Just reset everything
        onPlayerRemove(index);
    }

    inline void onPlayerRemove(U8 index) {
        _playerState[index].resetAll();
    }

    inline SceneStatePerPlayer& playerState() {
        return _playerState[playerPass()];
    }

    inline const SceneStatePerPlayer& playerState() const {
        return _playerState[playerPass()];
    }

    inline SceneStatePerPlayer& playerState(U8 index) {
        return _playerState[index];
    }

    inline const SceneStatePerPlayer& playerState(U8 index) const {
        return _playerState[index];
    }

    inline SceneRenderState& renderState()  { return _renderState; }
    inline MusicPlaylist& music(MusicType type) { return _music[to_U32(type)]; }

    inline const SceneRenderState& renderState() const { return _renderState; }
    inline const MusicPlaylist& music(MusicType type) const { return _music[to_U32(type)]; }

    inline void windSpeed(F32 speed) { _windSpeed = speed; }
    inline F32  windSpeed()    const { return _windSpeed; }

    inline void windDirX(F32 factor) { _windDirX = factor; }
    inline F32  windDirX()     const { return _windDirX; }

    inline void windDirZ(F32 factor) { _windDirZ = factor; }
    inline F32  windDirZ()     const { return _windDirZ; }

    inline vector<WaterDetails>& globalWaterBodies() { return _globalWaterBodies; }
    inline const vector<WaterDetails>& globalWaterBodies() const { return _globalWaterBodies; }

    inline void saveLoadDisabled(const bool state) { _saveLoadDisabled = state; }
    inline bool saveLoadDisabled()           const { return _saveLoadDisabled; }

    inline void playerPass(U8 pass) { _playerPass = pass; }
    inline U8   playerPass()  const { return _playerPass; }

protected:

    std::array<MusicPlaylist, to_base(MusicType::COUNT)> _music;
    std::array<SceneStatePerPlayer, Config::MAX_LOCAL_PLAYER_COUNT> _playerState;

    bool _saveLoadDisabled;

    vector<WaterDetails> _globalWaterBodies;

    /// saves all the rendering information for the scene
    /// (camera position, light info, draw states)
    SceneRenderState _renderState;
    F32 _windSpeed;
    F32 _windDirX;
    F32 _windDirZ;
    U8  _playerPass;
};

namespace Attorney {
class SceneRenderStateScene {
   private:
    static void playAnimations(SceneRenderState& sceneRenderState,
                               bool playAnimations) {
        if (playAnimations) {
            SetBit(sceneRenderState._stateMask, to_base(SceneRenderState::RenderOptions::PLAY_ANIMATIONS));
        } else {
            ClearBit(sceneRenderState._stateMask, to_base(SceneRenderState::RenderOptions::PLAY_ANIMATIONS));
        }
    }
    friend class Divide::Scene;
};

};  // namespace Attorney

};  // namespace Divide
#endif