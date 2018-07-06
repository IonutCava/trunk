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
    _gizmoState = GizmoState::NO_GIZMO;
    _objectState = ObjectRenderState::DRAW_OBJECT;
    _cameraMgr = &Application::getInstance().getKernel().getCameraMgr();
}

void SceneRenderState::toggleBoundingBoxes() {
    Console::d_printfn(Locale::get("TOGGLE_SCENE_BOUNDING_BOXES"));
    if (objectState() == ObjectRenderState::NO_DRAW) {
        objectState(ObjectRenderState::DRAW_OBJECT);
    } else if (objectState() == ObjectRenderState::DRAW_OBJECT) {
        objectState(ObjectRenderState::DRAW_OBJECT_WITH_BOUNDING_BOX);
    } else if (objectState() ==
               ObjectRenderState::DRAW_OBJECT_WITH_BOUNDING_BOX) {
        objectState(ObjectRenderState::DRAW_BOUNDING_BOX);
    } else {
        objectState(ObjectRenderState::NO_DRAW);
    }
}

/// Show/hide axis gizmos
void SceneRenderState::toggleAxisLines() {
    Console::d_printfn(Locale::get("TOGGLE_SCENE_AXIS_GIZMO"));
    if (gizmoState() == GizmoState::NO_GIZMO) {
        gizmoState(GizmoState::SELECTED_GIZMO);
    } else if (gizmoState() == GizmoState::SELECTED_GIZMO) {
        gizmoState(GizmoState::ALL_GIZMO);
    } else if (gizmoState() == GizmoState::ALL_GIZMO) {
        gizmoState(GizmoState::SCENE_GIZMO);
    } else {
        gizmoState(GizmoState::NO_GIZMO);
    }
}

};  // namespace Divide