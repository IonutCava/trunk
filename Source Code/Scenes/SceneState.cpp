#include "Headers/SceneState.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"

namespace Divide {

SceneRenderState::SceneRenderState()
    : _drawBB(false),
      _drawGeometry(true),
      _drawSkeletons(false),
      _drawBoundingBoxes(false),
      _drawWireframe(false),
      _drawOctreeRegions(false),
      _debugDrawLines(false),
      _debugDrawTargetLines(false),
      _playAnimations(true)

{
    _gizmoState = GizmoState::NO_GIZMO;
    _cameraMgr = &Application::getInstance().kernel().getCameraMgr();
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
    selection = ++selection % 4;
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