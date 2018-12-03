#include "stdafx.h"

#include "Headers/SceneState.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"

#include "Utility/Headers/Colours.h"

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
      _stateMask(to_base(RenderOptions::PLAY_ANIMATIONS)),
      _renderPass(0),
      _grassVisibility(1.0f),
      _treeVisibility(1.0f),
      _generalVisibility(1.0f)
{
    enableOption(RenderOptions::RENDER_GEOMETRY);

    _gizmoState = GizmoState::NO_GIZMO;
}

void SceneRenderState::toggleAxisLines() {
    static U32 selection = 0;
    Console::d_printfn(Locale::get(_ID("TOGGLE_SCENE_AXIS_GIZMO")));
    selection = (selection + 1) % to_base(GizmoState::COUNT);
    switch (selection) {
    case 0:
        gizmoState(GizmoState::SELECTED_GIZMO);
        break;
    case 1:
        gizmoState(GizmoState::ALL_GIZMO);
        break;
    case 2:
        gizmoState(GizmoState::SCENE_GIZMO);
        break;
    case 3:
        gizmoState(GizmoState::NO_GIZMO);
        break;
    }
}

void SceneRenderState::renderMask(U32 mask) {
    constexpr bool validateRenderMask = false;

    if (validateRenderMask) {
        auto validateMask = [mask]() -> U32 {
            U32 validMask = 0;
            for (U32 stateIt = 1; stateIt <= to_base(RenderOptions::COUNT); ++stateIt) {
                U32 bitState = toBit(stateIt);

                if (BitCompare(mask, bitState)) {
                    DIVIDE_ASSERT(static_cast<RenderOptions>(bitState) != RenderOptions::PLAY_ANIMATIONS,
                                  "SceneRenderState::renderMask error: can't update animation state directly!");
                    SetBit(validMask, bitState);
                }
            }
            return validMask;
        };

        U32 parsedMask = validateMask();
        DIVIDE_ASSERT(parsedMask != 0 && parsedMask == mask,
                      "SceneRenderState::renderMask error: Invalid state specified!");
        _stateMask = parsedMask;
    } else {
        _stateMask = mask;
    }
}

bool SceneRenderState::isEnabledOption(RenderOptions option) const {
    return BitCompare(_stateMask, to_U32(option));
}

void SceneRenderState::enableOption(RenderOptions option) {
    if (Config::Build::IS_DEBUG_BUILD) {
        DIVIDE_ASSERT(option != RenderOptions::PLAY_ANIMATIONS,
                      "SceneRenderState::enableOption error: can't update animation state directly!");

        if (option == RenderOptions::RENDER_SKELETONS) {
            Console::d_printfn(Locale::get(_ID("TOGGLE_SCENE_SKELETONS")), "On");
        } else if (option == RenderOptions::RENDER_AABB) {
            Console::d_printfn(Locale::get(_ID("TOGGLE_SCENE_BOUNDING_BOXES")), "On");
        }
    }

    SetBit(_stateMask, to_U32(option));
}

void SceneRenderState::disableOption(RenderOptions option) {
    if (Config::Build::IS_DEBUG_BUILD) {
        DIVIDE_ASSERT(option != RenderOptions::PLAY_ANIMATIONS,
                      "SceneRenderState::disableOption error: can't update animation state directly!");

        if (option == RenderOptions::RENDER_SKELETONS) {
            Console::d_printfn(Locale::get(_ID("TOGGLE_SCENE_SKELETONS")), "Off");
        } else if (option == RenderOptions::RENDER_AABB) {
            Console::d_printfn(Locale::get(_ID("TOGGLE_SCENE_BOUNDING_BOXES")), "Off");
        }
    }

    ClearBit(_stateMask, to_U32(option));
}

void SceneRenderState::toggleOption(RenderOptions option) {
    toggleOption(option, !isEnabledOption(option));
}

void SceneRenderState::toggleOption(RenderOptions option, const bool state) {
    DIVIDE_ASSERT(option != RenderOptions::PLAY_ANIMATIONS,
                  "SceneRenderState::toggleOption error: can't update animation state directly!");

    if (state) {
        enableOption(option);
    } else {
        disableOption(option);
    }
}

};  // namespace Divide