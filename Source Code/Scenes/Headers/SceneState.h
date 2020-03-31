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

    inline bool dirty() const noexcept { return _dirty; }
    inline F32 density() const noexcept { return _density; }
    inline const vec3<F32>& colour() const noexcept { return _colour; }

    inline void set(const vec3<F32>& colour, F32 density) {
        CLAMP_01(density);
        _colour.set(colour);
        _density = density;
        _dirty = true;
    }


  protected:
    friend class SceneManager;
    inline void clean() noexcept {
        _dirty = false;
    }
    inline void active(const bool state) noexcept {
        _active = state;
    }
    inline bool active() const noexcept {
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
        /// Show/hide bounding boxes
        RENDER_AABB = toBit(1),
        /// Show/hide bounding spheres
        RENDER_BSPHERES = toBit(2),
        /// Show/hide debug lines
        RENDER_DEBUG_LINES = toBit(3),
        RENDER_DEBUG_TARGET_LINES = toBit(4),
        /// Show/hide geometry
        RENDER_GEOMETRY = toBit(5),
        /// Render skeletons for animated geometry
        RENDER_SKELETONS = toBit(6),
        /// Render wireframe for all scene geometry
        RENDER_WIREFRAME = toBit(7),
        RENDER_OCTREE_REGIONS = toBit(8),
        PLAY_ANIMATIONS = toBit(9),
        COUNT = 9
    };

    explicit SceneRenderState(Scene& parentScene);

    void renderMask(U16 mask);
    bool isEnabledOption(RenderOptions option) const;
    void enableOption(RenderOptions option);
    void disableOption(RenderOptions option);
    void toggleOption(RenderOptions option);
    void toggleOption(RenderOptions option, const bool state);

    /// Show/hide axis gizmos
    void toggleAxisLines();
    
    inline void gizmoState(GizmoState newState) noexcept {
        _gizmoState = newState;
    }
    
    inline GizmoState gizmoState() const noexcept {
        return _gizmoState;
    }

    inline void generalVisibility(F32 distance) noexcept { _generalVisibility = distance; }
    inline F32  generalVisibility()       const noexcept { return _generalVisibility; }

    inline void grassVisibility(F32 distance) noexcept { _grassVisibility = distance; }
    inline F32  grassVisibility()       const noexcept { return _grassVisibility; }

    inline void treeVisibility(F32 distance) noexcept { _treeVisibility = distance; }
    inline F32  treeVisibility()       const noexcept { return _treeVisibility; }

    inline void renderPass(U8 renderPass) noexcept { _renderPass = renderPass; }
    inline U8   renderPass()        const noexcept { return _renderPass; }

    inline vec4<U16>& lodThresholds() noexcept { return _lod; }
    inline FogDescriptor& fogDescriptor() noexcept { return _fog; }
    inline const vec4<U16>& lodThresholds() const noexcept { return _lod; }
    inline const FogDescriptor& fogDescriptor() const noexcept { return _fog; }

   protected:
       
    vec4<U16> _lod;
    FogDescriptor _fog;
    GizmoState _gizmoState;
    F32 _grassVisibility;
    F32 _treeVisibility;
    F32 _generalVisibility;
    U16 _stateMask;
    U8 _renderPass;
};

class Camera;

enum class MoveDirection : I8 {
    NONE = 0,
    NEGATIVE = -1,
    POSITIVE = 1
};

constexpr F32 DEFAULT_PLAYER_HEIGHT = 1.82f;

class SceneStatePerPlayer {
  public:
    SceneStatePerPlayer() noexcept
      : _headHeight(DEFAULT_PLAYER_HEIGHT)
    {
        resetAll();
    }

    inline void resetMovement() noexcept {
        _moveFB = _moveLR = _moveUD = _angleUD = _angleLR = _roll = _zoom = MoveDirection::NONE;
    }

    inline void resetAll() noexcept {
        resetMovement();
        _cameraUnderwater = false;
        _cameraUpdated = false;
        _overrideCamera = nullptr;
        _cameraLockedToMouse = false;
    }

    inline void cameraUnderwater(bool state) noexcept { _cameraUnderwater = state; }
    inline bool cameraUnderwater()     const noexcept { return _cameraUnderwater; }

    inline void cameraUpdated(bool state) noexcept { _cameraUpdated = state; }
    inline bool cameraUpdated()     const noexcept { return _cameraUpdated; }

    inline void moveFB(MoveDirection factor) noexcept { _moveFB = factor; }
    inline MoveDirection  moveFB()     const noexcept { return _moveFB; }

    inline void moveLR(MoveDirection factor) noexcept { _moveLR = factor; }
    inline MoveDirection  moveLR()     const noexcept { return _moveLR; }

    inline void moveUD(MoveDirection factor) noexcept { _moveUD = factor; }
    inline MoveDirection  moveUD()     const noexcept { return _moveUD; }

    inline void angleUD(MoveDirection factor) noexcept { _angleUD = factor; }
    inline MoveDirection  angleUD()     const noexcept { return _angleUD; }

    inline void angleLR(MoveDirection factor) noexcept { _angleLR = factor; }
    inline MoveDirection  angleLR()     const noexcept { return _angleLR; }

    inline void roll(MoveDirection factor) noexcept { _roll = factor; }
    inline MoveDirection  roll()     const noexcept { return _roll; }

    inline void zoom(MoveDirection factor) noexcept { _zoom = factor; }
    inline MoveDirection  zoom()     const noexcept { return _zoom; }

    inline void cameraLockedToMouse(bool state) noexcept { _cameraLockedToMouse = state; }
    inline bool cameraLockedToMouse()     const noexcept { return _cameraLockedToMouse; }

    inline void    overrideCamera(Camera* camera) noexcept { _overrideCamera = camera; }
    inline Camera* overrideCamera()         const noexcept { return _overrideCamera; }

    inline F32 headHeight() const noexcept { return _headHeight; }

private:
    const F32 _headHeight = DEFAULT_PLAYER_HEIGHT;
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

    inline SceneRenderState& renderState() noexcept { return _renderState; }
    inline MusicPlaylist& music(MusicType type) noexcept { return _music[to_U32(type)]; }

    inline const SceneRenderState& renderState() const noexcept { return _renderState; }
    inline const MusicPlaylist& music(MusicType type) const noexcept { return _music[to_U32(type)]; }

    inline void windSpeed(F32 speed) noexcept { _windSpeed = speed; }
    inline F32  windSpeed()    const noexcept { return _windSpeed; }

    inline void windDirX(F32 factor) noexcept { _windDirX = factor; }
    inline F32  windDirX()     const noexcept { return _windDirX; }

    inline void windDirZ(F32 factor) noexcept { _windDirZ = factor; }
    inline F32  windDirZ()     const noexcept { return _windDirZ; }

    inline std::vector<WaterDetails>& globalWaterBodies() noexcept { return _globalWaterBodies; }
    inline const std::vector<WaterDetails>& globalWaterBodies() const noexcept { return _globalWaterBodies; }

    inline void saveLoadDisabled(const bool state) noexcept { _saveLoadDisabled = state; }
    inline bool saveLoadDisabled()           const noexcept { return _saveLoadDisabled; }

    inline void playerPass(U8 pass) noexcept { _playerPass = pass; }
    inline U8   playerPass()  const noexcept { return _playerPass; }

protected:

    std::array<MusicPlaylist, to_base(MusicType::COUNT)> _music;
    std::array<SceneStatePerPlayer, Config::MAX_LOCAL_PLAYER_COUNT> _playerState;
    std::vector<WaterDetails> _globalWaterBodies;

    /// saves all the rendering information for the scene
    /// (camera position, light info, draw states)
    SceneRenderState _renderState;
    F32 _windSpeed;
    F32 _windDirX;
    F32 _windDirZ;
    U8  _playerPass;

    bool _saveLoadDisabled;
};

namespace Attorney {
class SceneRenderStateScene {
   private:
    static void playAnimations(SceneRenderState& sceneRenderState, bool playAnimations) noexcept {
        if (playAnimations) {
            SetBit(sceneRenderState._stateMask, SceneRenderState::RenderOptions::PLAY_ANIMATIONS);
        } else {
            ClearBit(sceneRenderState._stateMask, SceneRenderState::RenderOptions::PLAY_ANIMATIONS);
        }
    }
    friend class Divide::Scene;
};

};  // namespace Attorney

};  // namespace Divide
#endif