#include "stdafx.h"

#include "Headers/SceneState.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"

#include "Utility/Headers/Colours.h"

#include "Platform/Video/Headers/RenderStagePass.h"

namespace Divide {

FogDescriptor::FogDescriptor()
   : _dirty(true),
     _active(true),
     _density(0.0f),
     _colour(DefaultColours::WHITE.rgb())
{
}

SceneRenderState::SceneRenderState(Scene& parentScene)
    : SceneComponent(parentScene),
      _stateMask(0u),
      _renderPass(0),
      _grassVisibility(1.0f),
      _treeVisibility(1.0f),
      _generalVisibility(1.0f)
{
    enableOption(RenderOptions::RENDER_GEOMETRY);
    enableOption(RenderOptions::PLAY_ANIMATIONS);

    _lodThresholds.set(25, 45, 85, 165);
    _fog.set(vec3<F32>(0.2f, 0.2f, 0.2f), 0.01f);
}

void SceneRenderState::renderMask(U16 mask) {
    constexpr bool validateRenderMask = false;

    if (validateRenderMask) {
        auto validateMask = [mask]() -> U16 {
            U16 validMask = 0;
            for (U16 stateIt = 1; stateIt <= to_base(RenderOptions::COUNT); ++stateIt) {
                U16 bitState = toBit(stateIt);

                if (BitCompare(mask, bitState)) {
                    DIVIDE_ASSERT(static_cast<RenderOptions>(bitState) != RenderOptions::PLAY_ANIMATIONS,
                                  "SceneRenderState::renderMask error: can't update animation state directly!");
                    SetBit(validMask, bitState);
                }
            }
            return validMask;
        };

        U16 parsedMask = validateMask();
        DIVIDE_ASSERT(parsedMask != 0 && parsedMask == mask,
                      "SceneRenderState::renderMask error: Invalid state specified!");
        _stateMask = parsedMask;
    } else {
        _stateMask = mask;
    }
}

bool SceneRenderState::isEnabledOption(RenderOptions option) const {
    return BitCompare(_stateMask, option);
}

void SceneRenderState::enableOption(RenderOptions option) {
    SetBit(_stateMask, option);
}

void SceneRenderState::disableOption(RenderOptions option) {
    ClearBit(_stateMask, option);
}

void SceneRenderState::toggleOption(RenderOptions option) {
    toggleOption(option, !isEnabledOption(option));
}

void SceneRenderState::toggleOption(RenderOptions option, const bool state) {
    if (state) {
        enableOption(option);
    } else {
        disableOption(option);
    }
}

vec4<U16> SceneRenderState::lodThresholds(RenderStage stage) const noexcept {
    // Hack dumping ground. Scene specific lod management can be tweaked here to keep the components clean
    if (stage == RenderStage::REFLECTION || stage == RenderStage::REFRACTION) {
        // cancel out LoD Level 0
        return {
            0u,
            _lodThresholds.y,
            _lodThresholds.z,
            _lodThresholds.w };
    } else if (stage == RenderStage::SHADOW) {
        return _lodThresholds * 3u;
    }

    return _lodThresholds;
}
};  // namespace Divide