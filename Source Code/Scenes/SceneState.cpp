#include "Headers/SceneState.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"

#include "Utility/Headers/Colours.h"

namespace Divide {

FogDescriptor::FogDescriptor()
    : _dirty(true),
     _active(true),
     _density(0.0f),
     _colour(DefaultColours::WHITE().rgb())
{
}

SceneRenderState::SceneRenderState(Scene& parentScene)
    : SceneComponent(parentScene),
      _drawBB(false),
      _drawGeometry(true),
      _drawSkeletons(false),
      _drawBoundingBoxes(false),
      _drawWireframe(false),
      _drawOctreeRegions(false),
      _debugDrawLines(false),
      _debugDrawTargetLines(false),
      _playAnimations(true),
      _currentStagePass(-1)
{
    _gizmoState = GizmoState::NO_GIZMO;
    _cameraMgr = &Application::instance().kernel().getCameraMgr();
}

void SceneRenderState::toggleSkeletons() {
    Console::d_printfn(Locale::get(_ID("TOGGLE_SCENE_SKELETONS")));
    drawSkeletons(!drawSkeletons());
}

void SceneRenderState::toggleBoundingBoxes() {
    Console::d_printfn(Locale::get(_ID("TOGGLE_SCENE_BOUNDING_BOXES")));
    drawBoundingBoxes(!drawBoundingBoxes());
}

void SceneRenderState::toggleAxisLines() {
    static U32 selection = 0;
    Console::d_printfn(Locale::get(_ID("TOGGLE_SCENE_AXIS_GIZMO")));
    selection = (selection + 1) % to_const_uint(GizmoState::COUNT);
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

void SceneRenderState::toggleWireframe() {
    drawWireframe(!drawWireframe());
}

void SceneRenderState::toggleDebugLines() {
    drawDebugLines(!drawDebugLines());
}

void SceneRenderState::toggleGeometry() {
    drawGeometry(!drawGeometry());
}

};  // namespace Divide