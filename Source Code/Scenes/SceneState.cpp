#include "Headers/SceneState.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"

namespace Divide {

SceneRenderState::SceneRenderState()
    : _drawBB(false),
      _drawSkeletons(false),
      _drawObjects(true),
      _debugDrawLines(false),
      _debugDrawTargetLines(false)

{
    _gizmoState = NO_GIZMO;
    _objectState = DRAW_OBJECT;
    _cameraMgr = &Application::getInstance().getKernel().getCameraMgr();
}

void SceneRenderState::toggleBoundingBoxes() {
    Console::d_printfn(Locale::get("TOGGLE_SCENE_BOUNDING_BOXES"));
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
void SceneRenderState::toggleAxisLines() {
    Console::d_printfn(Locale::get("TOGGLE_SCENE_AXIS_GIZMO"));
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

};  // namespace Divide