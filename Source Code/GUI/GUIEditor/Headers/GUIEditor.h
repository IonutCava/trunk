/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _GUI_EDITOR_H_
#define _GUI_EDITOR_H_

namespace CEGUI {
    class Font;
    class Window;
    class Editbox;
    class EventArgs;
    class ToggleButton;
};

#include "GUIEditorInterface.h"
#include "Core/Headers/Singleton.h"

///Our world editor interface
DEFINE_SINGLETON( GUIEditor )
    public:
        bool init();
        void setVisible(bool visible); //< Hide or show the editor
        bool isVisible();              //< Return true if editor is visible, false if is hidden
        bool update(const U64 deltaTime);    //< Used to update time dependent elements

    private:
        GUIEditor();
        ~GUIEditor();
        void RegisterHandlers();   

        bool Handle_CreateNavMesh(const CEGUI::EventArgs &e);
        bool Handle_SaveScene(const CEGUI::EventArgs &e);
        bool Handle_SaveSelection(const CEGUI::EventArgs &e);
        bool Handle_DeleteSelection(const CEGUI::EventArgs &e);
        bool Handle_ReloadScene(const CEGUI::EventArgs &e);
        bool Handle_PrintSceneGraph(const CEGUI::EventArgs &e);
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
        enum ToggleButtons {
            TOGGLE_WIREFRAME = 0,
            TOGGLE_DEPTH_PREVIEW = 1,
            TOGGLE_SHADOW_MAPPING = 2,
            TOGGLE_FOG = 3,
            TOGGLE_POST_FX = 4,
            TOGGLE_BOUNDING_BOXES = 5,
            TOGGLE_NAV_MESH_DRAW = 6,
            TOGGLE_SKELETONS = 7,
            ToggleButtons_PLACEHOLDER = 8
        };
        enum ControlField {
            CONTROL_FIELD_X = 0,
            CONTROL_FIELD_Y = 1,
            CONTROL_FIELD_Z = 2,
            CONTROL_FIELD_GRANULARITY = 3,
            ControlField_PLACEHOLDER = 4
        };

        bool _init;
        bool _createNavMeshQueued;
        CEGUI::Window *_editorWindow;  //< This will be a pointer to the EditorRoot window.
        CEGUI::ToggleButton *_toggleButtons[ToggleButtons_PLACEHOLDER];
        CEGUI::Editbox      *_positionValuesField[ControlField_PLACEHOLDER];
        CEGUI::Editbox      *_rotationValuesField[ControlField_PLACEHOLDER];
        CEGUI::Editbox      *_scaleValuesField[ControlField_PLACEHOLDER];
        F32 _currentPositionValues[ControlField_PLACEHOLDER];
        F32 _currentRotationValues[ControlField_PLACEHOLDER];   
        F32 _currentScaleValues[ControlField_PLACEHOLDER];

END_SINGLETON

#endif