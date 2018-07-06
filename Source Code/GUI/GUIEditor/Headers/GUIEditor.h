/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _GUI_EDITOR_H_
#define _GUI_EDITOR_H_

#include "Platform/Headers/PlatformDefines.h"
#include <assert.h>

namespace CEGUI {
class Font;
class Window;
class Editbox;
class EventArgs;
class ToggleButton;
};

namespace Divide {

class SceneGraphNode;
typedef std::weak_ptr<SceneGraphNode> SceneGraphNode_wptr;
/// Our world editor interface
DEFINE_SINGLETON(GUIEditor)
  public:
    bool init();
    void setVisible(bool visible);  //< Hide or show the editor
    bool isVisible();  //< Return true if editor is visible, false if is hidden
    bool update(const U64 deltaTime);  //< Used to update time dependent elements
    bool Handle_ChangeSelection(SceneGraphNode_wptr newNode);

    /// Returns true if the last click was in one of the editor's windows
    inline bool wasControlClick() { return _wasControlClick; }

  private:
    GUIEditor();
    ~GUIEditor();
    void RegisterHandlers();
    void UpdateControls();
    void TrackSelection();

    bool Handle_MenuBarClickOn(const CEGUI::EventArgs &e);
    bool Handle_EditFieldClick(const CEGUI::EventArgs &e);
    bool Handle_CreateNavMesh(const CEGUI::EventArgs &e);
    bool Handle_SaveScene(const CEGUI::EventArgs &e);
    bool Handle_SaveSelection(const CEGUI::EventArgs &e);
    bool Handle_DeleteSelection(const CEGUI::EventArgs &e);
    bool Handle_ReloadScene(const CEGUI::EventArgs &e);
    bool Handle_WireframeToggle(const CEGUI::EventArgs &e);
    bool Handle_DepthPreviewToggle(const CEGUI::EventArgs &e);
    bool Handle_ShadowMappingToggle(const CEGUI::EventArgs &e);
    bool Handle_FogToggle(const CEGUI::EventArgs &e);
    bool Handle_PostFXToggle(const CEGUI::EventArgs &e);
    bool Handle_BoundingBoxesToggle(const CEGUI::EventArgs &e);
    bool Handle_DrawNavMeshToggle(const CEGUI::EventArgs &e);
    bool Handle_SkeletonsToggle(const CEGUI::EventArgs &e);
    bool Handle_PositionXChange(const CEGUI::EventArgs &e);
    bool Handle_PositionYChange(const CEGUI::EventArgs &e);
    bool Handle_PositionZChange(const CEGUI::EventArgs &e);
    bool Handle_PositionGranularityChange(const CEGUI::EventArgs &e);
    bool Handle_RotationXChange(const CEGUI::EventArgs &e);
    bool Handle_RotationYChange(const CEGUI::EventArgs &e);
    bool Handle_RotationZChange(const CEGUI::EventArgs &e);
    bool Handle_RotationGranularityChange(const CEGUI::EventArgs &e);
    bool Handle_ScaleXChange(const CEGUI::EventArgs &e);
    bool Handle_ScaleYChange(const CEGUI::EventArgs &e);
    bool Handle_ScaleZChange(const CEGUI::EventArgs &e);
    bool Handle_ScaleGranularityChange(const CEGUI::EventArgs &e);
    bool Handle_IncrementPositionX(const CEGUI::EventArgs &e);
    bool Handle_DecrementPositionX(const CEGUI::EventArgs &e);
    bool Handle_IncrementPositionY(const CEGUI::EventArgs &e);
    bool Handle_DecrementPositionY(const CEGUI::EventArgs &e);
    bool Handle_IncrementPositionZ(const CEGUI::EventArgs &e);
    bool Handle_DecrementPositionZ(const CEGUI::EventArgs &e);
    bool Handle_IncrementPositionGranularity(const CEGUI::EventArgs &e);
    bool Handle_DecrementPositionGranularity(const CEGUI::EventArgs &e);
    bool Handle_IncrementRotationX(const CEGUI::EventArgs &e);
    bool Handle_DecrementRotationX(const CEGUI::EventArgs &e);
    bool Handle_IncrementRotationY(const CEGUI::EventArgs &e);
    bool Handle_DecrementRotationY(const CEGUI::EventArgs &e);
    bool Handle_IncrementRotationZ(const CEGUI::EventArgs &e);
    bool Handle_DecrementRotationZ(const CEGUI::EventArgs &e);
    bool Handle_IncrementRotationGranularity(const CEGUI::EventArgs &e);
    bool Handle_DecrementRotationGranularity(const CEGUI::EventArgs &e);
    bool Handle_IncrementScaleX(const CEGUI::EventArgs &e);
    bool Handle_DecrementScaleX(const CEGUI::EventArgs &e);
    bool Handle_IncrementScaleY(const CEGUI::EventArgs &e);
    bool Handle_DecrementScaleY(const CEGUI::EventArgs &e);
    bool Handle_IncrementScaleZ(const CEGUI::EventArgs &e);
    bool Handle_DecrementScaleZ(const CEGUI::EventArgs &e);
    bool Handle_IncrementScaleGranularity(const CEGUI::EventArgs &e);
    bool Handle_DecrementScaleGranularity(const CEGUI::EventArgs &e);

private:
    enum class ToggleButtons : U32 {
        TOGGLE_WIREFRAME = 0,
        TOGGLE_DEPTH_PREVIEW = 1,
        TOGGLE_SHADOW_MAPPING = 2,
        TOGGLE_FOG = 3,
        TOGGLE_POST_FX = 4,
        TOGGLE_BOUNDING_BOXES = 5,
        TOGGLE_NAV_MESH_DRAW = 6,
        TOGGLE_SKELETONS = 7,
        COUNT
    };
    enum class ControlFields : U32 {
        CONTROL_FIELD_X = 0,
        CONTROL_FIELD_Y = 1,
        CONTROL_FIELD_Z = 2,
        CONTROL_FIELD_GRANULARITY = 3,
        COUNT
    };
    enum class TransformFields : U32 {
        TRANSFORM_POSITION = 0,
        TRANSFORM_ROTATION = 1,
        TRANSFORM_SCALE = 2,
        COUNT
    };

    F32& currentValues(TransformFields transform, ControlFields control) {
        assert(transform < TransformFields::COUNT &&
                control < ControlFields::COUNT);
        return _currentValues[to_uint(transform)][to_uint(control)];
    }

    CEGUI::Editbox*& valuesField(TransformFields transform, ControlFields control) {
        assert(transform < TransformFields::COUNT &&
                control < ControlFields::COUNT);
        return _valuesField[to_uint(transform)][to_uint(control)];
    }

    CEGUI::ToggleButton*& toggleButton(ToggleButtons button) {
        assert(button < ToggleButtons::COUNT);
        return _toggleButtons[to_uint(button)];
    }

    CEGUI::Window*& transformButtonsInc(TransformFields transform, ControlFields control) {
           assert(transform < TransformFields::COUNT &&
                  control < ControlFields::COUNT);
        return _transformButtonsInc[to_uint(transform)][to_uint(control)];
    }

     CEGUI::Window*& transformButtonsDec(TransformFields transform, ControlFields control) {
           assert(transform < TransformFields::COUNT &&
                  control < ControlFields::COUNT);
        return _transformButtonsDec[to_uint(transform)][to_uint(control)];
    }
  private:
    bool _init;
    bool _wasControlClick;
    bool _createNavMeshQueued;
    bool _pauseSelectionTracking;
    SceneGraphNode_wptr _currentSelection;
    CEGUI::Window *
        _editorWindow;  //< This will be a pointer to the EditorRoot window.
    CEGUI::Window *_saveSelectionButton;
    CEGUI::Window *_deleteSelectionButton;
    std::array<CEGUI::ToggleButton *, to_const_uint(ToggleButtons::COUNT)>
        _toggleButtons;
    std::array<
        std::array<CEGUI::Window *, to_const_uint(ControlFields::COUNT)>,
        to_const_uint(TransformFields::COUNT)> _transformButtonsInc;
    std::array<
        std::array<CEGUI::Window *, to_const_uint(ControlFields::COUNT)>,
        to_const_uint(TransformFields::COUNT)> _transformButtonsDec;
    std::array<
        std::array<CEGUI::Editbox *, to_const_uint(ControlFields::COUNT)>,
        to_const_uint(TransformFields::COUNT)> _valuesField;
    std::array<std::array<F32, to_const_uint(ControlFields::COUNT)>,
               to_const_uint(TransformFields::COUNT)> _currentValues;

END_SINGLETON

};  // namespace Divide
#endif
