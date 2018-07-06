#include "stdafx.h"

#include "Headers/GUIEditor.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIConsole.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "AI/PathFinding/NavMeshes/Headers/NavMesh.h"

#include <CEGUI/CEGUI.h>

namespace Divide {

GUIEditor::GUIEditor(PlatformContext& context, ResourceCache& cache)
    : _context(context),
      _resCache(cache),
      _init(false),
      _wasControlClick(false),
      _createNavMeshQueued(false),
      _pauseSelectionTracking(false),
      _editorWindow(nullptr)
{
    _saveSelectionButton = nullptr;
    _deleteSelectionButton = nullptr;

    U32 transFieldCount =
        to_base(TransformFields::COUNT);

    _toggleButtons.fill(0);

    for (auto& internalArray : _currentValues) {
        internalArray.fill(0.0f);
    }

    for (auto& internalArray : _valuesField) {
        internalArray.fill(nullptr);
    }

    for (auto& internalArray : _transformButtonsInc) {
        internalArray.fill(nullptr);
    }

    for (auto& internalArray : _transformButtonsDec) {
        internalArray.fill(nullptr);
    }

    for (U32 i = 0; i < transFieldCount; ++i) {
        _currentValues[i][to_U32(
            ControlFields::CONTROL_FIELD_GRANULARITY)] = 1.0;
    }
}

GUIEditor::~GUIEditor() {
    _init = false;
}

bool GUIEditor::init() {
    if (_init) {
        Console::errorfn(Locale::get(_ID("ERROR_EDITOR_DOUBLE_INIT")));
        return false;
    }

    Console::printfn(Locale::get(_ID("GUI_EDITOR_INIT")));
    // Get a local pointer to the CEGUI Window Manager, Purely for convenience
    // to reduce typing
    CEGUI::WindowManager *pWindowManager =
        CEGUI::WindowManager::getSingletonPtr();
    // load the editor Window from the layout file
    const stringImpl &layoutFile = _context.config().gui.editorLayoutFile;
    _editorWindow = pWindowManager->loadLayoutFromFile(layoutFile.c_str());

    if (_editorWindow) {
        // Add the Window to the GUI Root Sheet
        CEGUI_DEFAULT_CTX.getRootWindow()->addChild(_editorWindow);
        // Now register the handlers for the events (Clicking, typing, etc)
        RegisterHandlers();
    } else {
        // Loading layout from file, failed, so output an error Message.
        CEGUI::Logger::getSingleton().logEvent(
            "Error: Unable to load the Editor from .layout");
        Console::errorfn(Locale::get(_ID("ERROR_EDITOR_LAYOUT_FILE")),
                         layoutFile.c_str());
        return false;
    }

    setVisible(false);

    _init = true;
    Console::printfn(Locale::get(_ID("GUI_EDITOR_CREATED")));
    return true;
}

bool GUIEditor::Handle_ChangeSelection(SceneGraphNode_wptr newNode) {
    _currentSelection = newNode;
    if (isVisible()) {
        UpdateControls();
    }
    return true;
}

void GUIEditor::TrackSelection() {
    SceneGraphNode_ptr node = _currentSelection.lock();

    if (node && !_pauseSelectionTracking) {
        PhysicsComponent *const selectionTransform =
            node->get<PhysicsComponent>();
        const vec3<F32> &localPosition = selectionTransform->getPosition();
        const vec3<F32> &localScale = selectionTransform->getScale();
        vec3<F32> localOrientation(Angle::to_DEGREES(GetEuler(selectionTransform->getOrientation())));

        currentValues(TransformFields::TRANSFORM_POSITION,
                      ControlFields::CONTROL_FIELD_X) = localPosition.x;
        currentValues(TransformFields::TRANSFORM_POSITION,
                      ControlFields::CONTROL_FIELD_Y) = localPosition.y;
        currentValues(TransformFields::TRANSFORM_POSITION,
                      ControlFields::CONTROL_FIELD_Z) = localPosition.z;

        currentValues(TransformFields::TRANSFORM_ROTATION,
                      ControlFields::CONTROL_FIELD_X) = localOrientation.x;
        currentValues(TransformFields::TRANSFORM_ROTATION,
                      ControlFields::CONTROL_FIELD_Y) = localOrientation.y;
        currentValues(TransformFields::TRANSFORM_ROTATION,
                      ControlFields::CONTROL_FIELD_Z) = localOrientation.z;

        currentValues(TransformFields::TRANSFORM_SCALE,
                      ControlFields::CONTROL_FIELD_X) = localScale.x;
        currentValues(TransformFields::TRANSFORM_SCALE,
                      ControlFields::CONTROL_FIELD_Y) = localScale.y;
        currentValues(TransformFields::TRANSFORM_SCALE,
                      ControlFields::CONTROL_FIELD_Z) = localScale.z;

        for (U32 i = 0;
             i < to_base(TransformFields::COUNT);
             ++i) {
            // Skip granularity
            for (U32 j = 0;
                 j < to_base(ControlFields::COUNT) - 1;
                 ++j) {
                _valuesField[i][j]->setText(
                    CEGUI::PropertyHelper<F32>::toString(_currentValues[i][j]));
            }
        }

        RenderingComponent *rComp =  node->get<RenderingComponent>();
        toggleButton(ToggleButtons::TOGGLE_SKELETONS)->setSelected(rComp->renderOptionEnabled(RenderingComponent::RenderOptions::RENDER_SKELETON));
        toggleButton(ToggleButtons::TOGGLE_SHADOW_MAPPING)->setSelected(rComp->renderOptionEnabled(RenderingComponent::RenderOptions::CAST_SHADOWS));
        toggleButton(ToggleButtons::TOGGLE_WIREFRAME)->setSelected(rComp->renderOptionEnabled(RenderingComponent::RenderOptions::RENDER_WIREFRAME));
        toggleButton(ToggleButtons::TOGGLE_BOUNDING_BOXES)->setSelected(rComp->renderOptionEnabled(RenderingComponent::RenderOptions::RENDER_BOUNDS_AABB));
    }
}

void GUIEditor::UpdateControls() {
    SceneGraphNode_ptr node = _currentSelection.lock();

    bool hasValidTransform = false;
    if (node) {
        hasValidTransform = (node->get<PhysicsComponent>() != nullptr);
        toggleButton(ToggleButtons::TOGGLE_WIREFRAME)->setEnabled(true);
        toggleButton(ToggleButtons::TOGGLE_BOUNDING_BOXES)->setEnabled(true);
        toggleButton(ToggleButtons::TOGGLE_SKELETONS)->setEnabled(true);
        toggleButton(ToggleButtons::TOGGLE_SHADOW_MAPPING)->setEnabled(_context.gfx().shadowDetailLevel() != RenderDetailLevel::OFF);
    }

    if (!hasValidTransform) {
        for (U32 i = 0;
             i < to_base(TransformFields::COUNT);
             ++i) {
            // Skip granularity
            for (U32 j = 0;
                 j < to_base(ControlFields::COUNT) - 1;
                 ++j) {
                _valuesField[i][j]->setText("N/A");
                _valuesField[i][j]->setEnabled(false);
                _transformButtonsInc[i][j]->setEnabled(false);
                _transformButtonsDec[i][j]->setEnabled(false);
            }
        }
        _deleteSelectionButton->setEnabled(false);
        _saveSelectionButton->setEnabled(false);
        toggleButton(ToggleButtons::TOGGLE_WIREFRAME)->setEnabled(false);
        toggleButton(ToggleButtons::TOGGLE_WIREFRAME)->setSelected(false);
        toggleButton(ToggleButtons::TOGGLE_BOUNDING_BOXES)->setEnabled(false);
        toggleButton(ToggleButtons::TOGGLE_BOUNDING_BOXES)->setSelected(false);
        toggleButton(ToggleButtons::TOGGLE_SKELETONS)->setEnabled(false);
        toggleButton(ToggleButtons::TOGGLE_SKELETONS)->setSelected(false);
        toggleButton(ToggleButtons::TOGGLE_SHADOW_MAPPING)->setEnabled(false);
        toggleButton(ToggleButtons::TOGGLE_SHADOW_MAPPING)->setSelected(false);
    } else {
        for (U32 i = 0;
             i < to_base(TransformFields::COUNT);
             ++i) {
            // Skip granularity
            for (U32 j = 0;
                 j < to_base(ControlFields::COUNT) - 1;
                 ++j) {
                _valuesField[i][j]->setText(
                    CEGUI::PropertyHelper<F32>::toString(_currentValues[i][j]));
                _valuesField[i][j]->setEnabled(true);
                _transformButtonsInc[i][j]->setEnabled(true);
                _transformButtonsDec[i][j]->setEnabled(true);
            }
        }
        _deleteSelectionButton->setEnabled(true);
        _saveSelectionButton->setEnabled(true);
    }
    toggleButton(ToggleButtons::TOGGLE_POST_FX)
        ->setSelected(false);
    toggleButton(ToggleButtons::TOGGLE_POST_FX)
        ->setEnabled(false);
    toggleButton(ToggleButtons::TOGGLE_FOG)
        ->setSelected(_context.config().rendering.enableFog);

    toggleButton(ToggleButtons::TOGGLE_DEPTH_PREVIEW)
        ->setSelected(false);
    toggleButton(ToggleButtons::TOGGLE_DEPTH_PREVIEW)
        ->setEnabled(false);
}

void GUIEditor::setVisible(bool visible) {
    UpdateControls();
    _editorWindow->setVisible(visible);
}

bool GUIEditor::isVisible() const {
    return _editorWindow->isVisible();
}

bool GUIEditor::update(const U64 deltaTime) {
    _wasControlClick = false;
    bool state = true;
    if (_createNavMeshQueued) {
        Scene& activeScene = *_context.gui().activeScene();
        AI::AIManager& aiManager = _context.gui().activeScene()->aiManager();

        state = false;
        // Check if we already have a NavMesh created
        AI::Navigation::NavigationMesh *temp = aiManager.getNavMesh(AI::AIEntity::PresetAgentRadius::AGENT_RADIUS_SMALL);
        // Check debug rendering status
        aiManager.toggleNavMeshDebugDraw(toggleButton(ToggleButtons::TOGGLE_NAV_MESH_DRAW)->isSelected());
        // Create a new NavMesh if we don't currently have one
        if (!temp) {
            temp = MemoryManager_NEW AI::Navigation::NavigationMesh(_context);
        }
        // Set it's file name
        temp->setFileName(activeScene.getName());
        // Try to load it from file
        bool loaded = temp->load(activeScene.sceneGraph().getRoot());
        if (!loaded) {
            // If we failed to load it from file, we need to build it first
            loaded = temp->build(
                activeScene.sceneGraph().getRoot(),
                AI::Navigation::NavigationMesh::CreationCallback(), false);
            // Then save it to file
            temp->save(activeScene.sceneGraph().getRoot());
        }
        // If we loaded/built the NavMesh correctly, add it to the AIManager
        if (loaded) {
            state = aiManager.addNavMesh(AI::AIEntity::PresetAgentRadius::AGENT_RADIUS_SMALL, temp);
        }

        _createNavMeshQueued = false;
    }

    if (isVisible()) {
        TrackSelection();
    }
    return state;
}

void GUIEditor::RegisterHandlers() {
    CEGUI::Window *EditorBar =
        static_cast<CEGUI::Window *>(_editorWindow->getChild("MenuBar"));
    EditorBar->subscribeEvent(
        CEGUI::Window::EventMouseButtonUp,
        CEGUI::Event::Subscriber(&GUIEditor::Handle_MenuBarClickOn, this));

    for (U16 i = 0; i < EditorBar->getChildCount(); ++i) {
        EditorBar->getChildElementAtIdx(i)->subscribeEvent(
            CEGUI::Window::EventMouseButtonUp,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_MenuBarClickOn, this));
    }

    // Save Scene Button
    {
        CEGUI::Window *saveSceneButton = static_cast<CEGUI::Window *>(
            EditorBar->getChild("SaveSceneButton"));
        saveSceneButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_SaveScene, this));
    }
    // Save Selection Button
    {
        _saveSelectionButton = static_cast<CEGUI::Window *>(
            EditorBar->getChild("SaveSelectionButton"));
        _saveSelectionButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_SaveSelection, this));
    }
    // Delete Selection Button
    {
        _deleteSelectionButton = static_cast<CEGUI::Window *>(
            EditorBar->getChild("DeleteSelection"));
        _deleteSelectionButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_DeleteSelection, this));
    }
    // Generate NavMesh button
    {
        CEGUI::Window *createNavMeshButton =
            static_cast<CEGUI::Window *>(EditorBar->getChild("NavMeshButton"));
        createNavMeshButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_CreateNavMesh, this));
    }
    // Reload Scene Button
    {
        CEGUI::Window *reloadSceneButton = static_cast<CEGUI::Window *>(
            EditorBar->getChild("ReloadSceneButton"));
        reloadSceneButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_ReloadScene, this));
    }
    // Print SceneGraph Button
    {
        CEGUI::Window *dumpSceneGraphButton = static_cast<CEGUI::Window *>(
            EditorBar->getChild("DumpSceneGraphTree"));
        dumpSceneGraphButton->setEnabled(false);
    }
    CEGUI::ToggleButton *toggleButtonPtr = nullptr;
    // Toggle Wireframe rendering
    {
        toggleButtonPtr = static_cast<CEGUI::ToggleButton *>(
            EditorBar->getChild("ToggleWireframe"));
        toggleButton(ToggleButtons::TOGGLE_WIREFRAME) = toggleButtonPtr;
        toggleButtonPtr->subscribeEvent(
            CEGUI::ToggleButton::EventSelectStateChanged,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_WireframeToggle, this));
    }
    // Toggle Depth Preview
    {
        toggleButtonPtr = static_cast<CEGUI::ToggleButton *>(
            EditorBar->getChild("ToggleDepthPreview"));
        toggleButton(ToggleButtons::TOGGLE_DEPTH_PREVIEW) = toggleButtonPtr;
        toggleButtonPtr->subscribeEvent(
            CEGUI::ToggleButton::EventSelectStateChanged,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_DepthPreviewToggle,
                                     this));
    }
    // Toggle Shadows
    {
        toggleButtonPtr = static_cast<CEGUI::ToggleButton *>(
            EditorBar->getChild("ToggleShadowMapping"));
        toggleButton(ToggleButtons::TOGGLE_SHADOW_MAPPING) = toggleButtonPtr;
        toggleButtonPtr->subscribeEvent(
            CEGUI::ToggleButton::EventSelectStateChanged,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_ShadowMappingToggle,
                                     this));
    }
    // Toggle Fog
    {
        toggleButtonPtr = static_cast<CEGUI::ToggleButton *>(
            EditorBar->getChild("ToggleFog"));
        toggleButton(ToggleButtons::TOGGLE_FOG) = toggleButtonPtr;
        toggleButtonPtr->subscribeEvent(
            CEGUI::ToggleButton::EventSelectStateChanged,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_FogToggle, this));
    }
    // Toggle PostFX
    {
        toggleButtonPtr = static_cast<CEGUI::ToggleButton *>(
            EditorBar->getChild("TogglePostFX"));
        toggleButton(ToggleButtons::TOGGLE_POST_FX) = toggleButtonPtr;
        toggleButtonPtr->subscribeEvent(
            CEGUI::ToggleButton::EventSelectStateChanged,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_PostFXToggle, this));
    }
    // Toggle Bounding Boxes
    {
        toggleButtonPtr = static_cast<CEGUI::ToggleButton *>(
            EditorBar->getChild("ToggleBoundingBoxes"));
        toggleButton(ToggleButtons::TOGGLE_BOUNDING_BOXES) = toggleButtonPtr;
        toggleButtonPtr->subscribeEvent(
            CEGUI::ToggleButton::EventSelectStateChanged,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_BoundingBoxesToggle,
                                     this));
    }
    // Toggle NavMesh Rendering
    {
        toggleButtonPtr = static_cast<CEGUI::ToggleButton *>(
            EditorBar->getChild("DebugDraw"));
        toggleButton(ToggleButtons::TOGGLE_NAV_MESH_DRAW) = toggleButtonPtr;
        toggleButtonPtr->subscribeEvent(
            CEGUI::ToggleButton::EventSelectStateChanged,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_DrawNavMeshToggle,
                                     this));
    }
    // Toggle Skeleton Rendering
    {
        toggleButtonPtr = static_cast<CEGUI::ToggleButton *>(
            EditorBar->getChild("ToggleSkeletons"));
        toggleButton(ToggleButtons::TOGGLE_SKELETONS) = toggleButtonPtr;
        toggleButtonPtr->subscribeEvent(
            CEGUI::ToggleButton::EventSelectStateChanged,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_SkeletonsToggle, this));
    }

    CEGUI::Editbox *field = nullptr;
    // Type new X Position
    {
        valuesField(TransformFields::TRANSFORM_POSITION,
                    ControlFields::CONTROL_FIELD_X) =
            static_cast<CEGUI::Editbox *>(
                EditorBar->getChild("XPositionValue"));
        field = valuesField(TransformFields::TRANSFORM_POSITION,
                            ControlFields::CONTROL_FIELD_X);
        field->subscribeEvent(
            CEGUI::Editbox::EventTextAccepted,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_PositionXChange, this));
        field->subscribeEvent(
            CEGUI::Editbox::EventInputCaptureGained,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        field->setValidationString(
            CEGUI::String("^[-+]?[0-9]{1,10}([.,0-9]{1,10})?$"));
    }
    // Type new Y Position
    {
        valuesField(TransformFields::TRANSFORM_POSITION,
                    ControlFields::CONTROL_FIELD_Y) =
            static_cast<CEGUI::Editbox *>(
                EditorBar->getChild("YPositionValue"));
        field = valuesField(TransformFields::TRANSFORM_POSITION,
                            ControlFields::CONTROL_FIELD_Y);
        field->subscribeEvent(
            CEGUI::Editbox::EventTextAccepted,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_PositionYChange, this));
        field->subscribeEvent(
            CEGUI::Editbox::EventInputCaptureGained,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        field->setValidationString(
            CEGUI::String("^[-+]?[0-9]{1,10}([.,0-9]{1,10})?$"));
    }
    // Type new Z Position
    {
        valuesField(TransformFields::TRANSFORM_POSITION,
                    ControlFields::CONTROL_FIELD_Z) =
            static_cast<CEGUI::Editbox *>(
                EditorBar->getChild("ZPositionValue"));
        field = valuesField(TransformFields::TRANSFORM_POSITION,
                            ControlFields::CONTROL_FIELD_Z);
        field->subscribeEvent(
            CEGUI::Editbox::EventTextAccepted,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_PositionZChange, this));
        field->subscribeEvent(
            CEGUI::Editbox::EventInputCaptureGained,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        field->setValidationString(
            CEGUI::String("^[-+]?[0-9]{1,10}([.,0-9]{1,10})?$"));
    }

    // Type new Position Granularity
    {
        valuesField(TransformFields::TRANSFORM_POSITION,
                    ControlFields::CONTROL_FIELD_GRANULARITY) =
            static_cast<CEGUI::Editbox *>(
                EditorBar->getChild("PositionGranularityValue"));
        field = valuesField(TransformFields::TRANSFORM_POSITION,
                            ControlFields::CONTROL_FIELD_GRANULARITY);
        field->subscribeEvent(
            CEGUI::Editbox::EventTextAccepted,
            CEGUI::Event::Subscriber(
                &GUIEditor::Handle_PositionGranularityChange, this));
        field->setText(CEGUI::PropertyHelper<F32>::toString(
            currentValues(TransformFields::TRANSFORM_POSITION,
                          ControlFields::CONTROL_FIELD_GRANULARITY)));
        field->setValidationString(
            CEGUI::String("^[-+]?[0-9]{1,10}([.,0-9]{1,10})?$"));
    }
    // Type new X Rotation
    {
        valuesField(TransformFields::TRANSFORM_ROTATION,
                    ControlFields::CONTROL_FIELD_X) =
            static_cast<CEGUI::Editbox *>(
                EditorBar->getChild("XRotationValue"));
        field = valuesField(TransformFields::TRANSFORM_ROTATION,
                            ControlFields::CONTROL_FIELD_X);
        field->subscribeEvent(
            CEGUI::Editbox::EventTextAccepted,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_RotationXChange, this));
        field->subscribeEvent(
            CEGUI::Editbox::EventInputCaptureGained,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        field->setValidationString(
            CEGUI::String("^[-+]?[0-9]{1,10}([.,0-9]{1,10})?$"));
    }
    // Type new Y Rotation
    {
        valuesField(TransformFields::TRANSFORM_ROTATION,
                    ControlFields::CONTROL_FIELD_Y) =
            static_cast<CEGUI::Editbox *>(
                EditorBar->getChild("YRotationValue"));
        field = valuesField(TransformFields::TRANSFORM_ROTATION,
                            ControlFields::CONTROL_FIELD_Y);
        field->subscribeEvent(
            CEGUI::Editbox::EventTextAccepted,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_RotationYChange, this));
        field->subscribeEvent(
            CEGUI::Editbox::EventInputCaptureGained,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        field->setValidationString(
            CEGUI::String("^[-+]?[0-9]{1,10}([.,0-9]{1,10})?$"));
    }
    // Type new Z Rotation
    {
        valuesField(TransformFields::TRANSFORM_ROTATION,
                    ControlFields::CONTROL_FIELD_Z) =
            static_cast<CEGUI::Editbox *>(
                EditorBar->getChild("ZRotationValue"));
        field = valuesField(TransformFields::TRANSFORM_ROTATION,
                            ControlFields::CONTROL_FIELD_Z);
        field->subscribeEvent(
            CEGUI::Editbox::EventTextAccepted,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_RotationZChange, this));
        field->subscribeEvent(
            CEGUI::Editbox::EventInputCaptureGained,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        field->setValidationString(
            CEGUI::String("^[-+]?[0-9]{1,10}([.,0-9]{1,10})?$"));
    }
    // Type new Rotation Granularity
    {
        valuesField(TransformFields::TRANSFORM_ROTATION,
                    ControlFields::CONTROL_FIELD_GRANULARITY) =
            static_cast<CEGUI::Editbox *>(
                EditorBar->getChild("RotationGranularityValue"));
        field = valuesField(TransformFields::TRANSFORM_ROTATION,
                            ControlFields::CONTROL_FIELD_GRANULARITY);
        field->subscribeEvent(
            CEGUI::Editbox::EventTextAccepted,
            CEGUI::Event::Subscriber(
                &GUIEditor::Handle_RotationGranularityChange, this));
        field->setText(CEGUI::PropertyHelper<F32>::toString(
            currentValues(TransformFields::TRANSFORM_ROTATION,
                          ControlFields::CONTROL_FIELD_GRANULARITY)));
        field->setValidationString(
            CEGUI::String("^[-+]?[0-9]{1,10}([.,0-9]{1,10})?$"));
    }
    // Type new X Scale
    {
        valuesField(TransformFields::TRANSFORM_SCALE,
                    ControlFields::CONTROL_FIELD_X) =
            static_cast<CEGUI::Editbox *>(EditorBar->getChild("XScaleValue"));
        field = valuesField(TransformFields::TRANSFORM_SCALE,
                            ControlFields::CONTROL_FIELD_X);
        field->subscribeEvent(
            CEGUI::Editbox::EventTextAccepted,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_ScaleXChange, this));
        field->subscribeEvent(
            CEGUI::Editbox::EventInputCaptureGained,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        field->setValidationString(
            CEGUI::String("^[-+]?[0-9]{1,10}([.,0-9]{1,10})?$"));
    }
    // Type new Y Scale
    {
        valuesField(TransformFields::TRANSFORM_SCALE,
                    ControlFields::CONTROL_FIELD_Y) =
            static_cast<CEGUI::Editbox *>(EditorBar->getChild("YScaleValue"));
        field = valuesField(TransformFields::TRANSFORM_SCALE,
                            ControlFields::CONTROL_FIELD_Y);
        field->subscribeEvent(
            CEGUI::Editbox::EventTextAccepted,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_ScaleYChange, this));
        field->subscribeEvent(
            CEGUI::Editbox::EventInputCaptureGained,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        field->setValidationString(
            CEGUI::String("^[-+]?[0-9]{1,10}([.,0-9]{1,10})?$"));
    }
    // Type new Z Scale
    {
        valuesField(TransformFields::TRANSFORM_SCALE,
                    ControlFields::CONTROL_FIELD_Z) =
            static_cast<CEGUI::Editbox *>(EditorBar->getChild("ZScaleValue"));
        field = valuesField(TransformFields::TRANSFORM_SCALE,
                            ControlFields::CONTROL_FIELD_Z);
        field->subscribeEvent(
            CEGUI::Editbox::EventTextAccepted,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_ScaleZChange, this));
        field->subscribeEvent(
            CEGUI::Editbox::EventInputCaptureGained,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_EditFieldClick, this));
        field->setValidationString(
            CEGUI::String("^[-+]?[0-9]{1,10}([.,0-9]{1,10})?$"));
    }
    // Type new Scale Granularity
    {
        valuesField(TransformFields::TRANSFORM_SCALE,
                    ControlFields::CONTROL_FIELD_GRANULARITY) =
            static_cast<CEGUI::Editbox *>(
                EditorBar->getChild("ScaleGranularityValue"));
        field = valuesField(TransformFields::TRANSFORM_SCALE,
                            ControlFields::CONTROL_FIELD_GRANULARITY);
        field->subscribeEvent(
            CEGUI::Editbox::EventTextAccepted,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_ScaleGranularityChange,
                                     this));
        field->setText(CEGUI::PropertyHelper<F32>::toString(
            currentValues(TransformFields::TRANSFORM_SCALE,
                          ControlFields::CONTROL_FIELD_GRANULARITY)));
        field->setValidationString(
            CEGUI::String("^[-+]?[0-9]{1,10}([.,0-9]{1,10})?$"));
    }
    CEGUI::Window *transformButton = nullptr;
    // Increment X Position
    {
        transformButtonsInc(TransformFields::TRANSFORM_POSITION,
                            ControlFields::CONTROL_FIELD_X) =
            EditorBar->getChild("IncPositionXButton");
        transformButton =
            transformButtonsInc(TransformFields::TRANSFORM_POSITION,
                                ControlFields::CONTROL_FIELD_X);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementPositionX,
                                     this));
    }
    // Decrement X Position
    {
        transformButtonsDec(TransformFields::TRANSFORM_POSITION,
                            ControlFields::CONTROL_FIELD_X) =
            EditorBar->getChild("DecPositionXButton");
        transformButton =
            transformButtonsDec(TransformFields::TRANSFORM_POSITION,
                                ControlFields::CONTROL_FIELD_X);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementPositionX,
                                     this));
    }
    // Increment Y Position
    {
        transformButtonsInc(TransformFields::TRANSFORM_POSITION,
                            ControlFields::CONTROL_FIELD_Y) =
            EditorBar->getChild("IncPositionYButton");
        transformButton =
            transformButtonsInc(TransformFields::TRANSFORM_POSITION,
                                ControlFields::CONTROL_FIELD_Y);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementPositionY,
                                     this));
    }
    // Decrement Y Position
    {
        transformButtonsDec(TransformFields::TRANSFORM_POSITION,
                            ControlFields::CONTROL_FIELD_Y) =
            EditorBar->getChild("DecPositionYButton");
        transformButton =
            transformButtonsDec(TransformFields::TRANSFORM_POSITION,
                                ControlFields::CONTROL_FIELD_Y);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementPositionY,
                                     this));
    }
    // Increment Z Position
    {
        transformButtonsInc(TransformFields::TRANSFORM_POSITION,
                            ControlFields::CONTROL_FIELD_Z) =
            EditorBar->getChild("IncPositionZButton");
        transformButton =
            transformButtonsInc(TransformFields::TRANSFORM_POSITION,
                                ControlFields::CONTROL_FIELD_Z);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementPositionZ,
                                     this));
    }
    // Decrement Z Position
    {
        transformButtonsDec(TransformFields::TRANSFORM_POSITION,
                            ControlFields::CONTROL_FIELD_Z) =
            EditorBar->getChild("DecPositionZButton");
        transformButton =
            transformButtonsDec(TransformFields::TRANSFORM_POSITION,
                                ControlFields::CONTROL_FIELD_Z);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementPositionZ,
                                     this));
    }
    // Increment Position Granularity
    {
        transformButtonsInc(TransformFields::TRANSFORM_POSITION,
                            ControlFields::CONTROL_FIELD_GRANULARITY) =
            EditorBar->getChild("IncPositionGranButton");
        transformButton =
            transformButtonsInc(TransformFields::TRANSFORM_POSITION,
                                ControlFields::CONTROL_FIELD_GRANULARITY);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(
                &GUIEditor::Handle_IncrementPositionGranularity, this));
    }
    // Decrement Position Granularity
    {
        transformButtonsDec(TransformFields::TRANSFORM_ROTATION,
                            ControlFields::CONTROL_FIELD_GRANULARITY) =
            EditorBar->getChild("DecPositionGranButton");
        transformButton =
            transformButtonsDec(TransformFields::TRANSFORM_ROTATION,
                                ControlFields::CONTROL_FIELD_GRANULARITY);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(
                &GUIEditor::Handle_DecrementPositionGranularity, this));
    }
    // Increment X Rotation
    {
        transformButtonsInc(TransformFields::TRANSFORM_ROTATION,
                            ControlFields::CONTROL_FIELD_X) =
            EditorBar->getChild("IncRotationXButton");
        transformButton =
            transformButtonsInc(TransformFields::TRANSFORM_ROTATION,
                                ControlFields::CONTROL_FIELD_X);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementRotationX,
                                     this));
    }
    // Decrement X Rotation
    {
        transformButtonsDec(TransformFields::TRANSFORM_ROTATION,
                            ControlFields::CONTROL_FIELD_X) =
            EditorBar->getChild("DecRotationXButton");
        transformButton =
            transformButtonsDec(TransformFields::TRANSFORM_ROTATION,
                                ControlFields::CONTROL_FIELD_X);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementRotationX,
                                     this));
    }
    // Increment Y Rotation
    {
        transformButtonsInc(TransformFields::TRANSFORM_ROTATION,
                            ControlFields::CONTROL_FIELD_Y) =
            EditorBar->getChild("IncRotationYButton");
        transformButton =
            transformButtonsInc(TransformFields::TRANSFORM_ROTATION,
                                ControlFields::CONTROL_FIELD_Y);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementRotationY,
                                     this));
    }
    // Decrement Y Rotation
    {
        transformButtonsDec(TransformFields::TRANSFORM_ROTATION,
                            ControlFields::CONTROL_FIELD_Y) =
            EditorBar->getChild("DecRotationYButton");
        transformButton =
            transformButtonsDec(TransformFields::TRANSFORM_ROTATION,
                                ControlFields::CONTROL_FIELD_Y);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementRotationY,
                                     this));
    }
    // Increment Z Rotation
    {
        transformButtonsInc(TransformFields::TRANSFORM_ROTATION,
                            ControlFields::CONTROL_FIELD_Z) =
            EditorBar->getChild("IncRotationZButton");
        transformButton =
            transformButtonsInc(TransformFields::TRANSFORM_ROTATION,
                                ControlFields::CONTROL_FIELD_Z);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementRotationZ,
                                     this));
    }
    // Decrement Z Rotation
    {
        transformButtonsDec(TransformFields::TRANSFORM_ROTATION,
                            ControlFields::CONTROL_FIELD_Z) =
            EditorBar->getChild("DecRotationZButton");
        transformButton =
            transformButtonsDec(TransformFields::TRANSFORM_ROTATION,
                                ControlFields::CONTROL_FIELD_Z);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementRotationZ,
                                     this));
    }
    // Increment Rotation Granularity
    {
        transformButtonsInc(TransformFields::TRANSFORM_ROTATION,
                            ControlFields::CONTROL_FIELD_GRANULARITY) =
            EditorBar->getChild("IncRotationGranButton");
        transformButton =
            transformButtonsInc(TransformFields::TRANSFORM_ROTATION,
                                ControlFields::CONTROL_FIELD_GRANULARITY);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(
                &GUIEditor::Handle_IncrementRotationGranularity, this));
    }
    // Decrement Rotation Granularity
    {
        transformButtonsDec(TransformFields::TRANSFORM_ROTATION,
                            ControlFields::CONTROL_FIELD_GRANULARITY) =
            EditorBar->getChild("DecRotationGranButton");
        transformButton =
            transformButtonsDec(TransformFields::TRANSFORM_ROTATION,
                                ControlFields::CONTROL_FIELD_GRANULARITY);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(
                &GUIEditor::Handle_DecrementRotationGranularity, this));
    }
    // Increment X Scale
    {
        transformButtonsInc(TransformFields::TRANSFORM_SCALE,
                            ControlFields::CONTROL_FIELD_X) =
            EditorBar->getChild("IncScaleXButton");
        transformButtonsInc(TransformFields::TRANSFORM_SCALE,
                            ControlFields::CONTROL_FIELD_X);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementScaleX, this));
    }
    // Decrement X Scale
    {
        transformButtonsDec(TransformFields::TRANSFORM_SCALE,
                            ControlFields::CONTROL_FIELD_X) =
            EditorBar->getChild("DecScaleXButton");
        transformButton = transformButtonsDec(TransformFields::TRANSFORM_SCALE,
                                              ControlFields::CONTROL_FIELD_X);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementScaleX, this));
    }
    // Increment Y Scale
    {
        transformButtonsInc(TransformFields::TRANSFORM_SCALE,
                            ControlFields::CONTROL_FIELD_Y) =
            EditorBar->getChild("IncScaleYButton");
        transformButton = transformButtonsInc(TransformFields::TRANSFORM_SCALE,
                                              ControlFields::CONTROL_FIELD_Y);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementScaleY, this));
    }
    // Decrement Y Scale
    {
        transformButtonsDec(TransformFields::TRANSFORM_SCALE,
                            ControlFields::CONTROL_FIELD_Y) =
            EditorBar->getChild("DecScaleYButton");
        transformButton = transformButtonsDec(TransformFields::TRANSFORM_SCALE,
                                              ControlFields::CONTROL_FIELD_Y);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementScaleY, this));
    }
    // Increment Z Scale
    {
        transformButtonsInc(TransformFields::TRANSFORM_SCALE,
                            ControlFields::CONTROL_FIELD_Z) =
            EditorBar->getChild("IncScaleZButton");
        transformButton = transformButtonsInc(TransformFields::TRANSFORM_SCALE,
                                              ControlFields::CONTROL_FIELD_Z);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementScaleZ, this));
    }
    // Decrement Z Scale
    {
        transformButtonsDec(TransformFields::TRANSFORM_SCALE,
                            ControlFields::CONTROL_FIELD_Z) =
            EditorBar->getChild("DecScaleZButton");
        transformButton = transformButtonsDec(TransformFields::TRANSFORM_SCALE,
                                              ControlFields::CONTROL_FIELD_Z);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementScaleZ, this));
    }
    // Increment Scale Granularity
    {
        transformButtonsInc(TransformFields::TRANSFORM_SCALE,
                            ControlFields::CONTROL_FIELD_GRANULARITY) =
            EditorBar->getChild("IncScaleGranButton");
        transformButton =
            transformButtonsInc(TransformFields::TRANSFORM_SCALE,
                                ControlFields::CONTROL_FIELD_GRANULARITY);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(
                &GUIEditor::Handle_IncrementScaleGranularity, this));
    }
    // Decrement Scale Granularity
    {
        transformButtonsDec(TransformFields::TRANSFORM_SCALE,
                            ControlFields::CONTROL_FIELD_GRANULARITY) =
            EditorBar->getChild("DecScaleGranButton");
        transformButton =
            transformButtonsDec(TransformFields::TRANSFORM_SCALE,
                                ControlFields::CONTROL_FIELD_GRANULARITY);
        transformButton->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(
                &GUIEditor::Handle_DecrementScaleGranularity, this));
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
    _context.gui().getConsole().setVisible(true);
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
    _context.gui().activeScene()->sceneGraph().deleteNode(_currentSelection, false);
    return true;
}

bool GUIEditor::Handle_ReloadScene(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Reloading scene!");
    return true;
}

bool GUIEditor::Handle_WireframeToggle(const CEGUI::EventArgs &e) {
    if (toggleButton(ToggleButtons::TOGGLE_WIREFRAME)->isSelected()) {
        Console::d_printfn("[Editor]: Wireframe rendering enabled!");
    } else {
        Console::d_printfn("[Editor]: Wireframe rendering disabled!");
    }
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        node->get<RenderingComponent>()->toggleRenderOption(RenderingComponent::RenderOptions::RENDER_WIREFRAME,
            toggleButton(ToggleButtons::TOGGLE_WIREFRAME)->isSelected());
    }
    return true;
}

bool GUIEditor::Handle_DepthPreviewToggle(const CEGUI::EventArgs &e) {
    if (toggleButton(ToggleButtons::TOGGLE_DEPTH_PREVIEW)->isSelected()) {
        Console::d_printfn("[Editor]: Depth Preview enabled!");
    } else {
        Console::d_printfn("[Editor]: Depth Preview disabled!");
    }

    return true;
}

bool GUIEditor::Handle_ShadowMappingToggle(const CEGUI::EventArgs &e) {
    if (toggleButton(ToggleButtons::TOGGLE_SHADOW_MAPPING)->isSelected()) {
        Console::d_printfn("[Editor]: Shadow Mapping enabled!");
    } else {
        Console::d_printfn("[Editor]: Shadow Mapping disabled!");
    }
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        node->get<RenderingComponent>()->toggleRenderOption(RenderingComponent::RenderOptions::CAST_SHADOWS,
            toggleButton(ToggleButtons::TOGGLE_SHADOW_MAPPING)->isSelected());
    }
    return true;
}

bool GUIEditor::Handle_FogToggle(const CEGUI::EventArgs &e) {
    if (toggleButton(ToggleButtons::TOGGLE_FOG)->isSelected()) {
        Console::d_printfn("[Editor]: Fog enabled!");
    } else {
        Console::d_printfn("[Editor]: Fog disabled!");
    }

    _context.config().rendering.enableFog = toggleButton(ToggleButtons::TOGGLE_FOG)->isSelected();

    return true;
}

bool GUIEditor::Handle_PostFXToggle(const CEGUI::EventArgs &e) {
    if (toggleButton(ToggleButtons::TOGGLE_POST_FX)->isSelected()) {
        Console::d_printfn("[Editor]: PostFX enabled!");
    } else {
        Console::d_printfn("[Editor]: PostFX disabled!");
    }

    return true;
}

bool GUIEditor::Handle_BoundingBoxesToggle(const CEGUI::EventArgs &e) {
    if (toggleButton(ToggleButtons::TOGGLE_BOUNDING_BOXES)->isSelected()) {
        Console::d_printfn("[Editor]: Bounding Box rendering enabled!");
    } else {
        Console::d_printfn("[Editor]: Bounding Box rendering disabled!");
    }
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        RenderingComponent *rComp = node->get<RenderingComponent>();
        rComp->toggleRenderOption(RenderingComponent::RenderOptions::RENDER_BOUNDS_AABB,
            toggleButton(ToggleButtons::TOGGLE_BOUNDING_BOXES)->isSelected());
    }
    return true;
}

bool GUIEditor::Handle_DrawNavMeshToggle(const CEGUI::EventArgs &e) {
    if (toggleButton(ToggleButtons::TOGGLE_NAV_MESH_DRAW)->isSelected()) {
        Console::d_printfn("[Editor]: NavMesh rendering enabled!");
    } else {
        Console::d_printfn("[Editor]: NavMesh rendering disabled!");
    }
    _context.gui()
        .activeScene()
        ->aiManager()
        .toggleNavMeshDebugDraw(toggleButton(ToggleButtons::TOGGLE_NAV_MESH_DRAW)->isSelected());
    return true;
}

bool GUIEditor::Handle_SkeletonsToggle(const CEGUI::EventArgs &e) {
    if (toggleButton(ToggleButtons::TOGGLE_SKELETONS)->isSelected()) {
        Console::d_printfn("[Editor]: Skeleton rendering enabled!");
    } else {
        Console::d_printfn("[Editor]: Skeleton rendering disabled!");
    }

    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        node->get<RenderingComponent>()->toggleRenderOption(RenderingComponent::RenderOptions::RENDER_SKELETON,
            toggleButton(ToggleButtons::TOGGLE_SKELETONS)->isSelected());
    }
    return true;
}

bool GUIEditor::Handle_PositionXChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position X changed via text edit!");
    F32 x = CEGUI::PropertyHelper<F32>::fromString(
        valuesField(TransformFields::TRANSFORM_POSITION,
                    ControlFields::CONTROL_FIELD_X)->getText());
    currentValues(TransformFields::TRANSFORM_POSITION,
                  ControlFields::CONTROL_FIELD_X) = x;
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        node->get<PhysicsComponent>()->setPositionX(
            currentValues(TransformFields::TRANSFORM_POSITION,
            ControlFields::CONTROL_FIELD_X));
    }
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_PositionYChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position Y changed via text edit!");
    F32 y = CEGUI::PropertyHelper<F32>::fromString(
        valuesField(TransformFields::TRANSFORM_POSITION,
                    ControlFields::CONTROL_FIELD_Y)->getText());
    currentValues(TransformFields::TRANSFORM_POSITION,
                  ControlFields::CONTROL_FIELD_Y) = y;
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        node->get<PhysicsComponent>()->setPositionY(
            currentValues(TransformFields::TRANSFORM_POSITION,
            ControlFields::CONTROL_FIELD_Y));
    }
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_PositionZChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position Z changed via text edit!");
    F32 z = CEGUI::PropertyHelper<F32>::fromString(
        valuesField(TransformFields::TRANSFORM_POSITION,
                    ControlFields::CONTROL_FIELD_Z)->getText());
    currentValues(TransformFields::TRANSFORM_POSITION,
                  ControlFields::CONTROL_FIELD_Z) = z;
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        node->get<PhysicsComponent>()->setPositionZ(
            currentValues(TransformFields::TRANSFORM_POSITION,
            ControlFields::CONTROL_FIELD_Z));
    }
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_PositionGranularityChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position Granularity changed via text edit!");
    F32 gran = CEGUI::PropertyHelper<F32>::fromString(
        valuesField(TransformFields::TRANSFORM_POSITION,
                    ControlFields::CONTROL_FIELD_GRANULARITY)->getText());
    currentValues(TransformFields::TRANSFORM_POSITION,
                  ControlFields::CONTROL_FIELD_GRANULARITY) = gran;
    CLAMP<F32>(currentValues(TransformFields::TRANSFORM_POSITION,
                             ControlFields::CONTROL_FIELD_GRANULARITY),
               0.0000001f, 10000000.0f);
    CEGUI::String granStr = CEGUI::PropertyHelper<F32>::toString(
        currentValues(TransformFields::TRANSFORM_POSITION,
                      ControlFields::CONTROL_FIELD_GRANULARITY));
    valuesField(TransformFields::TRANSFORM_POSITION,
                ControlFields::CONTROL_FIELD_GRANULARITY)->setText(granStr);
    return true;
}

bool GUIEditor::Handle_RotationXChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Rotation X changed via text edit!");
    F32 x = CEGUI::PropertyHelper<F32>::fromString(
        valuesField(TransformFields::TRANSFORM_ROTATION,
                    ControlFields::CONTROL_FIELD_X)->getText());
    currentValues(TransformFields::TRANSFORM_ROTATION,
                  ControlFields::CONTROL_FIELD_X) = x;
    CLAMP<F32>(currentValues(TransformFields::TRANSFORM_ROTATION,
                             ControlFields::CONTROL_FIELD_X),
               -360.0f, 360.0f);
    CEGUI::String xStr = CEGUI::PropertyHelper<F32>::toString(currentValues(
        TransformFields::TRANSFORM_ROTATION, ControlFields::CONTROL_FIELD_X));
    valuesField(TransformFields::TRANSFORM_ROTATION,
                ControlFields::CONTROL_FIELD_X)->setText(xStr);
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        PhysicsComponent *pComp =
            node->get<PhysicsComponent>();
        pComp->setRotation(
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_X),
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_Y),
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_Z));
    }
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_RotationYChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Rotation Y changed via text edit!");
    F32 y = CEGUI::PropertyHelper<F32>::fromString(
        valuesField(TransformFields::TRANSFORM_ROTATION,
                    ControlFields::CONTROL_FIELD_Y)->getText());
    currentValues(TransformFields::TRANSFORM_ROTATION,
                  ControlFields::CONTROL_FIELD_Y) = y;
    CLAMP<F32>(currentValues(TransformFields::TRANSFORM_ROTATION,
                             ControlFields::CONTROL_FIELD_Y),
               -360.0f, 360.0f);
    CEGUI::String yStr = CEGUI::PropertyHelper<F32>::toString(currentValues(
        TransformFields::TRANSFORM_ROTATION, ControlFields::CONTROL_FIELD_Y));
    valuesField(TransformFields::TRANSFORM_ROTATION,
                ControlFields::CONTROL_FIELD_Y)->setText(yStr);
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        PhysicsComponent *pComp =
            node->get<PhysicsComponent>();
        pComp->setRotation(
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_X),
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_Y),
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_Z));
    }
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_RotationZChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position X changed via text edit!");
    F32 z = CEGUI::PropertyHelper<F32>::fromString(
        valuesField(TransformFields::TRANSFORM_ROTATION,
                    ControlFields::CONTROL_FIELD_Z)->getText());
    currentValues(TransformFields::TRANSFORM_ROTATION,
                  ControlFields::CONTROL_FIELD_Z) = z;
    CLAMP<F32>(currentValues(TransformFields::TRANSFORM_ROTATION,
                             ControlFields::CONTROL_FIELD_Z),
               -360.0f, 360.0f);
    CEGUI::String zStr = CEGUI::PropertyHelper<F32>::toString(currentValues(
        TransformFields::TRANSFORM_ROTATION, ControlFields::CONTROL_FIELD_Z));
    valuesField(TransformFields::TRANSFORM_ROTATION,
                ControlFields::CONTROL_FIELD_Z)->setText(zStr);
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        PhysicsComponent *pComp =
            node->get<PhysicsComponent>();
        pComp->setRotation(
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_X),
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_Y),
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_Z));
    }
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_RotationGranularityChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Rotation Granularity changed via text edit!");
    F32 gran = CEGUI::PropertyHelper<F32>::fromString(
        valuesField(TransformFields::TRANSFORM_ROTATION,
                    ControlFields::CONTROL_FIELD_GRANULARITY)->getText());
    currentValues(TransformFields::TRANSFORM_ROTATION,
                  ControlFields::CONTROL_FIELD_GRANULARITY) = gran;
    CLAMP<F32>(currentValues(TransformFields::TRANSFORM_ROTATION,
                             ControlFields::CONTROL_FIELD_GRANULARITY),
               0.0000001f, 359.0f);
    CEGUI::String granStr = CEGUI::PropertyHelper<F32>::toString(
        currentValues(TransformFields::TRANSFORM_ROTATION,
                      ControlFields::CONTROL_FIELD_GRANULARITY));
    valuesField(TransformFields::TRANSFORM_ROTATION,
                ControlFields::CONTROL_FIELD_GRANULARITY)->setText(granStr);
    return true;
}

bool GUIEditor::Handle_ScaleXChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale X changed via text edit!");
    F32 x = CEGUI::PropertyHelper<F32>::fromString(
        valuesField(TransformFields::TRANSFORM_SCALE,
                    ControlFields::CONTROL_FIELD_X)->getText());
    currentValues(TransformFields::TRANSFORM_SCALE,
                  ControlFields::CONTROL_FIELD_X) = x;
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        node->get<PhysicsComponent>()->setScaleX(
            currentValues(TransformFields::TRANSFORM_SCALE,
            ControlFields::CONTROL_FIELD_X));
    }
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_ScaleYChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale Y changed via text edit!");
    F32 y = CEGUI::PropertyHelper<F32>::fromString(
        valuesField(TransformFields::TRANSFORM_SCALE,
                    ControlFields::CONTROL_FIELD_Y)->getText());
    currentValues(TransformFields::TRANSFORM_SCALE,
                  ControlFields::CONTROL_FIELD_Y) = y;
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        node->get<PhysicsComponent>()->setScaleY(
            currentValues(TransformFields::TRANSFORM_SCALE,
            ControlFields::CONTROL_FIELD_Y));
    }
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_ScaleZChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale Z changed via text edit!");
    F32 z = CEGUI::PropertyHelper<F32>::fromString(
        valuesField(TransformFields::TRANSFORM_SCALE,
                    ControlFields::CONTROL_FIELD_Z)->getText());
    currentValues(TransformFields::TRANSFORM_SCALE,
                  ControlFields::CONTROL_FIELD_Z) = z;
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        node->get<PhysicsComponent>()->setScaleZ(
            currentValues(TransformFields::TRANSFORM_SCALE,
            ControlFields::CONTROL_FIELD_Z));
    }
    _pauseSelectionTracking = false;
    return true;
}

bool GUIEditor::Handle_ScaleGranularityChange(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale Granularity changed via text edit!");
    F32 gran = CEGUI::PropertyHelper<F32>::fromString(
        valuesField(TransformFields::TRANSFORM_SCALE,
                    ControlFields::CONTROL_FIELD_GRANULARITY)->getText());
    currentValues(TransformFields::TRANSFORM_SCALE,
                  ControlFields::CONTROL_FIELD_GRANULARITY) = gran;
    CLAMP<F32>(currentValues(TransformFields::TRANSFORM_SCALE,
                             ControlFields::CONTROL_FIELD_GRANULARITY),
               0.0000001f, 10000000.0f);
    CEGUI::String granStr = CEGUI::PropertyHelper<F32>::toString(
        currentValues(TransformFields::TRANSFORM_SCALE,
                      ControlFields::CONTROL_FIELD_GRANULARITY));
    valuesField(TransformFields::TRANSFORM_SCALE,
                ControlFields::CONTROL_FIELD_GRANULARITY)->setText(granStr);
    return true;
}

bool GUIEditor::Handle_IncrementPositionX(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position X incremented via button!");
    // If we don't have a selection, the button is disabled
    currentValues(TransformFields::TRANSFORM_POSITION,
                  ControlFields::CONTROL_FIELD_X) +=
        currentValues(TransformFields::TRANSFORM_POSITION,
                      ControlFields::CONTROL_FIELD_GRANULARITY);
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        node->get<PhysicsComponent>()->setPositionX(
            currentValues(TransformFields::TRANSFORM_POSITION,
            ControlFields::CONTROL_FIELD_X));
    }
    return true;
}

bool GUIEditor::Handle_DecrementPositionX(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position X decremented via button!");
    // If we don't have a selection, the button is disabled
    currentValues(TransformFields::TRANSFORM_POSITION,
                  ControlFields::CONTROL_FIELD_X) -=
        currentValues(TransformFields::TRANSFORM_POSITION,
                      ControlFields::CONTROL_FIELD_GRANULARITY);
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        node->get<PhysicsComponent>()->setPositionX(
            currentValues(TransformFields::TRANSFORM_POSITION,
            ControlFields::CONTROL_FIELD_X));
    }
    return true;
}

bool GUIEditor::Handle_IncrementPositionY(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position Y incremented via button!");
    // If we don't have a selection, the button is disabled
    currentValues(TransformFields::TRANSFORM_POSITION,
                  ControlFields::CONTROL_FIELD_Y) +=
        currentValues(TransformFields::TRANSFORM_POSITION,
                      ControlFields::CONTROL_FIELD_GRANULARITY);
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        node->get<PhysicsComponent>()->setPositionY(
            currentValues(TransformFields::TRANSFORM_POSITION,
            ControlFields::CONTROL_FIELD_Y));
    }
    return true;
}

bool GUIEditor::Handle_DecrementPositionY(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position Y decremented via button!");
    // If we don't have a selection, the button is disabled
    currentValues(TransformFields::TRANSFORM_POSITION,
                  ControlFields::CONTROL_FIELD_Y) -=
        currentValues(TransformFields::TRANSFORM_POSITION,
                      ControlFields::CONTROL_FIELD_GRANULARITY);
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        node->get<PhysicsComponent>()->setPositionY(
            currentValues(TransformFields::TRANSFORM_POSITION,
            ControlFields::CONTROL_FIELD_Y));
    }
    return true;
}

bool GUIEditor::Handle_IncrementPositionZ(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position Z incremented via button!");
    // If we don't have a selection, the button is disabled
    currentValues(TransformFields::TRANSFORM_POSITION,
                  ControlFields::CONTROL_FIELD_Z) +=
        currentValues(TransformFields::TRANSFORM_POSITION,
                      ControlFields::CONTROL_FIELD_GRANULARITY);
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        node->get<PhysicsComponent>()->setPositionZ(
            currentValues(TransformFields::TRANSFORM_POSITION,
            ControlFields::CONTROL_FIELD_Z));
    }
    return true;
}

bool GUIEditor::Handle_DecrementPositionZ(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Position Z decremented via button!");
    // If we don't have a selection, the button is disabled
    currentValues(TransformFields::TRANSFORM_POSITION,
                  ControlFields::CONTROL_FIELD_Z) -=
        currentValues(TransformFields::TRANSFORM_POSITION,
                      ControlFields::CONTROL_FIELD_GRANULARITY);
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        node->get<PhysicsComponent>()->setPositionZ(
            currentValues(TransformFields::TRANSFORM_POSITION,
            ControlFields::CONTROL_FIELD_Z));
    }
    return true;
}

bool GUIEditor::Handle_IncrementPositionGranularity(const CEGUI::EventArgs &e) {
    Console::d_printfn(
        "[Editor]: Position Granularity incremented via button!");
    F32 pos = std::min(currentValues(TransformFields::TRANSFORM_POSITION,
                                     ControlFields::CONTROL_FIELD_GRANULARITY) *
                           10.0f,
                       10000000.0f);
    currentValues(TransformFields::TRANSFORM_POSITION,
                  ControlFields::CONTROL_FIELD_GRANULARITY) = pos;
    CEGUI::String posStr = CEGUI::PropertyHelper<F32>::toString(
        currentValues(TransformFields::TRANSFORM_POSITION,
                      ControlFields::CONTROL_FIELD_GRANULARITY));
    valuesField(TransformFields::TRANSFORM_POSITION,
                ControlFields::CONTROL_FIELD_GRANULARITY)->setText(posStr);
    return true;
}

bool GUIEditor::Handle_DecrementPositionGranularity(const CEGUI::EventArgs &e) {
    Console::d_printfn(
        "[Editor]: Position Granularity decremented via button!");
    F32 pos = std::max(currentValues(TransformFields::TRANSFORM_POSITION,
                                     ControlFields::CONTROL_FIELD_GRANULARITY) /
                           10.0f,
                       0.0000001f);
    currentValues(TransformFields::TRANSFORM_POSITION,
                  ControlFields::CONTROL_FIELD_GRANULARITY) = pos;
    CEGUI::String posStr = CEGUI::PropertyHelper<F32>::toString(
        currentValues(TransformFields::TRANSFORM_POSITION,
                      ControlFields::CONTROL_FIELD_GRANULARITY));
    valuesField(TransformFields::TRANSFORM_POSITION,
                ControlFields::CONTROL_FIELD_GRANULARITY)->setText(posStr);
    return true;
}

bool GUIEditor::Handle_IncrementRotationX(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Rotation X incremented via button!");
    currentValues(TransformFields::TRANSFORM_ROTATION,
                  ControlFields::CONTROL_FIELD_X) +=
        currentValues(TransformFields::TRANSFORM_ROTATION,
                      ControlFields::CONTROL_FIELD_GRANULARITY);
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        PhysicsComponent *pComp =
            node->get<PhysicsComponent>();
        pComp->setRotation(
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_X),
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_Y),
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_Z));
    }
    return true;
}

bool GUIEditor::Handle_DecrementRotationX(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Rotation X decremented via button!");
    currentValues(TransformFields::TRANSFORM_ROTATION,
                  ControlFields::CONTROL_FIELD_X) -=
        currentValues(TransformFields::TRANSFORM_ROTATION,
                      ControlFields::CONTROL_FIELD_GRANULARITY);
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        PhysicsComponent *pComp =
            node->get<PhysicsComponent>();
        pComp->setRotation(
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_X),
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_Y),
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_Z));
    }

    return true;
}

bool GUIEditor::Handle_IncrementRotationY(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Rotation Y incremented via button!");
    
    currentValues(TransformFields::TRANSFORM_ROTATION,
                  ControlFields::CONTROL_FIELD_Y) +=
        currentValues(TransformFields::TRANSFORM_ROTATION,
                      ControlFields::CONTROL_FIELD_GRANULARITY);
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        PhysicsComponent *pComp =
            node->get<PhysicsComponent>();
        pComp->setRotation(
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_X),
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_Y),
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_Z));
    }
    return true;
}

bool GUIEditor::Handle_DecrementRotationY(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Rotation Y decremented via button!");
    currentValues(TransformFields::TRANSFORM_ROTATION,
                  ControlFields::CONTROL_FIELD_Y) -=
        currentValues(TransformFields::TRANSFORM_ROTATION,
                      ControlFields::CONTROL_FIELD_GRANULARITY);
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        PhysicsComponent *pComp =
            node->get<PhysicsComponent>();
        pComp->setRotation(
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_X),
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_Y),
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_Z));
    }
    return true;
}

bool GUIEditor::Handle_IncrementRotationZ(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Rotation Z incremented via button!");
    
    currentValues(TransformFields::TRANSFORM_ROTATION,
                  ControlFields::CONTROL_FIELD_Z) +=
        currentValues(TransformFields::TRANSFORM_ROTATION,
                      ControlFields::CONTROL_FIELD_GRANULARITY);
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        PhysicsComponent *pComp =
            node->get<PhysicsComponent>();
        pComp->setRotation(
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_X),
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_Y),
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_Z));
    }
    return true;
}

bool GUIEditor::Handle_DecrementRotationZ(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Rotation Z decremented via button!");
    
    currentValues(TransformFields::TRANSFORM_ROTATION,
                  ControlFields::CONTROL_FIELD_Z) -=
        currentValues(TransformFields::TRANSFORM_ROTATION,
                      ControlFields::CONTROL_FIELD_GRANULARITY);
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        PhysicsComponent *pComp =
            node->get<PhysicsComponent>();
        pComp->setRotation(
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_X),
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_Y),
            currentValues(TransformFields::TRANSFORM_ROTATION,
            ControlFields::CONTROL_FIELD_Z));
    }
    return true;
}

bool GUIEditor::Handle_IncrementRotationGranularity(const CEGUI::EventArgs &e) {
    Console::d_printfn(
        "[Editor]: Rotation Granularity incremented via button!");
    F32 gran =
        std::min(currentValues(TransformFields::TRANSFORM_ROTATION,
                               ControlFields::CONTROL_FIELD_GRANULARITY) *
                     10.0f,
                 359.0f);
    currentValues(TransformFields::TRANSFORM_ROTATION,
                  ControlFields::CONTROL_FIELD_GRANULARITY) = gran;
    CEGUI::String granStr = CEGUI::PropertyHelper<F32>::toString(
        currentValues(TransformFields::TRANSFORM_ROTATION,
                      ControlFields::CONTROL_FIELD_GRANULARITY));
    valuesField(TransformFields::TRANSFORM_ROTATION,
                ControlFields::CONTROL_FIELD_GRANULARITY)->setText(granStr);
    return true;
}

bool GUIEditor::Handle_DecrementRotationGranularity(const CEGUI::EventArgs &e) {
    Console::d_printfn(
        "[Editor]: Rotation Granularity decremented via button!");
    F32 gran =
        std::max(currentValues(TransformFields::TRANSFORM_ROTATION,
                               ControlFields::CONTROL_FIELD_GRANULARITY) /
                     10.0f,
                 0.0000001f);
    currentValues(TransformFields::TRANSFORM_ROTATION,
                  ControlFields::CONTROL_FIELD_GRANULARITY) = gran;
    CEGUI::String granStr = CEGUI::PropertyHelper<F32>::toString(
        currentValues(TransformFields::TRANSFORM_ROTATION,
                      ControlFields::CONTROL_FIELD_GRANULARITY));
    valuesField(TransformFields::TRANSFORM_ROTATION,
                ControlFields::CONTROL_FIELD_GRANULARITY)->setText(granStr);
    return true;
}

bool GUIEditor::Handle_IncrementScaleX(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale X incremented via button!");
    
    currentValues(TransformFields::TRANSFORM_SCALE,
                  ControlFields::CONTROL_FIELD_X) +=
        currentValues(TransformFields::TRANSFORM_SCALE,
                      ControlFields::CONTROL_FIELD_GRANULARITY);
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        node->get<PhysicsComponent>()->setScaleX(
            currentValues(TransformFields::TRANSFORM_SCALE,
            ControlFields::CONTROL_FIELD_X));
    }
    return true;
}

bool GUIEditor::Handle_DecrementScaleX(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale X decremented via button!");
    
    currentValues(TransformFields::TRANSFORM_SCALE,
                  ControlFields::CONTROL_FIELD_X) -=
        currentValues(TransformFields::TRANSFORM_SCALE,
                      ControlFields::CONTROL_FIELD_GRANULARITY);
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        node->get<PhysicsComponent>()->setScaleX(
            currentValues(TransformFields::TRANSFORM_SCALE,
            ControlFields::CONTROL_FIELD_X));
    }
    return true;
}

bool GUIEditor::Handle_IncrementScaleY(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale Y incremented via button!");
    
    currentValues(TransformFields::TRANSFORM_SCALE,
                  ControlFields::CONTROL_FIELD_Y) +=
        currentValues(TransformFields::TRANSFORM_SCALE,
                      ControlFields::CONTROL_FIELD_GRANULARITY);
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        node->get<PhysicsComponent>()->setScaleY(
            currentValues(TransformFields::TRANSFORM_SCALE,
            ControlFields::CONTROL_FIELD_Y));
    }
    return true;
}

bool GUIEditor::Handle_DecrementScaleY(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale Y decremented via button!");
    
    currentValues(TransformFields::TRANSFORM_SCALE,
                  ControlFields::CONTROL_FIELD_Y) -=
        currentValues(TransformFields::TRANSFORM_SCALE,
                      ControlFields::CONTROL_FIELD_GRANULARITY);
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        node->get<PhysicsComponent>()->setScaleY(
            currentValues(TransformFields::TRANSFORM_SCALE,
            ControlFields::CONTROL_FIELD_Y));
    }
    return true;
}

bool GUIEditor::Handle_IncrementScaleZ(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale Z incremented via button!");
    currentValues(TransformFields::TRANSFORM_SCALE,
                  ControlFields::CONTROL_FIELD_Z) +=
        currentValues(TransformFields::TRANSFORM_SCALE,
                      ControlFields::CONTROL_FIELD_GRANULARITY);
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        node->get<PhysicsComponent>()->setScaleZ(
            currentValues(TransformFields::TRANSFORM_SCALE,
            ControlFields::CONTROL_FIELD_Z));
    }
    return true;
}

bool GUIEditor::Handle_DecrementScaleZ(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale Z decremented via button!");
    currentValues(TransformFields::TRANSFORM_SCALE,
                  ControlFields::CONTROL_FIELD_Z) -=
        currentValues(TransformFields::TRANSFORM_SCALE,
                      ControlFields::CONTROL_FIELD_GRANULARITY);
    SceneGraphNode_ptr node = _currentSelection.lock();
    if (node) {
        node->get<PhysicsComponent>()->setScaleZ(
            currentValues(TransformFields::TRANSFORM_SCALE,
            ControlFields::CONTROL_FIELD_Z));
    }
    return true;
}

bool GUIEditor::Handle_IncrementScaleGranularity(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale Granularity incremented via button!");
    F32 gran =
        std::min(currentValues(TransformFields::TRANSFORM_SCALE,
                               ControlFields::CONTROL_FIELD_GRANULARITY) *
                     10.0f,
                 10000000.0f);
    currentValues(TransformFields::TRANSFORM_SCALE,
                  ControlFields::CONTROL_FIELD_GRANULARITY) = gran;
    CEGUI::String granStr = CEGUI::PropertyHelper<F32>::toString(
        currentValues(TransformFields::TRANSFORM_SCALE,
                      ControlFields::CONTROL_FIELD_GRANULARITY));
    valuesField(TransformFields::TRANSFORM_SCALE,
                ControlFields::CONTROL_FIELD_GRANULARITY)->setText(granStr);
    return true;
}

bool GUIEditor::Handle_DecrementScaleGranularity(const CEGUI::EventArgs &e) {
    Console::d_printfn("[Editor]: Scale Granularity decremented via button!");
    F32 gran =
        std::max(currentValues(TransformFields::TRANSFORM_SCALE,
                               ControlFields::CONTROL_FIELD_GRANULARITY) /
                     10.0f,
                 0.0000001f);
    currentValues(TransformFields::TRANSFORM_SCALE,
                  ControlFields::CONTROL_FIELD_GRANULARITY) = gran;
    CEGUI::String granStr = CEGUI::PropertyHelper<F32>::toString(
        currentValues(TransformFields::TRANSFORM_SCALE,
                      ControlFields::CONTROL_FIELD_GRANULARITY));
    valuesField(TransformFields::TRANSFORM_SCALE,
                ControlFields::CONTROL_FIELD_GRANULARITY)->setText(granStr);
    return true;
}

};  // namespace Divide