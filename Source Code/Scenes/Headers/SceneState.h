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

#pragma once
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

/// Contains all the information needed to render the scene: camera position,
/// render state, etc
class SceneRenderState : public SceneComponent {
   public:
    enum class RenderOptions : U16 {
        /// Show/hide bounding boxes
        RENDER_AABB = toBit(1),
        /// Show/hide bounding spheres
        RENDER_BSPHERES = toBit(2),
        /// Show/hide custom IMPrimitive elements
        RENDER_CUSTOM_PRIMITIVES = toBit(3),
        /// Show/hide geometry
        RENDER_GEOMETRY = toBit(4),
        /// Render skeletons for animated geometry
        RENDER_SKELETONS = toBit(5),
        /// Render wireframe for all scene geometry
        RENDER_WIREFRAME = toBit(6),
        RENDER_OCTREE_REGIONS = toBit(7),
        PLAY_ANIMATIONS = toBit(8),
        SCENE_GIZMO = toBit(9),
        SELECTION_GIZMO = toBit(10),
        ALL_GIZMOS = toBit(11),
        COUNT = 11
    };

    explicit SceneRenderState(Scene& parentScene);

    void renderMask(U16 mask);
    bool isEnabledOption(RenderOptions option) const;
    void enableOption(RenderOptions option);
    void disableOption(RenderOptions option);
    void toggleOption(RenderOptions option);
    void toggleOption(RenderOptions option, const bool state);
    
    PROPERTY_RW(F32, generalVisibility, 1000.0f);
    PROPERTY_RW(F32, grassVisibility, 1000.0f);
    PROPERTY_RW(F32, treeVisibility, 1000.0f);
    PROPERTY_RW(U8, renderPass, 0u);

    inline vec4<U16>& lodThresholds() noexcept { return _lodThresholds; }
    inline FogDescriptor& fogDescriptor() noexcept { return _fog; }
    inline const FogDescriptor& fogDescriptor() const noexcept { return _fog; }

    vec4<U16> lodThresholds(RenderStage stage = RenderStage::DISPLAY) const noexcept;

  protected:
    FogDescriptor _fog;
    vec4<U16> _lodThresholds;
    U16 _stateMask;
};

class Camera;

enum class MoveDirection : I8 {
    NONE = 0,
    NEGATIVE = -1,
    POSITIVE = 1
};

constexpr F32 DEFAULT_PLAYER_HEIGHT = 1.82f;

struct SceneStatePerPlayer {
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

    PROPERTY_RW(bool, cameraUnderwater, false);
    PROPERTY_RW(bool, cameraUpdated, false);
    PROPERTY_RW(bool, cameraLockedToMouse, false);
    PROPERTY_RW(MoveDirection, moveFB, MoveDirection::NONE);   ///< forward-back move change detected
    PROPERTY_RW(MoveDirection, moveLR, MoveDirection::NONE);   ///< left-right move change detected
    PROPERTY_RW(MoveDirection, moveUD, MoveDirection::NONE);   ///< up-down move change detected
    PROPERTY_RW(MoveDirection, angleUD, MoveDirection::NONE);  ///< up-down angle change detected
    PROPERTY_RW(MoveDirection, angleLR, MoveDirection::NONE);  ///< left-right angle change detected
    PROPERTY_RW(MoveDirection, roll, MoveDirection::NONE);     ///< roll left or right change detected
    PROPERTY_RW(MoveDirection, zoom, MoveDirection::NONE);     ///< zoom in or out detected
    POINTER_RW(Camera, overrideCamera, nullptr);
    const F32 _headHeight = DEFAULT_PLAYER_HEIGHT;

};

struct WaterDetails {
    F32 _heightOffset = 0.0f;
    F32 _depth = 0.0f;
    vec3<F32> _normal = WORLD_Y_AXIS;
};

class SceneState : public SceneComponent {
   public:
    /// Background music map : trackName - track
    using MusicPlaylist = hashMap<U64, AudioDescriptor_ptr>;

    SceneState(Scene& parentScene)
        : SceneComponent(parentScene),
          _renderState(parentScene)
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

    inline vectorEASTL<WaterDetails>& globalWaterBodies() noexcept { return _globalWaterBodies; }
    inline const vectorEASTL<WaterDetails>& globalWaterBodies() const noexcept { return _globalWaterBodies; }

    PROPERTY_RW(U8, playerPass, 0u);
    PROPERTY_RW(bool, saveLoadDisabled, false);
    PROPERTY_RW(F32, windSpeed, 1.0f);
    PROPERTY_RW(F32, windDirX, 0.1f);
    PROPERTY_RW(F32, windDirZ, 0.7f);
    PROPERTY_RW(F32, lightBleedBias, 0.2f);
    PROPERTY_RW(F32, minShadowVariance, 0.001f);
    PROPERTY_RW(U16, shadowFadeDistance, 900u);
    PROPERTY_RW(U16, shadowDistance, 1000u);

protected:
    SceneRenderState _renderState;
    std::array<MusicPlaylist, to_base(MusicType::COUNT)> _music;
    std::array<SceneStatePerPlayer, Config::MAX_LOCAL_PLAYER_COUNT> _playerState;
    vectorEASTL<WaterDetails> _globalWaterBodies;
};

};  // namespace Divide
#endif