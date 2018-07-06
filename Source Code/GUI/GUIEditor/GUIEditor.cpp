#include <CEGUI/CEGUI.h>
#include "Headers/GUIEditor.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIConsole.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/AIManager.h"
#include "Managers/Headers/SceneManager.h"

namespace Divide {

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
        Console::errorfn(Locale::get("ERROR_EDITOR_DOUBLE_INIT"));
        return false;
    }

    Console::printfn(Locale::get("GUI_EDITOR_INIT"));
    // Get a local pointer to the CEGUI Window Manager, Purely for convenience to reduce typing
    CEGUI::WindowManager *pWindowManager = CEGUI::WindowManager::getSingletonPtr();
    // load the editor Window from the layout file
    const stringImpl& layoutFile = ParamHandler::getInstance().getParam<stringImpl>("GUI.editorLayout");
    _editorWindow = pWindowManager->loadLayoutFromFile(layoutFile.c_str());

    if (_editorWindow) {
         // Add the Window to the GUI Root Sheet
         CEGUI_DEFAULT_CONTEXT.getRootWindow()->addChild(_editorWindow);
         // Now register the handlers for the events (Clicking, typing, etc)
         RegisterHandlers();
    } else {
         // Loading layout from file, failed, so output an error Message.
         CEGUI::Logger::getSingleton().logEvent("Error: Unable to load the Editor from .layout");
         Console::errorfn(Locale::get("ERROR_EDITOR_LAYOUT_FILE"),layoutFile.c_str());
         return false;
    }

    setVisible(false);

    _init = true;
    Console::printfn(Locale::get("GUI_EDITOR_CREATED"));
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
        PhysicsComponent* const selectionTransform = _currentSelection->getComponent<PhysicsComponent>();
        const vec3<F32>& localPosition = selectionTransform->getPosition();
        const vec3<F32>& localScale = selectionTransform->getScale();
        vec3<F32> localOrientation = Divide::getEuler(selectionTransform->getOrientation(), true);

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

        RenderingComponent* rComp = _currentSelection->getComponent<RenderingComponent>();
        _toggleButtons[TOGGLE_SKELETONS]->setSelected(rComp->renderSkeleton());
        _toggleButtons[TOGGLE_SHADOW_MAPPING]->setSelected(rComp->castsShadows());
        _toggleButtons[TOGGLE_WIREFRAME]->setSelected(rComp->renderWireframe());
        _toggleButtons[TOGGLE_BOUNDING_BOXES]->setSelected(rComp->renderBoundingBox());
    }
}

void GUIEditor::UpdateControls() {
    bool hasValidTransform = false;
    if (_currentSelection) {
        hasValidTransform = (_currentSelection->getComponent<PhysicsComponent>() != nullptr);
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

    bool depthPreview = ParamHandler::getInstance().getParam<bool>("postProcessing.fullScreenDepthBuffer", false);
    _toggleButtons[TOGGLE_DEPTH_PREVIEW]->setSelected(depthPreview);
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
        AI::Navigation::NavigationMesh* temp = AI::AIManager::getInstance().getNavMesh(AI::AIEntity::AGENT_RADIUS_SMALL);
        // Check debug rendering status
        AI::AIManager::getInstance().toggleNavMeshDebugDraw(_toggleButtons[TOGGLE_NAV_MESH_DRAW]->isSelected());
        // Create a new NavMesh if we don't currently have one
        if (!temp) {
            temp = MemoryManager_NEW AI::Navigation::NavigationMesh();
        }
        // Set it's file name
        temp->setFileName(GET_ACTIVE_SCENE()->getName());
        // Try to load it from file
        bool loaded = temp->load(nullptr);
        if (!loaded) {
            // If we failed to load it from file, we need to build it first
            loaded = temp->build(nullptr, AI::Navigation::NavigationMesh::CreationCallback(), false);
            // Then save it to file
            temp->save();
        }
        // If we loaded/built the NavMesh correctly, add it to the AIManager
        if (loaded) {
            state = AI::AIManager::getInstance().addNavMesh(AI::AIEntity::AGENT_RADIUS_SMALL, temp);
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
    EditorBar->subscribeEvent(CEGUI::Window::EventMouseButtonUp,
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_MenuBarClickOn, this));

    for (U16 i = 0; i < EditorBar->getChildCount(); ++i) { 
        EditorBar->getChildElementAtIdx(i)->subscribeEvent(CEGUI::Window::EventMouseButtonUp, 
                                                           CEGUI::Event::Subscriber(&GUIEditor::Handle_MenuBarClickOn, this));
    }

    // Save Scene Button
    {
        CEGUI::Window* saveSceneButton = static_cast<CEGUI::Window*>(EditorBar->getChild("SaveSceneButton"));
        saveSceneButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_SaveScene, this));
    }
    // Save Selection Button
    { 
        _saveSelectionButton = static_cast<CEGUI::Window*>(EditorBar->getChild("SaveSelectionButton"));
        _saveSelectionButton->subscribeEvent(CEGUI::PushButton::EventClicked,
                                             CEGUI::Event::Subscriber(&GUIEditor::Handle_SaveSelection, this));
    }
    // Delete Selection Button
    {
        _deleteSelectionButton = static_cast<CEGUI::Window*>(EditorBar->getChild("DeleteSelection"));
        _deleteSelectionButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                               CEGUI::Event::Subscriber(&GUIEditor::Handle_DeleteSelection, this));
    }
    // Generate NavMesh button
    {
        CEGUI::Window* createNavMeshButton = static_cast<CEGUI::Window*>(EditorBar->getChild("NavMeshButton"));
        createNavMeshButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                            CEGUI::Event::Subscriber(&GUIEditor::Handle_CreateNavMesh, this));
    }
    // Reload Scene Button
    {
        CEGUI::Window* reloadSceneButton = static_cast<CEGUI::Window*>(EditorBar->getChild("ReloadSceneButton"));
        reloadSceneButton->subscribeEvent(CEGUI::PushButton::EventClicked,
                                          CEGUI::Event::Subscriber(&GUIEditor::Handle_ReloadScene, this));
    }
    // Print SceneGraph Button
    {
        CEGUI::Window* dumpSceneGraphButton = static_cast<CEGUI::Window*>(EditorBar->getChild("DumpSceneGraphTree"));
        dumpSceneGraphButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                             CEGUI::Event::Subscriber(&GUIEditor::Handle_PrintSceneGraph, this));
    }
    CEGUI::ToggleButton* toggleButton = nullptr;
    // Toggle Wireframe rendering
    {
        _toggleButtons[TOGGLE_WIREFRAME] = static_cast<CEGUI::ToggleButton*>(EditorBar->getChild("ToggleWireframe"));
        toggleButton = _toggleButtons[TOGGLE_WIREFRAME];
        toggleButton->subscribeEvent(CEGUI::ToggleButton::EventSelectStateChanged,
                                     CEGUI::Event::Subscriber(&GUIEditor::Handle_WireframeToggle, this));
    }
    // Toggle Depth Preview
    {
        _toggleButtons[TOGGLE_DEPTH_PREVIEW]= static_cast<CEGUI::ToggleButton*>(EditorBar->getChild("ToggleDepthPreview"));
        toggleButton = _toggleButtons[TOGGLE_DEPTH_PREVIEW];
        toggleButton->subscribeEvent(CEGUI::ToggleButton::EventSelectStateChanged,
                                     CEGUI::Event::Subscriber(&GUIEditor::Handle_DepthPreviewToggle, this));
    }
    // Toggle Shadows
    {
        _toggleButtons[TOGGLE_SHADOW_MAPPING] = static_cast<CEGUI::ToggleButton*>(EditorBar->getChild("ToggleShadowMapping"));
        toggleButton = _toggleButtons[TOGGLE_SHADOW_MAPPING];
        toggleButton->subscribeEvent(CEGUI::ToggleButton::EventSelectStateChanged,
                                     CEGUI::Event::Subscriber(&GUIEditor::Handle_ShadowMappingToggle, this));    
    }
    // Toggle Fog
    {
        _toggleButtons[TOGGLE_FOG] = static_cast<CEGUI::ToggleButton*>(EditorBar->getChild("ToggleFog"));
        toggleButton = _toggleButtons[TOGGLE_FOG];
        toggleButton->subscribeEvent(CEGUI::ToggleButton::EventSelectStateChanged,
                                     CEGUI::Event::Subscriber(&GUIEditor::Handle_FogToggle, this));
    }
    // Toggle PostFX
    {
        _toggleButtons[TOGGLE_POST_FX] = static_cast<CEGUI::ToggleButton*>(EditorBar->getChild("TogglePostFX"));
        toggleButton = _toggleButtons[TOGGLE_POST_FX];
        toggleButton->subscribeEvent(CEGUI::ToggleButton::EventSelectStateChanged,
                                     CEGUI::Event::Subscriber(&GUIEditor::Handle_PostFXToggle, this));
    }
    // Toggle Bounding Boxes
    {
        _toggleButtons[TOGGLE_BOUNDING_BOXES] = static_cast<CEGUI::ToggleButton*>(EditorBar->getChild("ToggleBoundingBoxes"));
        toggleButton = _toggleButtons[TOGGLE_BOUNDING_BOXES];
        toggleButton->subscribeEvent(CEGUI::ToggleButton::EventSelectStateChanged,
                                     CEGUI::Event::Subscriber(&GUIEditor::Handle_BoundingBoxesToggle, this));
    }
    // Toggle NavMesh Rendering
    {
        _toggleButtons[TOGGLE_NAV_MESH_DRAW] = static_cast<CEGUI::ToggleButton*>(EditorBar->getChild("DebugDraw"));
        toggleButton = _toggleButtons[TOGGLE_NAV_MESH_DRAW];
        toggleButton->subscribeEvent(CEGUI::ToggleButton::EventSelectStateChanged,
                                     CEGUI::Event::Subscriber(&GUIEditor::Handle_DrawNavMeshToggle, this));
    }
    // Toggle Skeleton Rendering
    {
        _toggleButtons[TOGGLE_SKELETONS] = static_cast<CEGUI::ToggleButton*>(EditorBar->getChild("ToggleSkeletons"));
        toggleButton = _toggleButtons[TOGGLE_SKELETONS];
        toggleButton->subscribeEvent(CEGUI::ToggleButton::EventSelectStateChanged,
                                     CEGUI::Event::Subscriber(&GUIEditor::Handle_SkeletonsToggle, this));
    }

    CEGUI::Editbox* field = nullptr;
    // Type new X Position
    {
        _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_X] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("XPositionValue"));
        field = _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_X];
        field->subscribeEvent(CEGUI::Editbox::EventTextAccepted,
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_PositionXChange, this));
        field->subscribeEvent(CEGUI::Editbox::EventInputCaptureGained,
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        field->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Type new Y Position
    {
        _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_Y] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("YPositionValue"));
        field = _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_Y];
        field->subscribeEvent(CEGUI::Editbox::EventTextAccepted,
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_PositionYChange, this));
        field->subscribeEvent(CEGUI::Editbox::EventInputCaptureGained,
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        field->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Type new Z Position
    {
        _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_Z] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("ZPositionValue"));
        field = _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_Z];
        field->subscribeEvent(CEGUI::Editbox::EventTextAccepted,
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_PositionZChange, this));
        field->subscribeEvent(CEGUI::Editbox::EventInputCaptureGained,
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        field->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }

    // Type new Position Granularity
    {
        _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY] = 
                                                    static_cast<CEGUI::Editbox*>(EditorBar->getChild("PositionGranularityValue"));
        field = _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY];
        field->subscribeEvent(CEGUI::Editbox::EventTextAccepted, 
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_PositionGranularityChange, this));
        field->setText(CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY]));
        field->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Type new X Rotation
    {
        _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_X] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("XRotationValue"));
        field = _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_X];
        field->subscribeEvent(CEGUI::Editbox::EventTextAccepted, 
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_RotationXChange, this));
        field->subscribeEvent(CEGUI::Editbox::EventInputCaptureGained,
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        field->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Type new Y Rotation
    {
        _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_Y] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("YRotationValue"));
        field = _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_Y];
        field->subscribeEvent(CEGUI::Editbox::EventTextAccepted,
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_RotationYChange, this));
        field->subscribeEvent(CEGUI::Editbox::EventInputCaptureGained,
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        field->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Type new Z Rotation
    {
        _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_Z] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("ZRotationValue"));
        field = _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_Z];
        field->subscribeEvent(CEGUI::Editbox::EventTextAccepted, 
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_RotationZChange, this));
        field->subscribeEvent(CEGUI::Editbox::EventInputCaptureGained, 
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        field->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Type new Rotation Granularity
    {
        _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY] =
                                                    static_cast<CEGUI::Editbox*>(EditorBar->getChild("RotationGranularityValue"));
        field = _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY];
        field->subscribeEvent(CEGUI::Editbox::EventTextAccepted, 
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_RotationGranularityChange, this));
        field->setText(CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]));
        field->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Type new X Scale
    {
        _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_X] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("XScaleValue"));
        field = _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_X];
        field->subscribeEvent(CEGUI::Editbox::EventTextAccepted, 
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_ScaleXChange, this));
        field->subscribeEvent(CEGUI::Editbox::EventInputCaptureGained, 
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        field->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Type new Y Scale
    {
        _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_Y] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("YScaleValue"));
        field = _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_Y];
        field->subscribeEvent(CEGUI::Editbox::EventTextAccepted, 
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_ScaleYChange, this));
        field->subscribeEvent(CEGUI::Editbox::EventInputCaptureGained, 
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        field->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Type new Z Scale
    {
        _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_Z] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("ZScaleValue"));
        field = _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_Z];
        field->subscribeEvent(CEGUI::Editbox::EventTextAccepted, 
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_ScaleZChange, this));
        field->subscribeEvent(CEGUI::Editbox::EventInputCaptureGained, 
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        field->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    // Type new Scale Granularity
    {
        _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY] =
                                                        static_cast<CEGUI::Editbox*>(EditorBar->getChild("ScaleGranularityValue"));
        field = _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY];
        field->subscribeEvent(CEGUI::Editbox::EventTextAccepted,
                              CEGUI::Event::Subscriber(&GUIEditor::Handle_ScaleGranularityChange, this));
        field->setText(CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY]));
        field->setValidationString(CEGUI::String("^[-+]?[0-9]{1,10}([.][0-9]{1,10})?$"));
    }
    CEGUI::Window* transformButton = nullptr;
    // Increment X Position
    {
        _transformButtonsInc[TRANSFORM_POSITION][CONTROL_FIELD_X] = EditorBar->getChild("IncPositionXButton");
        transformButton = _transformButtonsInc[TRANSFORM_POSITION][CONTROL_FIELD_X];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked,
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementPositionX, this));
    }
    // Decrement X Position
    {
        _transformButtonsDec[TRANSFORM_POSITION][CONTROL_FIELD_X] = EditorBar->getChild("DecPositionXButton");
        transformButton = _transformButtonsDec[TRANSFORM_POSITION][CONTROL_FIELD_X];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementPositionX, this));
    }
    // Increment Y Position
    {
        _transformButtonsInc[TRANSFORM_POSITION][CONTROL_FIELD_Y]  = EditorBar->getChild("IncPositionYButton");
        transformButton = _transformButtonsInc[TRANSFORM_POSITION][CONTROL_FIELD_Y];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementPositionY, this));
    }
    // Decrement Y Position
    {
        _transformButtonsDec[TRANSFORM_POSITION][CONTROL_FIELD_Y] = EditorBar->getChild("DecPositionYButton");
        transformButton = _transformButtonsDec[TRANSFORM_POSITION][CONTROL_FIELD_Y];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementPositionY, this));
    }
    // Increment Z Position
    {
        _transformButtonsInc[TRANSFORM_POSITION][CONTROL_FIELD_Z] = EditorBar->getChild("IncPositionZButton");
        transformButton = _transformButtonsInc[TRANSFORM_POSITION][CONTROL_FIELD_Z];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementPositionZ, this));
    }
    // Decrement Z Position
    {
        _transformButtonsDec[TRANSFORM_POSITION][CONTROL_FIELD_Z] = EditorBar->getChild("DecPositionZButton");
        transformButton = _transformButtonsDec[TRANSFORM_POSITION][CONTROL_FIELD_Z];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementPositionZ, this));
    }
    // Increment Position Granularity
    {
        _transformButtonsInc[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY] = EditorBar->getChild("IncPositionGranButton");
        transformButton = _transformButtonsInc[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementPositionGranularity, this));
    }
    // Decrement Position Granularity
    {
        _transformButtonsDec[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY] = EditorBar->getChild("DecPositionGranButton");
        transformButton = _transformButtonsDec[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementPositionGranularity, this));
    }
    // Increment X Rotation
    {
        _transformButtonsInc[TRANSFORM_ROTATION][CONTROL_FIELD_X] = EditorBar->getChild("IncRotationXButton");
        transformButton = _transformButtonsInc[TRANSFORM_ROTATION][CONTROL_FIELD_X];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementRotationX, this));
    }
    // Decrement X Rotation
    {
        _transformButtonsDec[TRANSFORM_ROTATION][CONTROL_FIELD_X] = EditorBar->getChild("DecRotationXButton");
        transformButton = _transformButtonsDec[TRANSFORM_ROTATION][CONTROL_FIELD_X];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementRotationX, this));
    }
    // Increment Y Rotation
    {
        _transformButtonsInc[TRANSFORM_ROTATION][CONTROL_FIELD_Y] = EditorBar->getChild("IncRotationYButton");
        transformButton = _transformButtonsInc[TRANSFORM_ROTATION][CONTROL_FIELD_Y];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementRotationY, this));
    }
    // Decrement Y Rotation
    {
        _transformButtonsDec[TRANSFORM_ROTATION][CONTROL_FIELD_Y] = EditorBar->getChild("DecRotationYButton");
        transformButton = _transformButtonsDec[TRANSFORM_ROTATION][CONTROL_FIELD_Y];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementRotationY, this));
    }
    // Increment Z Rotation
    {
        _transformButtonsInc[TRANSFORM_ROTATION][CONTROL_FIELD_Z] = EditorBar->getChild("IncRotationZButton");
        transformButton = _transformButtonsInc[TRANSFORM_ROTATION][CONTROL_FIELD_Z];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementRotationZ, this));
    }
    // Decrement Z Rotation
    {
        _transformButtonsDec[TRANSFORM_ROTATION][CONTROL_FIELD_Z] = EditorBar->getChild("DecRotationZButton");
        transformButton = _transformButtonsDec[TRANSFORM_ROTATION][CONTROL_FIELD_Z];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementRotationZ, this));
    }
    // Increment Rotation Granularity
    {
        _transformButtonsInc[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY] = EditorBar->getChild("IncRotationGranButton");
        transformButton = _transformButtonsInc[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked,
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementRotationGranularity, this));
    }
    // Decrement Rotation Granularity
    {
        _transformButtonsDec[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY] = EditorBar->getChild("DecRotationGranButton");
        transformButton = _transformButtonsDec[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementRotationGranularity, this));
    }
    // Increment X Scale
    {
        _transformButtonsInc[TRANSFORM_SCALE][CONTROL_FIELD_X] = EditorBar->getChild("IncScaleXButton");
        _transformButtonsInc[TRANSFORM_SCALE][CONTROL_FIELD_X];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementScaleX, this));
    }
    // Decrement X Scale
    {
        _transformButtonsDec[TRANSFORM_SCALE][CONTROL_FIELD_X] = EditorBar->getChild("DecScaleXButton");
        transformButton = _transformButtonsDec[TRANSFORM_SCALE][CONTROL_FIELD_X];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementScaleX, this));
    }
    // Increment Y Scale
    {
        _transformButtonsInc[TRANSFORM_SCALE][CONTROL_FIELD_Y]= EditorBar->getChild("IncScaleYButton");
        transformButton = _transformButtonsInc[TRANSFORM_SCALE][CONTROL_FIELD_Y];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementScaleY, this));
    }
    // Decrement Y Scale
    {
        _transformButtonsDec[TRANSFORM_SCALE][CONTROL_FIELD_Y] = EditorBar->getChild("DecScaleYButton");
        transformButton = _transformButtonsDec[TRANSFORM_SCALE][CONTROL_FIELD_Y];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked,
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementScaleY, this));
    }
    // Increment Z Scale
    {
        _transformButtonsInc[TRANSFORM_SCALE][CONTROL_FIELD_Z] = EditorBar->getChild("IncScaleZButton");
        transformButton = _transformButtonsInc[TRANSFORM_SCALE][CONTROL_FIELD_Z];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementScaleZ, this));
    }
    // Decrement Z Scale
    {
        _transformButtonsDec[TRANSFORM_SCALE][CONTROL_FIELD_Z] = EditorBar->getChild("DecScaleZButton");
        transformButton = _transformButtonsDec[TRANSFORM_SCALE][CONTROL_FIELD_Z];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementScaleZ, this));
    }
    // Increment Scale Granularity
    {
        _transformButtonsInc[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY] = EditorBar->getChild("IncScaleGranButton");
        transformButton = _transformButtonsInc[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementScaleGranularity, this));
    }
    // Decrement Scale Granularity
    {
        _transformButtonsDec[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY] = EditorBar->getChild("DecScaleGranButton");
        transformButton = _transformButtonsDec[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY];
        transformButton->subscribeEvent(CEGUI::PushButton::EventClicked, 
                                        CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementScaleGranularity, this));
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
    Console::d_printfn("[Editor]: NavMesh creation queued!");
    GUI::getInstance().getConsole()->setVisible(true);
    _createNavMeshQueued = true;
    return true;
}

bool GUIEditor::Handle_SaveScene(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Saving scene!");
    return true;
}

bool GUIEditor::Handle_SaveSelection(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Saving selection!");
    return true;
}

bool GUIEditor::Handle_DeleteSelection(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Deleting selection!");
    return true;
}

bool GUIEditor::Handle_ReloadScene(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Reloading scene!");
    return true;
}

bool GUIEditor::Handle_PrintSceneGraph(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]:Printing scene graph!");
    GET_ACTIVE_SCENEGRAPH().print();
    return true;
}

bool GUIEditor::Handle_WireframeToggle(const CEGUI::EventArgs &e) {
    if ( _toggleButtons[TOGGLE_WIREFRAME]->isSelected() ) {
        Console::d_printfn("[Editor]: Wireframe rendering enabled!");
    } else {
        Console::d_printfn("[Editor]: Wireframe rendering disabled!");
    }
    if (_currentSelection) {
        _currentSelection->getComponent<RenderingComponent>()->renderWireframe(_toggleButtons[TOGGLE_WIREFRAME]->isSelected());
    }
    return true;
}

bool GUIEditor::Handle_DepthPreviewToggle(const CEGUI::EventArgs &e) {
    if ( _toggleButtons[TOGGLE_DEPTH_PREVIEW]->isSelected() ) {
        Console::d_printfn("[Editor]: Depth Preview enabled!");
    } else {
        Console::d_printfn("[Editor]: Depth Preview disabled!");
    }
    ParamHandler::getInstance().setParam("postProcessing.fullScreenDepthBuffer", 
                                         _toggleButtons[TOGGLE_DEPTH_PREVIEW]->isSelected());
    return true;
}

bool GUIEditor::Handle_ShadowMappingToggle(const CEGUI::EventArgs &e) {
    if ( _toggleButtons[TOGGLE_SHADOW_MAPPING]->isSelected() ) {
        Console::d_printfn("[Editor]: Shadow Mapping enabled!");
    } else {
        Console::d_printfn("[Editor]: Shadow Mapping disabled!");
    }
    if (_currentSelection) {
        _currentSelection->getComponent<RenderingComponent>()->castsShadows(_toggleButtons[TOGGLE_SHADOW_MAPPING]->isSelected());
    }
    return true;
}

bool GUIEditor::Handle_FogToggle(const CEGUI::EventArgs &e) {
    if (_toggleButtons[TOGGLE_FOG]->isSelected() ) {
        Console::d_printfn("[Editor]: Fog enabled!");
    } else {
        Console::d_printfn("[Editor]: Fog disabled!");
    }

    ParamHandler::getInstance().setParam("rendering.enableFog", _toggleButtons[TOGGLE_FOG]->isSelected());

    return true;
}

bool GUIEditor::Handle_PostFXToggle(const CEGUI::EventArgs &e) {
    if ( _toggleButtons[TOGGLE_POST_FX]->isSelected() ) {
        Console::d_printfn("[Editor]: PostFX enabled!");
    } else {
        Console::d_printfn("[Editor]: PostFX disabled!");
    }
    GFX_DEVICE.postProcessingEnabled(_toggleButtons[TOGGLE_POST_FX]->isSelected());
    return true;
}

bool GUIEditor::Handle_BoundingBoxesToggle(const CEGUI::EventArgs &e) {
    if ( _toggleButtons[TOGGLE_BOUNDING_BOXES]->isSelected() ) {
        Console::d_printfn("[Editor]: Bounding Box rendering enabled!");
    } else {
        Console::d_printfn("[Editor]: Bounding Box rendering disabled!");
    }
    if (_currentSelection) {
        RenderingComponent* rComp = _currentSelection->getComponent<RenderingComponent>();
        rComp->renderBoundingBox(_toggleButtons[TOGGLE_BOUNDING_BOXES]->isSelected());
    }
    return true;
}

bool GUIEditor::Handle_DrawNavMeshToggle(const CEGUI::EventArgs &e) {
    if ( _toggleButtons[TOGGLE_NAV_MESH_DRAW]->isSelected() ) {
        Console::d_printfn("[Editor]: NavMesh rendering enabled!");
    } else {
        Console::d_printfn("[Editor]: NavMesh rendering disabled!");
    }
    AI::AIManager::getInstance().toggleNavMeshDebugDraw(_toggleButtons[TOGGLE_NAV_MESH_DRAW]->isSelected());
    return true;
}

bool GUIEditor::Handle_SkeletonsToggle(const CEGUI::EventArgs &e) {
    if ( _toggleButtons[TOGGLE_SKELETONS]->isSelected() ) {
        Console::d_printfn("[Editor]: Skeleton rendering enabled!");
    } else {
        Console::d_printfn("[Editor]: Skeleton rendering disabled!");
    }

    if (_currentSelection) {
        _currentSelection->getComponent<RenderingComponent>()->renderSkeleton(_toggleButtons[TOGGLE_SKELETONS]->isSelected());
    }
    return true;
}

bool GUIEditor::Handle_PositionXChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position X changed via text edit!");
    F32 x = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_POSITION][CONTROL_FIELD_X]->getText());
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_X] = x;
    _currentSelection->getComponent<PhysicsComponent>()->setPositionX(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_X]);
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_PositionYChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position Y changed via text edit!");
    F32 y = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_POSITION][CONTROL_FIELD_Y]->getText());
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Y] = y;
    _currentSelection->getComponent<PhysicsComponent>()->setPositionY(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Y]);
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_PositionZChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position Z changed via text edit!");
    F32 z = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_POSITION][CONTROL_FIELD_Z]->getText());
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Z] = z;
    _currentSelection->getComponent<PhysicsComponent>()->setPositionZ(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Z]);
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_PositionGranularityChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position Granularity changed via text edit!");
    F32 gran = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY]->getText());
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY] = gran;
    CLAMP<F32>(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY], 0.0000001f, 10000000.0f);
    CEGUI::String granStr = CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY]);
    _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY]->setText(granStr);
    return true;
}

bool GUIEditor::Handle_RotationXChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Rotation X changed via text edit!");
    F32 x = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_X]->getText());
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X] = x;
    CLAMP<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X], -360.0f, 360.0f);
    CEGUI::String xStr = CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X]);
    _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_X]->setText(xStr);
    PhysicsComponent* pComp = _currentSelection->getComponent<PhysicsComponent>();
    pComp->setRotation(vec3<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X],
                                 _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y],
                                 _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z]));
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_RotationYChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Rotation Y changed via text edit!");
    F32 y = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_Y]->getText());
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y] = y;
    CLAMP<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y], -360.0f, 360.0f);
    CEGUI::String yStr = CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y]);
    _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_Y]->setText(yStr);
    PhysicsComponent* pComp = _currentSelection->getComponent<PhysicsComponent>();
    pComp->setRotation(vec3<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X],
                                 _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y],
                                 _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z]));
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_RotationZChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position X changed via text edit!");
    F32 z = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_Z]->getText());
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z] = z;
    CLAMP<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z], -360.0f, 360.0f);
    CEGUI::String zStr = CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z]);
    _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_Z]->setText(zStr);
    PhysicsComponent* pComp = _currentSelection->getComponent<PhysicsComponent>();
    pComp->setRotation(vec3<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X],
                                 _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y],
                                 _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z]));
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_RotationGranularityChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Rotation Granularity changed via text edit!");
    F32 gran = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]->getText());
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY] = gran;
    CLAMP<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY], 0.0000001f, 359.0f);
    CEGUI::String granStr = CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]);
    _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]->setText(granStr);
    return true;
}

bool GUIEditor::Handle_ScaleXChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale X changed via text edit!");
    F32 x = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_SCALE][CONTROL_FIELD_X]->getText());
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_X] = x;
    _currentSelection->getComponent<PhysicsComponent>()->setScaleX(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_X]);
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_ScaleYChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale Y changed via text edit!");
    F32 y = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_SCALE][CONTROL_FIELD_Y]->getText());
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Y] = y;
    _currentSelection->getComponent<PhysicsComponent>()->setScaleY(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Y]);
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_ScaleZChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale Z changed via text edit!");
    F32 z = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_SCALE][CONTROL_FIELD_Z]->getText());
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Z] = z;
    _currentSelection->getComponent<PhysicsComponent>()->setScaleZ(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Z]);
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_ScaleGranularityChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale Granularity changed via text edit!");
    F32 gran = CEGUI::PropertyHelper<F32>::fromString(_valuesField[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY]->getText());
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY] = gran;
    CLAMP<F32>(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY], 0.0000001f, 10000000.0f);
    CEGUI::String granStr = CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY]);
    _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY]->setText(granStr);
    return true;
}

bool GUIEditor::Handle_IncrementPositionX(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position X incremented via button!");
    // If we don't have a selection, the button is disabled
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_X] += _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setPositionX(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_X]);
    return true;
}

bool GUIEditor::Handle_DecrementPositionX(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position X decremented via button!");
    // If we don't have a selection, the button is disabled
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_X] -= _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setPositionX(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_X]);
    return true;
}

bool GUIEditor::Handle_IncrementPositionY(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position Y incremented via button!");
    // If we don't have a selection, the button is disabled
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Y] += _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setPositionY(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Y]);
    return true;
}

bool GUIEditor::Handle_DecrementPositionY(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position Y decremented via button!");
    // If we don't have a selection, the button is disabled
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Y] -= _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setPositionY(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Y]);
    return true;
}

bool GUIEditor::Handle_IncrementPositionZ(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position Z incremented via button!");
    // If we don't have a selection, the button is disabled
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Z] += _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setPositionZ(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Z]);
    return true;
}

bool GUIEditor::Handle_DecrementPositionZ(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position Z decremented via button!");
    // If we don't have a selection, the button is disabled
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Z] -= _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setPositionZ(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_Z]);
    return true;
}

bool GUIEditor::Handle_IncrementPositionGranularity(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position Granularity incremented via button!");
    F32 pos = std::min(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY] * 10.0f, 10000000.0f);
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY] = pos;
    CEGUI::String posStr = CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY]);
    _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY]->setText(posStr);
    return true;
}

bool GUIEditor::Handle_DecrementPositionGranularity(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position Granularity decremented via button!");
    F32 pos = std::max(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY] / 10.0f, 0.0000001f);
    _currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY] = pos;
    CEGUI::String posStr = CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY]);
    _valuesField[TRANSFORM_POSITION][CONTROL_FIELD_GRANULARITY]->setText(posStr);
    return true;
}

bool GUIEditor::Handle_IncrementRotationX(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Rotation X incremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X] += _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY];
    PhysicsComponent* pComp = _currentSelection->getComponent<PhysicsComponent>();
    pComp->setRotation(vec3<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X],
                                 _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y],
                                 _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z]));
    return true;
}

bool GUIEditor::Handle_DecrementRotationX(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Rotation X decremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X] -= _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY];
    PhysicsComponent* pComp = _currentSelection->getComponent<PhysicsComponent>();
    pComp->setRotation(vec3<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X],
                                 _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y],
                                 _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z]));
    return true;
}

bool GUIEditor::Handle_IncrementRotationY(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Rotation Y incremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y] += _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY];
    PhysicsComponent* pComp = _currentSelection->getComponent<PhysicsComponent>();
    pComp->setRotation(vec3<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X],
                                 _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y],
                                 _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z]));
    return true;
}

bool GUIEditor::Handle_DecrementRotationY(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Rotation Y decremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y] -= _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY];
    PhysicsComponent* pComp = _currentSelection->getComponent<PhysicsComponent>();
    pComp->setRotation(vec3<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X],
                                 _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y],
                                 _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z]));
    return true;
}

bool GUIEditor::Handle_IncrementRotationZ(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Rotation Z incremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z] += _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY];
    PhysicsComponent* pComp = _currentSelection->getComponent<PhysicsComponent>();
    pComp->setRotation(vec3<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X],
                                 _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y],
                                 _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z]));
    return true;
}

bool GUIEditor::Handle_DecrementRotationZ(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Rotation Z decremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z] -= _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY];
    PhysicsComponent* pComp = _currentSelection->getComponent<PhysicsComponent>();
    pComp->setRotation(vec3<F32>(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_X],
                                 _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Y],
                                 _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_Z]));
    return true;
}

bool GUIEditor::Handle_IncrementRotationGranularity(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Rotation Granularity incremented via button!");
    F32 gran = std::min(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY] * 10.0f, 359.0f);
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY] = gran;
    CEGUI::String granStr = CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]);
    _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]->setText(granStr);
    return true;
}

bool GUIEditor::Handle_DecrementRotationGranularity(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Rotation Granularity decremented via button!");
    F32 gran = std::max(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY] / 10.0f, 0.0000001f);
    _currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY] = gran;
    CEGUI::String granStr = CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]);
    _valuesField[TRANSFORM_ROTATION][CONTROL_FIELD_GRANULARITY]->setText(granStr);
    return true;
}

bool GUIEditor::Handle_IncrementScaleX(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale X incremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_X] += _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setScaleX(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_X]);
    return true;
}

bool GUIEditor::Handle_DecrementScaleX(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale X decremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_X] -= _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setScaleX(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_X]);
    return true;
}

bool GUIEditor::Handle_IncrementScaleY(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale Y incremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Y] += _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setScaleY(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Y]);
    return true;
}

bool GUIEditor::Handle_DecrementScaleY(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale Y decremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Y] -= _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setScaleY(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Y]);
    return true;
}

bool GUIEditor::Handle_IncrementScaleZ(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale Z incremented via button!");
    assert(_currentSelection != nullptr);
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Z] += _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setScaleZ(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Z]);
    return true;
}

bool GUIEditor::Handle_DecrementScaleZ(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale Z decremented via button!");
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Z] -= _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY];
    _currentSelection->getComponent<PhysicsComponent>()->setScaleZ(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_Z]);
    return true;
}

bool GUIEditor::Handle_IncrementScaleGranularity(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale Granularity incremented via button!");
    F32 gran = std::min(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY] * 10.0f, 10000000.0f);
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY] = gran;
    CEGUI::String granStr = CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY]);
    _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY]->setText(granStr);
    return true;
}

bool GUIEditor::Handle_DecrementScaleGranularity(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale Granularity decremented via button!");
    F32 gran = std::max(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY] / 10.0f, 0.0000001f);
    _currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY] = gran;
    CEGUI::String granStr = CEGUI::PropertyHelper<F32>::toString(_currentValues[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY]);
    _valuesField[TRANSFORM_SCALE][CONTROL_FIELD_GRANULARITY]->setText(granStr);
    return true;
}

}; //namespace Divide