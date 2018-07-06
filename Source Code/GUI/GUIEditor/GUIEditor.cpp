#include <CEGUI/CEGUI.h>
#include "Headers/GUIEditor.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIConsole.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/AIManager.h"
#include "Managers/Headers/SceneManager.h"

GUIEditor::GUIEditor() : _init(false), 
                         _wasControlClick(false),
                         _createNavMeshQueued(false),
                         _pauseSelectionTracking(false),
                         _editorWindow(nullptr),
                         _currentSelection(nullptr)
{
    _saveSelectionButton = nullptr;
    _deleteSelectionButton = nullptr;

    for (U8 i = 0; i < ToggleButtons_PLACEHOLDER; ++i) {
        _toggleButtons[i] = nullptr;
    }

    for (U8 i = 0; i < TransformFields_PLACEHOLDER; ++i) {
        for (U8 j = 0; j < ControlField_PLACEHOLDER; ++j) {
            _currentValues[i][j] = 0.0;
            _valuesField[i][j] = nullptr;
            _transformButtonsInc[i][j] = nullptr;
            _transformButtonsDec[i][j] = nullptr;
        }
        _currentValues[i][CONTROL_FIELD_GRANULARITY] = 1.0;
    }
}

GUIEditor::~GUIEditor()
{
    _init = false;
}

bool GUIEditor::init() {
    if (_init) {
        ERROR_FN(Locale::get("ERROR_EDITOR_DOUBLE_INIT"));
        return false;
    }

    PRINT_FN(Locale::get("GUI_EDITOR_INIT"));
    // Get a local pointer to the CEGUI Window Manager, Purely for convenience to reduce typing
    CEGUI::WindowManager *pWindowManager = CEGUI::WindowManager::getSingletonPtr();
    // load the editor Window from the layout file
    std::string layoutFile(ParamHandler::getInstance().getParam<std::string>("GUI.editorLayout"));

    _editorWindow = pWindowManager->loadLayoutFromFile(layoutFile);

    if (_editorWindow) {
         // Add the Window to the GUI Root Sheet
         CEGUI_DEFAULT_CONTEXT.getRootWindow()->addChild(_editorWindow);
         // Now register the handlers for the events (Clicking, typing, etc)
         RegisterHandlers();
    } else {
         // Loading layout from file, failed, so output an error Message.
         CEGUI::Logger::getSingleton().logEvent("Error: Unable to load the Editor from .layout");
         ERROR_FN(Locale::get("ERROR_EDITOR_LAYOUT_FILE"),layoutFile.c_str());
         return false;
    }

    setVisible(false);

    _init = true;
    PRINT_FN(Locale::get("GUI_EDITOR_CREATED"));
    return true;
}

bool GUIEditor::Handle_ChangeSelection(SceneGraphNode* const newNode) {
    _currentSelection = newNode;
    if (isVisible()) {
        UpdateControls();
    }
    return true;
}

void GUIEditor::TrackSelection() {
    if (_currentSelection && !_pauseSelectionTracking) {
        const Transform* selectionTransform = _currentSelection->getComponent<PhysicsComponent>()->getConstTransform();
        const vec3<F32>& localPosition = selectionTransform->getLocalPosition();
        const vec3<F32>& localScale = selectionTransform->getLocalScale();
        vec3<F32> localOrientation;
        selectionTransform->getLocalOrientation().getEuler(&localOrientation, true);

        _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_X] = localPosition.x;
        _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Y] = localPosition.y;
        _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Z] = localPosition.z;

        _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X] = localOrientation.x;
        _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y] = localOrientation.y;
        _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z] = localOrientation.z;

        _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_X] = localScale.x;
        _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Y] = localScale.y;
        _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Z] = localScale.z;

        for (U8 i = 0; i < TransformFields_PLACEHOLDER; ++i) {
            // Skip granularity
            for (U8 j = 0; j < ControlField_PLACEHOLDER - 1; ++j) {
                _valuesField[i][j]->setText(CEGUI::PropertyHelper<F32>::toString(_currentValues[i][j]));
            }
        }
        _toggleButtons[TOGGLE_SKELETONS]->setSelected(_currentSelection->renderSkeleton());
        _toggleButtons[TOGGLE_SHADOW_MAPPING]->setSelected(_currentSelection->castsShadows());
        _toggleButtons[TOGGLE_WIREFRAME]->setSelected(_currentSelection->renderWireframe());
        _toggleButtons[TOGGLE_BOUNDING_BOXES]->setSelected(_currentSelection->renderBoundingBox());
    }
}

void GUIEditor::UpdateControls() {
    bool hasValidTransform = false;
    if (_currentSelection) {
        hasValidTransform = (_currentSelection->getComponent<PhysicsComponent>()->getConstTransform() != nullptr);
        _toggleButtons[TOGGLE_WIREFRAME]->setEnabled(true);
        _toggleButtons[TOGGLE_BOUNDING_BOXES]->setEnabled(true);
        _toggleButtons[TOGGLE_SKELETONS]->setEnabled(true);
        _toggleButtons[TOGGLE_SHADOW_MAPPING]->setEnabled(LightManager::getInstance().shadowMappingEnabled());
    }

    if (!hasValidTransform) {
        for (U8 i = 0; i < TransformFields_PLACEHOLDER; ++i) {
            // Skip granularity
            for (U8 j = 0; j < ControlField_PLACEHOLDER - 1; ++j) {
                _valuesField[i][j]->setText("N/A");
                _valuesField[i][j]->setEnabled(false);
                _transformButtonsInc[i][j]->setEnabled(false);
                _transformButtonsDec[i][j]->setEnabled(false);
            }
        }
        _deleteSelectionButton->setEnabled(false);
        _saveSelectionButton->setEnabled(false);
        _toggleButtons[TOGGLE_WIREFRAME]->setEnabled(false);
        _toggleButtons[TOGGLE_WIREFRAME]->setSelected(false);
        _toggleButtons[TOGGLE_BOUNDING_BOXES]->setEnabled(false);
        _toggleButtons[TOGGLE_BOUNDING_BOXES]->setSelected(false);
        _toggleButtons[TOGGLE_SKELETONS]->setEnabled(false);
        _toggleButtons[TOGGLE_SKELETONS]->setSelected(false);
        _toggleButtons[TOGGLE_SHADOW_MAPPING]->setEnabled(false);
        _toggleButtons[TOGGLE_SHADOW_MAPPING]->setSelected(false);
    } else {
         for (U8 i = 0; i < TransformFields_PLACEHOLDER; ++i) {
            // Skip granularity
            for (U8 j = 0; j < ControlField_PLACEHOLDER - 1; ++j) {
                _valuesField[i][j]->setText(CEGUI::PropertyHelper<F32>::toString(_currentValues[i][j]));
                _valuesField[i][j]->setEnabled(true);
                _transformButtonsInc[i][j]->setEnabled(true);
                _transformButtonsDec[i][j]->setEnabled(true);
            }
        }
        _deleteSelectionButton->setEnabled(true);
        _saveSelectionButton->setEnabled(true);
    }
    _toggleButtons[TOGGLE_POST_FX]->setSelected(GFX_DEVICE.postProcessingEnabled());
    _toggleButtons[TOGGLE_FOG]->setSelected(ParamHandler::getInstance().getParam<bool>("rendering.enableFog", false));
    _toggleButtons[TOGGLE_DEPTH_PREVIEW]->setSelected(ParamHandler::getInstance().getParam<bool>("postProcessing.fullScreenDepthBuffer", false));
}

void GUIEditor::setVisible(bool visible) {
    UpdateControls();
     _editorWindow->setVisible(visible);
}

bool GUIEditor::isVisible() {
    return _editorWindow->isVisible();
}

bool GUIEditor::update(const U64 deltaTime) {
    _wasControlClick = false;
    bool state = true;
  	if (_createNavMeshQueued) {
        state = false;
        // Check if we already have a NavMesh created
        Navigation::NavigationMesh* temp = AIManager::getInstance().getNavMesh(AIEntity::AGENT_RADIUS_SMALL);
        // Check debug rendering status
		AIManager::getInstance().toggleNavMeshDebugDraw(_toggleButtons[TOGGLE_NAV_MESH_DRAW]->isSelected());
        // Create a new NavMesh if we don't currently have one
        if (!temp) {
		    temp = New Navigation::NavigationMesh();
        }
        // Set it's file name
		temp->setFileName(GET_ACTIVE_SCENE()->getName());
        // Try to load it from file
		bool loaded = temp->load(nullptr);
		if (!loaded) {
            // If we failed to load it from file, we need to build it first
			loaded = temp->build(nullptr, false);
            // Then save it to file
			temp->save();
		}
        // If we loaded/built the NavMesh correctly, add it to the AIManager
		if (loaded) {
			state = AIManager::getInstance().addNavMesh(AIEntity::AGENT_RADIUS_SMALL, temp);
		}

		_createNavMeshQueued = false;
	}

    if (isVisible()) {
        TrackSelection();
    }
	return state;
}

void GUIEditor::RegisterHandlers() {
    CEGUI::Window* EditorBar = static_cast<CEGUI::Window*>(_editorWindow->getChild("MenuBar"));
    EditorBar->subscribeEvent(CEGUI::Window::EventMouseButtonUp, CEGUI::Event::Subscriber(&GUIEditor::Handle_MenuBarClickOn, this));

    for (U16 i = 0; i < EditorBar->getChildCount(); ++i) { 
        EditorBar->getChildElementAtIdx(i)->subscribeEvent(CEGUI::Window::EventMouseButtonUp, CEGUI::Event::Subscriber(&GUIEditor::Handle_MenuBarClickOn, this));
    }

    // Save Scene Button
    {
        CEGUI::Window* saveSceneButton = static_cast<CEGUI::Window*>(EditorBar->getChild("SaveSceneButton"));
        saveSceneButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_SaveScene, this));
    }
    // Save Selection Button
    { 
        _saveSelectionButton = static_cast<CEGUI::Window*>(EditorBar->getChild("SaveSelectionButton"));
        _saveSelectionButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_SaveSelection, this));
    }
    // Delete Selection Button
    {
        _deleteSelectionButton = static_cast<CEGUI::Window*>(EditorBar->getChild("DeleteSelection"));
        _deleteSelectionButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DeleteSelection, this));
    }
    // Generate NavMesh button
    {
        CEGUI::Window* createNavMeshButton = static_cast<CEGUI::Window*>(EditorBar->getChild("NavMeshButton"));
        createNavMeshButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_CreateNavMesh, this));
    }
    // Reload Scene Button
    {
        CEGUI::Window* reloadSceneButton = static_cast<CEGUI::Window*>(EditorBar->getChild("ReloadSceneButton"));
        reloadSceneButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_ReloadScene, this));
    }
    // Print SceneGraph Button
    {
        CEGUI::Window* dumpSceneGraphButton = static_cast<CEGUI::Window*>(EditorBar->getChild("DumpSceneGraphTree"));
        dumpSceneGraphButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_PrintSceneGraph, this));
    }
    // Toggle Wireframe rendering
    {
        _toggleButtons[TOGGLE_WIREFRAME] = static_cast<CEGUI::ToggleButton*>(EditorBar->getChild("ToggleWireframe"));
        _toggleButtons[TOGGLE_WIREFRAME]->subscribeEvent(CEGUI::ToggleButton::EventSelectStateChanged, CEGUI::Event::Subscriber(&GUIEditor::Handle_WireframeToggle, this));
    }
    // Toggle Depth Preview
    {
        _toggleButtons[TOGGLE_DEPTH_PREVIEW]= static_cast<CEGUI::ToggleButton*>(EditorBar->getChild("ToggleDepthPreview"));
        _toggleButtons[TOGGLE_DEPTH_PREVIEW]->subscribeEvent(CEGUI::ToggleButton::EventSelectStateChanged, CEGUI::Event::Subscriber(&GUIEditor::Handle_DepthPreviewToggle, this));
    }
    // Toggle Shadows
    {
        _toggleButtons[TOGGLE_SHADOW_MAPPING] = static_cast<CEGUI::ToggleButton*>(EditorBar->getChild("ToggleShadowMapping"));
        _toggleButtons[TOGGLE_SHADOW_MAPPING]->subscribeEvent(CEGUI::ToggleButton::EventSelectStateChanged, CEGUI::Event::Subscriber(&GUIEditor::Handle_ShadowMappingToggle, this));
    }
    // Toggle Fog
    {
        _toggleButtons[TOGGLE_FOG] = static_cast<CEGUI::ToggleButton*>(EditorBar->getChild("ToggleFog"));
        _toggleButtons[TOGGLE_FOG]->subscribeEvent(CEGUI::ToggleButton::EventSelectStateChanged, CEGUI::Event::Subscriber(&GUIEditor::Handle_FogToggle, this));
    }
    // Toggle PostFX
    {
        _toggleButtons[TOGGLE_POST_FX] = static_cast<CEGUI::ToggleButton*>(EditorBar->getChild("TogglePostFX"));
        _toggleButtons[TOGGLE_POST_FX]->subscribeEvent(CEGUI::ToggleButton::EventSelectStateChanged, CEGUI::Event::Subscriber(&GUIEditor::Handle_PostFXToggle, this));
    }
    // Toggle Bounding Boxes
    {
        _toggleButtons[TOGGLE_BOUNDING_BOXES] = static_cast<CEGUI::ToggleButton*>(EditorBar->getChild("ToggleBoundingBoxes"));
        _toggleButtons[TOGGLE_BOUNDING_BOXES]->subscribeEvent(CEGUI::ToggleButton::EventSelectStateChanged, CEGUI::Event::Subscriber(&GUIEditor::Handle_BoundingBoxesToggle, this));
    }
    // Toggle NavMesh Rendering
    {
        _toggleButtons[TOGGLE_NAV_MESH_DRAW] = static_cast<CEGUI::ToggleButton*>(EditorBar->getChild("DebugDraw"));
        _toggleButtons[TOGGLE_NAV_MESH_DRAW]->subscribeEvent(CEGUI::ToggleButton::EventSelectStateChanged, CEGUI::Event::Subscriber(&GUIEditor::Handle_DrawNavMeshToggle, this));
    }
    // Toggle Skeleton Rendering
    {
        _toggleButtons[TOGGLE_SKELETONS] = static_cast<CEGUI::ToggleButton*>(EditorBar->getChild("ToggleSkeletons"));
        _toggleButtons[TOGGLE_SKELETONS]->subscribeEvent(CEGUI::ToggleButton::EventSelectStateChanged, CEGUI::Event::Subscriber(&GUIEditor::Handle_SkeletonsToggle, this));
    }

    // Type new X Position
    {
        _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_X] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("XPositionValue"));
        _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_X]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_PositionXChange, this));
        _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_X]->subscribeEvent(CEGUI::Editbox::EventInputCaptureGained, CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_X]->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Type new Y Position
    {
        _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_Y] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("YPositionValue"));
        _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_Y]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_PositionYChange, this));
        _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_Y]->subscribeEvent(CEGUI::Editbox::EventInputCaptureGained, CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_Y]->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Type new Z Position
    {
        _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_Z] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("ZPositionValue"));
        _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_Z]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_PositionZChange, this));
        _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_Z]->subscribeEvent(CEGUI::Editbox::EventInputCaptureGained, CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_Z]->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Type new Position Granularity
    {
        _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("PositionGranularityValue"));
        _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_PositionGranularityChange, this));
        _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY]->setText(CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY]));
        _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY]->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Type new X Rotation
    {
        _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_X] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("XRotationValue"));
        _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_X]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_RotationXChange, this));
        _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_X]->subscribeEvent(CEGUI::Editbox::EventInputCaptureGained, CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_X]->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Type new Y Rotation
    {
        _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_Y] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("YRotationValue"));
        _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_Y]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_RotationYChange, this));
        _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_Y]->subscribeEvent(CEGUI::Editbox::EventInputCaptureGained, CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_Y]->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Type new Z Rotation
    {
        _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_Z] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("ZRotationValue"));
        _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_Z]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_RotationZChange, this));
        _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_Z]->subscribeEvent(CEGUI::Editbox::EventInputCaptureGained, CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_Z]->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Type new Rotation Granularity
    {
        _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("RotationGranularityValue"));
        _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_RotationGranularityChange, this));
        _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]->setText(CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]));
        _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Type new X Scale
    {
        _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_X] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("XScaleValue"));
        _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_X]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_ScaleXChange, this));
        _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_X]->subscribeEvent(CEGUI::Editbox::EventInputCaptureGained, CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_X]->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Type new Y Scale
    {
        _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_Y] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("YScaleValue"));
        _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_Y]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_ScaleYChange, this));
        _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_Y]->subscribeEvent(CEGUI::Editbox::EventInputCaptureGained, CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_Y]->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Type new Z Scale
    {
        _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_Z] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("ZScaleValue"));
        _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_Z]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_ScaleZChange, this));
        _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_Z]->subscribeEvent(CEGUI::Editbox::EventInputCaptureGained, CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_Z]->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Type new Scale Granularity
    {
        _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("ScaleGranularityValue"));
        _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_ScaleGranularityChange, this));
        _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY]->setText(CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY]));
        _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY]->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Increment X Position
    {
        _transformButtonsInc[TRANSFORM_POSITION][CONTROL_FIELD_X] = static_cast<CEGUI::Window*>(EditorBar->getChild("IncPositionXButton"));
        _transformButtonsInc[TRANSFORM_POSITION][CONTROL_FIELD_X]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementPositionX, this));
    }
    // Decrement X Position
    {
        _transformButtonsDec[TRANSFORM_POSITION][CONTROL_FIELD_X] = static_cast<CEGUI::Window*>(EditorBar->getChild("DecPositionXButton"));
        _transformButtonsDec[TRANSFORM_POSITION][CONTROL_FIELD_X]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementPositionX, this));
    }
    // Increment Y Position
    {
        _transformButtonsInc[TRANSFORM_POSITION][CONTROL_FIELD_Y]  = static_cast<CEGUI::Window*>(EditorBar->getChild("IncPositionYButton"));
        _transformButtonsInc[TRANSFORM_POSITION][CONTROL_FIELD_Y]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementPositionY, this));
    }
    // Decrement Y Position
    {
        _transformButtonsDec[TRANSFORM_POSITION][CONTROL_FIELD_Y] = static_cast<CEGUI::Window*>(EditorBar->getChild("DecPositionYButton"));
        _transformButtonsDec[TRANSFORM_POSITION][CONTROL_FIELD_Y]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementPositionY, this));
    }
    // Increment Z Position
    {
        _transformButtonsInc[TRANSFORM_POSITION][CONTROL_FIELD_Z] = static_cast<CEGUI::Window*>(EditorBar->getChild("IncPositionZButton"));
        _transformButtonsInc[TRANSFORM_POSITION][CONTROL_FIELD_Z]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementPositionZ, this));
    }
    // Decrement Z Position
    {
        _transformButtonsDec[TRANSFORM_POSITION][CONTROL_FIELD_Z] = static_cast<CEGUI::Window*>(EditorBar->getChild("DecPositionZButton"));
        _transformButtonsDec[TRANSFORM_POSITION][CONTROL_FIELD_Z]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementPositionZ, this));
    }
    // Increment Position Granularity
    {
        _transformButtonsInc[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY] = static_cast<CEGUI::Window*>(EditorBar->getChild("IncPositionGranButton"));
        _transformButtonsInc[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementPositionGranularity, this));
    }
    // Decrement Position Granularity
    {
        _transformButtonsDec[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY] = static_cast<CEGUI::Window*>(EditorBar->getChild("DecPositionGranButton"));
        _transformButtonsDec[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementPositionGranularity, this));
    }
    // Increment X Rotation
    {
        _transformButtonsInc[TRANSFORM_ROTATION][CONTROL_FIELD_X] = static_cast<CEGUI::Window*>(EditorBar->getChild("IncRotationXButton"));
        _transformButtonsInc[TRANSFORM_ROTATION][CONTROL_FIELD_X]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementRotationX, this));
    }
    // Decrement X Rotation
    {
        _transformButtonsDec[TRANSFORM_ROTATION][CONTROL_FIELD_X] = static_cast<CEGUI::Window*>(EditorBar->getChild("DecRotationXButton"));
        _transformButtonsDec[TRANSFORM_ROTATION][CONTROL_FIELD_X]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementRotationX, this));
    }
    // Increment Y Rotation
    {
        _transformButtonsInc[TRANSFORM_ROTATION][CONTROL_FIELD_Y] = static_cast<CEGUI::Window*>(EditorBar->getChild("IncRotationYButton"));
        _transformButtonsInc[TRANSFORM_ROTATION][CONTROL_FIELD_Y]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementRotationY, this));
    }
    // Decrement Y Rotation
    {
        _transformButtonsDec[TRANSFORM_ROTATION][CONTROL_FIELD_Y] = static_cast<CEGUI::Window*>(EditorBar->getChild("DecRotationYButton"));
        _transformButtonsDec[TRANSFORM_ROTATION][CONTROL_FIELD_Y]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementRotationY, this));
    }
    // Increment Z Rotation
    {
        _transformButtonsInc[TRANSFORM_ROTATION][CONTROL_FIELD_Z] = static_cast<CEGUI::Window*>(EditorBar->getChild("IncRotationZButton"));
        _transformButtonsInc[TRANSFORM_ROTATION][CONTROL_FIELD_Z]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementRotationZ, this));
    }
    // Decrement Z Rotation
    {
        _transformButtonsDec[TRANSFORM_ROTATION][CONTROL_FIELD_Z] = static_cast<CEGUI::Window*>(EditorBar->getChild("DecRotationZButton"));
        _transformButtonsDec[TRANSFORM_ROTATION][CONTROL_FIELD_Z]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementRotationZ, this));
    }
    // Increment Rotation Granularity
    {
        _transformButtonsInc[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY] = static_cast<CEGUI::Window*>(EditorBar->getChild("IncRotationGranButton"));
        _transformButtonsInc[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementRotationGranularity, this));
    }
    // Decrement Rotation Granularity
    {
        _transformButtonsDec[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY] = static_cast<CEGUI::Window*>(EditorBar->getChild("DecRotationGranButton"));
        _transformButtonsDec[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementRotationGranularity, this));
    }
    // Increment X Scale
    {
        _transformButtonsInc[TRANSFORM_SCALE][CONTROL_FIELD_X] = static_cast<CEGUI::Window*>(EditorBar->getChild("IncScaleXButton"));
        _transformButtonsInc[TRANSFORM_SCALE][CONTROL_FIELD_X]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementScaleX, this));
    }
    // Decrement X Scale
    {
        _transformButtonsDec[TRANSFORM_SCALE][CONTROL_FIELD_X] = static_cast<CEGUI::Window*>(EditorBar->getChild("DecScaleXButton"));
        _transformButtonsDec[TRANSFORM_SCALE][CONTROL_FIELD_X]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementScaleX, this));
    }
    // Increment Y Scale
    {
        _transformButtonsInc[TRANSFORM_SCALE][CONTROL_FIELD_Y]= static_cast<CEGUI::Window*>(EditorBar->getChild("IncScaleYButton"));
        _transformButtonsInc[TRANSFORM_SCALE][CONTROL_FIELD_Y]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementScaleY, this));
    }
    // Decrement Y Scale
    {
        _transformButtonsDec[TRANSFORM_SCALE][CONTROL_FIELD_Y] = static_cast<CEGUI::Window*>(EditorBar->getChild("DecScaleYButton"));
        _transformButtonsDec[TRANSFORM_SCALE][CONTROL_FIELD_Y]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementScaleY, this));
    }
    // Increment Z Scale
    {
        _transformButtonsInc[TRANSFORM_SCALE][CONTROL_FIELD_Z] = static_cast<CEGUI::Window*>(EditorBar->getChild("IncScaleZButton"));
        _transformButtonsInc[TRANSFORM_SCALE][CONTROL_FIELD_Z]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementScaleZ, this));
    }
    // Decrement Z Scale
    {
        _transformButtonsDec[TRANSFORM_SCALE][CONTROL_FIELD_Z] = static_cast<CEGUI::Window*>(EditorBar->getChild("DecScaleZButton"));
        _transformButtonsDec[TRANSFORM_SCALE][CONTROL_FIELD_Z]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementScaleZ, this));
    }
    // Increment Scale Granularity
    {
        _transformButtonsInc[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY] = static_cast<CEGUI::Window*>(EditorBar->getChild("IncScaleGranButton"));
        _transformButtonsInc[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementScaleGranularity, this));
    }
    // Decrement Scale Granularity
    {
        _transformButtonsDec[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY] = static_cast<CEGUI::Window*>(EditorBar->getChild("DecScaleGranButton"));
        _transformButtonsDec[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY]->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementScaleGranularity, this));
    }
}

bool GUIEditor::Handle_MenuBarClickOn(const CEGUI::EventArgs &e) {
    _wasControlClick = true;
    return true;
}

bool GUIEditor::Handle_EditFieldClick(const CEGUI::EventArgs &e) {
    _pauseSelectionTracking = true;
    return true;
}

bool GUIEditor::Handle_CreateNavMesh(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: NavMesh creation queued!");
	GUI::getInstance().getConsole()->setVisible(true);
	_createNavMeshQueued = true;
	return true;
}

bool GUIEditor::Handle_SaveScene(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Saving scene!");
    return true;
}

bool GUIEditor::Handle_SaveSelection(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Saving selection!");
    return true;
}

bool GUIEditor::Handle_DeleteSelection(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Deleting selection!");
    return true;
}

bool GUIEditor::Handle_ReloadScene(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Reloading scene!");
    return true;
}

bool GUIEditor::Handle_PrintSceneGraph(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]:Printing scene graph!");
    GET_ACTIVE_SCENEGRAPH()->print();
    return true;
}

bool GUIEditor::Handle_WireframeToggle(const CEGUI::EventArgs &e) {
    if ( _toggleButtons[TOGGLE_WIREFRAME]->isSelected() ) {
        D_PRINT_FN("[Editor]: Wireframe rendering enabled!");
    } else {
        D_PRINT_FN("[Editor]: Wireframe rendering disabled!");
    }
    if (_currentSelection) {
        _currentSelection->renderWireframe(_toggleButtons[TOGGLE_WIREFRAME]->isSelected());
    }
    return true;
}

bool GUIEditor::Handle_DepthPreviewToggle(const CEGUI::EventArgs &e) {
    if ( _toggleButtons[TOGGLE_DEPTH_PREVIEW]->isSelected() ) {
        D_PRINT_FN("[Editor]: Depth Preview enabled!");
    } else {
        D_PRINT_FN("[Editor]: Depth Preview disabled!");
    }
    ParamHandler::getInstance().setParam("postProcessing.fullScreenDepthBuffer", _toggleButtons[TOGGLE_DEPTH_PREVIEW]->isSelected());
    return true;
}

bool GUIEditor::Handle_ShadowMappingToggle(const CEGUI::EventArgs &e) {
    if ( _toggleButtons[TOGGLE_SHADOW_MAPPING]->isSelected() ) {
        D_PRINT_FN("[Editor]: Shadow Mapping enabled!");
    } else {
        D_PRINT_FN("[Editor]: Shadow Mapping disabled!");
    }
    if (_currentSelection) {
        _currentSelection->castsShadows(_toggleButtons[TOGGLE_SHADOW_MAPPING]->isSelected());
    }
    return true;
}

bool GUIEditor::Handle_FogToggle(const CEGUI::EventArgs &e) {
    if (_toggleButtons[TOGGLE_FOG]->isSelected() ) {
        D_PRINT_FN("[Editor]: Fog enabled!");
    } else {
        D_PRINT_FN("[Editor]: Fog disabled!");
    }

    ParamHandler::getInstance().setParam("rendering.enableFog", _toggleButtons[TOGGLE_FOG]->isSelected());

    return true;
}

bool GUIEditor::Handle_PostFXToggle(const CEGUI::EventArgs &e) {
    if ( _toggleButtons[TOGGLE_POST_FX]->isSelected() ) {
        D_PRINT_FN("[Editor]: PostFX enabled!");
    } else {
        D_PRINT_FN("[Editor]: PostFX disabled!");
    }
    GFX_DEVICE.postProcessingEnabled(_toggleButtons[TOGGLE_POST_FX]->isSelected());
    return true;
}

bool GUIEditor::Handle_BoundingBoxesToggle(const CEGUI::EventArgs &e) {
    if ( _toggleButtons[TOGGLE_BOUNDING_BOXES]->isSelected() ) {
        D_PRINT_FN("[Editor]: Bounding Box rendering enabled!");
    } else {
        D_PRINT_FN("[Editor]: Bounding Box rendering disabled!");
    }
    if (_currentSelection) {
        _currentSelection->renderBoundingBox(_toggleButtons[TOGGLE_BOUNDING_BOXES]->isSelected());
    }
    return true;
}

bool GUIEditor::Handle_DrawNavMeshToggle(const CEGUI::EventArgs &e) {
    if ( _toggleButtons[TOGGLE_NAV_MESH_DRAW]->isSelected() ) {
        D_PRINT_FN("[Editor]: NavMesh rendering enabled!");
    } else {
        D_PRINT_FN("[Editor]: NavMesh rendering disabled!");
    }
    AIManager::getInstance().toggleNavMeshDebugDraw(_toggleButtons[TOGGLE_NAV_MESH_DRAW]->isSelected());
    return true;
}

bool GUIEditor::Handle_SkeletonsToggle(const CEGUI::EventArgs &e) {
    if ( _toggleButtons[TOGGLE_SKELETONS]->isSelected() ) {
        D_PRINT_FN("[Editor]: Skeleton rendering enabled!");
    } else {
        D_PRINT_FN("[Editor]: Skeleton rendering disabled!");
    }

    if (_currentSelection) {
        _currentSelection->renderSkeleton(_toggleButtons[TOGGLE_SKELETONS]->isSelected());
    }
    return true;
}

bool GUIEditor::Handle_PositionXChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position X changed via text edit!");
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_X] = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_POSITION][CONTROL_FIELD_X]->getText());
    _currentSelection->getComponent<PhysicsComponent>()->setPositionX(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_X]);
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_PositionYChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position Y changed via text edit!");
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Y] = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_POSITION][CONTROL_FIELD_Y]->getText());
    _currentSelection->getComponent<PhysicsComponent>()->setPositionY(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Y]);
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_PositionZChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position Z changed via text edit!");
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Z] = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_POSITION][CONTROL_FIELD_Z]->getText());
    _currentSelection->getComponent<PhysicsComponent>()->setPositionZ(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Z]);
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_PositionGranularityChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position Granularity changed via text edit!");
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY] = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY]->getText());
    CLAMP<F32>(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY], 0.0000001f, 10000000.0f);
    _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY]->setText(CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY]));
    return true;
}

bool GUIEditor::Handle_RotationXChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Rotation X changed via text edit!");
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X] = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_X]->getText());
    CLAMP<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X], -360.0f, 360.0f);
    _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_X]->setText(CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X]));
    _currentSelection->getComponent<PhysicsComponent>()->setRotation(vec3<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X],
                                                                               _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y],
                                                                               _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z]));
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_RotationYChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Rotation Y changed via text edit!");
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y] = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_Y]->getText());
    CLAMP<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y], -360.0f, 360.0f);
    _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_Y]->setText(CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y]));
    _currentSelection->getComponent<PhysicsComponent>()->setRotation(vec3<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X],
                                                                               _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y],
                                                                               _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z]));
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_RotationZChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position X changed via text edit!");
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z] = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_Z]->getText());
    CLAMP<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z], -360.0f, 360.0f);
    _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_Z]->setText(CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z]));
    _currentSelection->getComponent<PhysicsComponent>()->setRotation(vec3<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X],
                                                                               _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y],
                                                                               _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z]));
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_RotationGranularityChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Rotation Granularity changed via text edit!");
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY] = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]->getText());
    CLAMP<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY], 0.0000001f, 359.0f);
    _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]->setText(CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]));
    return true;
}

bool GUIEditor::Handle_ScaleXChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Scale X changed via text edit!");
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_X] = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_SCALE][CONTROL_FIELD_X]->getText());
    _currentSelection->getComponent<PhysicsComponent>()->setScaleX(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_X]);
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_ScaleYChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Scale Y changed via text edit!");
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Y] = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_SCALE][CONTROL_FIELD_Y]->getText());
    _currentSelection->getComponent<PhysicsComponent>()->setScaleY(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Y]);
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_ScaleZChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Scale Z changed via text edit!");
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Z] = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_SCALE][CONTROL_FIELD_Z]->getText());
    _currentSelection->getComponent<PhysicsComponent>()->setScaleZ(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Z]);
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_ScaleGranularityChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Scale Granularity changed via text edit!");
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY] = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY]->getText());
    CLAMP<F32>(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY], 0.0000001f, 10000000.0f);
    _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY]->setText(CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY]));
    return true;
}

bool GUIEditor::Handle_IncrementPositionX(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position X incremented via button!");
    // If we don't have a selection, the button is disabled
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_X] += _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setPositionX(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_X]);
    return true;
}

bool GUIEditor::Handle_DecrementPositionX(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position X decremented via button!");
    // If we don't have a selection, the button is disabled
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_X] -= _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setPositionX(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_X]);
    return true;
}

bool GUIEditor::Handle_IncrementPositionY(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position Y incremented via button!");
    // If we don't have a selection, the button is disabled
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Y] += _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setPositionY(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Y]);
    return true;
}

bool GUIEditor::Handle_DecrementPositionY(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position Y decremented via button!");
    // If we don't have a selection, the button is disabled
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Y] -= _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setPositionY(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Y]);
    return true;
}

bool GUIEditor::Handle_IncrementPositionZ(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position Z incremented via button!");
    // If we don't have a selection, the button is disabled
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Z] += _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setPositionZ(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Z]);
    return true;
}

bool GUIEditor::Handle_DecrementPositionZ(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position Z decremented via button!");
    // If we don't have a selection, the button is disabled
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Z] -= _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setPositionZ(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Z]);
    return true;
}

bool GUIEditor::Handle_IncrementPositionGranularity(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position Granularity incremented via button!");
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY] = std::min(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY] * 10.0f, 10000000.0f);
    _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY]->setText(CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY]));
    return true;
}

bool GUIEditor::Handle_DecrementPositionGranularity(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position Granularity decremented via button!");
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY] = std::max(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY] / 10.0f, 0.0000001f);
    _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY]->setText(CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY]));
    return true;
}

bool GUIEditor::Handle_IncrementRotationX(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Rotation X incremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X] += _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setRotation(vec3<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X],
                                                                               _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y],
                                                                               _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z]));
    return true;
}

bool GUIEditor::Handle_DecrementRotationX(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Rotation X decremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X] -= _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setRotation(vec3<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X],
                                                                               _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y],
                                                                               _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z]));
    return true;
}

bool GUIEditor::Handle_IncrementRotationY(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Rotation Y incremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y] += _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setRotation(vec3<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X],
                                                                               _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y],
                                                                               _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z]));
    return true;
}

bool GUIEditor::Handle_DecrementRotationY(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Rotation Y decremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y] -= _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setRotation(vec3<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X],
                                                                               _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y],
                                                                               _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z]));
    return true;
}

bool GUIEditor::Handle_IncrementRotationZ(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Rotation Z incremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z] += _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setRotation(vec3<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X],
                                                                               _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y],
                                                                               _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z]));
    return true;
}

bool GUIEditor::Handle_DecrementRotationZ(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Rotation Z decremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z] -= _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setRotation(vec3<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X],
                                                                               _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y],
                                                                               _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z]));
    return true;
}

bool GUIEditor::Handle_IncrementRotationGranularity(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Rotation Granularity incremented via button!");
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY] = std::min(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY] * 10.0f, 359.0f);
    _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]->setText(CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]));
    return true;
}

bool GUIEditor::Handle_DecrementRotationGranularity(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Rotation Granularity decremented via button!");
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY] = std::max(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY] / 10.0f, 0.0000001f);
    _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]->setText(CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]));
    return true;
}

bool GUIEditor::Handle_IncrementScaleX(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Scale X incremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_X] += _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setScaleX(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_X]);
    return true;
}

bool GUIEditor::Handle_DecrementScaleX(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Scale X decremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_X] -= _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setScaleX(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_X]);
    return true;
}

bool GUIEditor::Handle_IncrementScaleY(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Scale Y incremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Y] += _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setScaleY(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Y]);
    return true;
}

bool GUIEditor::Handle_DecrementScaleY(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Scale Y decremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Y] -= _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setScaleY(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Y]);
    return true;
}

bool GUIEditor::Handle_IncrementScaleZ(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Scale Z incremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Z] += _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setScaleZ(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Z]);
    return true;
}

bool GUIEditor::Handle_DecrementScaleZ(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Scale Z decremented via button!");
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Z] -= _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setScaleZ(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Z]);
    return true;
}

bool GUIEditor::Handle_IncrementScaleGranularity(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Scale Granularity incremented via button!");
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY] = std::min(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY] * 10.0f, 10000000.0f);
    _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY]->setText(CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY]));
    return true;
}

bool GUIEditor::Handle_DecrementScaleGranularity(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Scale Granularity decremented via button!");
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY] = std::max(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY] / 10.0f, 0.0000001f);
    _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY]->setText(CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY]));
    return true;
}