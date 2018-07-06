#include <CEGUI/CEGUI.h>
#include "Headers/GUIEditor.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIConsole.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/AIManager.h"
#include "Managers/Headers/SceneManager.h"

GUIEditor::GUIEditor() : _init(false), 
                         _createNavMeshQueued(false),
                         _editorWindow(nullptr)
{
    for (U8 i = 0; i < ToggleButtons_PLACEHOLDER; ++i) {
        _toggleButtons[i] = nullptr;
    }
    for (U8 i = 0; i < ControlField_PLACEHOLDER; ++i) {
        _positionValuesField[i] = nullptr;
        _rotationValuesField[i] = nullptr;
        _scaleValuesField[i] = nullptr;
    }
    _currentPositionValues[CONTROL_FIELD_GRANULARITY] = 1.0;
    _currentRotationValues[CONTROL_FIELD_GRANULARITY] = 1.0; 
    _currentScaleValues[CONTROL_FIELD_GRANULARITY] = 1.0;

    _currentPositionValues[CONTROL_FIELD_X] = 0.0;
    _currentRotationValues[CONTROL_FIELD_X] = 0.0; 
    _currentScaleValues[CONTROL_FIELD_X] = 0.0;

    _currentPositionValues[CONTROL_FIELD_Y] = 0.0;
    _currentRotationValues[CONTROL_FIELD_Y] = 0.0; 
    _currentScaleValues[CONTROL_FIELD_Y] = 0.0;

    _currentPositionValues[CONTROL_FIELD_Z] = 0.0;
    _currentRotationValues[CONTROL_FIELD_Z] = 0.0; 
    _currentScaleValues[CONTROL_FIELD_Z] = 0.0;
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

void GUIEditor::RegisterHandlers() {
    CEGUI::Window* EditorBar = static_cast<CEGUI::Window*>(_editorWindow->getChild("MenuBar"));

    // Save Scene Button
    {
        CEGUI::Window* saveSceneButton = static_cast<CEGUI::Window*>(EditorBar->getChild("SaveSceneButton"));
        saveSceneButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_SaveScene, this));
    }
    // Save Selection Button
    {
        CEGUI::Window* saveSelectionButton = static_cast<CEGUI::Window*>(EditorBar->getChild("SaveSelectionButton"));
        saveSelectionButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_SaveSelection, this));
    }
    // Delete Selection Button
    {
        CEGUI::Window* deleteSelectionButton = static_cast<CEGUI::Window*>(EditorBar->getChild("DeleteSelection"));
        deleteSelectionButton->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DeleteSelection, this));
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
        _positionValuesField[CONTROL_FIELD_X] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("XPositionValue"));
        _positionValuesField[CONTROL_FIELD_X]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_PositionXChange, this));
    }
    // Type new Y Position
    {
        _positionValuesField[CONTROL_FIELD_Y] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("YPositionValue"));
        _positionValuesField[CONTROL_FIELD_Y]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_PositionYChange, this));
    }
    // Type new Z Position
    {
        _positionValuesField[CONTROL_FIELD_Z] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("ZPositionValue"));
        _positionValuesField[CONTROL_FIELD_Z]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_PositionZChange, this));
    }
    // Type new Position Granularity
    {
        _positionValuesField[CONTROL_FIELD_GRANULARITY] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("PositionGranularityValue"));
        _positionValuesField[CONTROL_FIELD_GRANULARITY]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_PositionGranularityChange, this));
    }
    // Type new X Rotation
    {
        _rotationValuesField[CONTROL_FIELD_X] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("XRotationValue"));
        _rotationValuesField[CONTROL_FIELD_X]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_RotationXChange, this));
    }
    // Type new Y Rotation
    {
        _rotationValuesField[CONTROL_FIELD_Y] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("YRotationValue"));
        _rotationValuesField[CONTROL_FIELD_Y]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_RotationYChange, this));
    }
    // Type new Z Rotation
    {
        _rotationValuesField[CONTROL_FIELD_Z] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("ZRotationValue"));
        _rotationValuesField[CONTROL_FIELD_Z]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_RotationZChange, this));
    }
    // Type new Rotation Granularity
    {
        _rotationValuesField[CONTROL_FIELD_GRANULARITY] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("RotationGranularityValue"));
        _rotationValuesField[CONTROL_FIELD_GRANULARITY]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_RotationGranularityChange, this));
    }
    // Type new X Scale
    {
        _scaleValuesField[CONTROL_FIELD_X] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("XScaleValue"));
        _scaleValuesField[CONTROL_FIELD_X]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_ScaleXChange, this));
    }
    // Type new Y Scale
    {
        _scaleValuesField[CONTROL_FIELD_Y] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("YScaleValue"));
        _scaleValuesField[CONTROL_FIELD_Y]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_ScaleYChange, this));
    }
    // Type new Z Scale
    {
        _scaleValuesField[CONTROL_FIELD_Z] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("ZScaleValue"));
        _scaleValuesField[CONTROL_FIELD_Z]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_ScaleZChange, this));
    }
    // Type new Scale Granularity
    {
        _scaleValuesField[CONTROL_FIELD_GRANULARITY] = static_cast<CEGUI::Editbox*>(EditorBar->getChild("ScaleGranularityValue"));
        _scaleValuesField[CONTROL_FIELD_GRANULARITY]->subscribeEvent(CEGUI::Editbox::EventTextAccepted, CEGUI::Event::Subscriber(&GUIEditor::Handle_ScaleGranularityChange, this));
    }
    // Increment X Position
    {
        CEGUI::Window* increment = static_cast<CEGUI::Window*>(EditorBar->getChild("IncPositionXButton"));
        increment->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementPositionX, this));
    }
    // Decrement X Position
    {
        CEGUI::Window* decrement = static_cast<CEGUI::Window*>(EditorBar->getChild("DecPositionXButton"));
        decrement->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementPositionX, this));
    }
    // Increment Y Position
    {
        CEGUI::Window* increment = static_cast<CEGUI::Window*>(EditorBar->getChild("IncPositionYButton"));
        increment->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementPositionY, this));
    }
    // Decrement Y Position
    {
        CEGUI::Window* decrement = static_cast<CEGUI::Window*>(EditorBar->getChild("DecPositionYButton"));
        decrement->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementPositionY, this));
    }
    // Increment Z Position
    {
        CEGUI::Window* increment = static_cast<CEGUI::Window*>(EditorBar->getChild("IncPositionZButton"));
        increment->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementPositionZ, this));
    }
    // Decrement Z Position
    {
        CEGUI::Window* decrement = static_cast<CEGUI::Window*>(EditorBar->getChild("DecPositionZButton"));
        decrement->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementPositionZ, this));
    }
    // Increment Position Granularity
    {
        CEGUI::Window* increment = static_cast<CEGUI::Window*>(EditorBar->getChild("IncPositionGranButton"));
        increment->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementPositionGranularity, this));
    }
    // Decrement Position Granularity
    {
        CEGUI::Window* decrement = static_cast<CEGUI::Window*>(EditorBar->getChild("DecPositionGranButton"));
        decrement->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementPositionGranularity, this));
    }
    // Increment X Rotation
    {
        CEGUI::Window* increment = static_cast<CEGUI::Window*>(EditorBar->getChild("IncRotationXButton"));
        increment->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementRotationX, this));
    }
    // Decrement X Rotation
    {
        CEGUI::Window* decrement = static_cast<CEGUI::Window*>(EditorBar->getChild("DecRotationXButton"));
        decrement->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementRotationX, this));
    }
    // Increment Y Rotation
    {
        CEGUI::Window* increment = static_cast<CEGUI::Window*>(EditorBar->getChild("IncRotationYButton"));
        increment->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementRotationY, this));
    }
    // Decrement Y Rotation
    {
        CEGUI::Window* decrement = static_cast<CEGUI::Window*>(EditorBar->getChild("DecRotationYButton"));
        decrement->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementRotationY, this));
    }
    // Increment Z Rotation
    {
        CEGUI::Window* increment = static_cast<CEGUI::Window*>(EditorBar->getChild("IncRotationZButton"));
        increment->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementRotationZ, this));
    }
    // Decrement Z Rotation
    {
        CEGUI::Window* decrement = static_cast<CEGUI::Window*>(EditorBar->getChild("DecRotationZButton"));
        decrement->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementRotationZ, this));
    }
    // Increment Rotation Granularity
    {
        CEGUI::Window* increment = static_cast<CEGUI::Window*>(EditorBar->getChild("IncRotationGranButton"));
        increment->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementRotationGranularity, this));
    }
    // Decrement Rotation Granularity
    {
        CEGUI::Window* decrement = static_cast<CEGUI::Window*>(EditorBar->getChild("DecRotationGranButton"));
        decrement->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementRotationGranularity, this));
    }
    // Increment X Scale
    {
        CEGUI::Window* increment = static_cast<CEGUI::Window*>(EditorBar->getChild("IncScaleXButton"));
        increment->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementScaleX, this));
    }
    // Decrement X Scale
    {
        CEGUI::Window* decrement = static_cast<CEGUI::Window*>(EditorBar->getChild("DecScaleXButton"));
        decrement->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementScaleX, this));
    }
    // Increment Y Scale
    {
        CEGUI::Window* increment = static_cast<CEGUI::Window*>(EditorBar->getChild("IncScaleYButton"));
        increment->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementScaleY, this));
    }
    // Decrement Y Scale
    {
        CEGUI::Window* decrement = static_cast<CEGUI::Window*>(EditorBar->getChild("DecScaleYButton"));
        decrement->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementScaleY, this));
    }
    // Increment Z Scale
    {
        CEGUI::Window* increment = static_cast<CEGUI::Window*>(EditorBar->getChild("IncScaleZButton"));
        increment->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementScaleZ, this));
    }
    // Decrement Z Scale
    {
        CEGUI::Window* decrement = static_cast<CEGUI::Window*>(EditorBar->getChild("DecScaleZButton"));
        decrement->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementScaleZ, this));
    }
    // Increment Scale Granularity
    {
        CEGUI::Window* increment = static_cast<CEGUI::Window*>(EditorBar->getChild("IncScaleGranButton"));
        increment->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_IncrementScaleGranularity, this));
    }
    // Decrement Scale Granularity
    {
        CEGUI::Window* decrement = static_cast<CEGUI::Window*>(EditorBar->getChild("DecScaleGranButton"));
        decrement->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIEditor::Handle_DecrementScaleGranularity, this));
    }
}

void GUIEditor::setVisible(bool visible){
    _editorWindow->setVisible(visible);
}

bool GUIEditor::isVisible() {
    return _editorWindow->isVisible();
}

bool GUIEditor::update(const U64 deltaTime) {
    bool state = true;
  	if (_createNavMeshQueued) {
        state = false;
        // Check if we already have a NavMesh created
        Navigation::NavigationMesh* temp = AIManager::getInstance().getNavMesh(0);
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
			state = AIManager::getInstance().addNavMesh(temp);
		}

		_createNavMeshQueued = false;
	}

	return state;
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
    return true;
}

bool GUIEditor::Handle_WireframeToggle(const CEGUI::EventArgs &e) {
    if ( _toggleButtons[TOGGLE_WIREFRAME]->isSelected() ) {
        D_PRINT_FN("[Editor]: Wireframe rendering enabled!");
    } else {
        D_PRINT_FN("[Editor]: Wireframe rendering disabled!");
    }
    return true;
}

bool GUIEditor::Handle_DepthPreviewToggle(const CEGUI::EventArgs &e) {
    if ( _toggleButtons[TOGGLE_DEPTH_PREVIEW]->isSelected() ) {
        D_PRINT_FN("[Editor]: Depth Preview enabled!");
    } else {
        D_PRINT_FN("[Editor]: Depth Preview disabled!");
    }
    return true;
}

bool GUIEditor::Handle_ShadowMappingToggle(const CEGUI::EventArgs &e) {
    if ( _toggleButtons[TOGGLE_SHADOW_MAPPING]->isSelected() ) {
        D_PRINT_FN("[Editor]: Shadow Mapping enabled!");
    } else {
        D_PRINT_FN("[Editor]: Shadow Mapping disabled!");
    }
    return true;
}

bool GUIEditor::Handle_FogToggle(const CEGUI::EventArgs &e) {
    if (_toggleButtons[TOGGLE_FOG]->isSelected() ) {
        D_PRINT_FN("[Editor]: Fog enabled!");
    } else {
        D_PRINT_FN("[Editor]: Fog disabled!");
    }
    return true;
}

bool GUIEditor::Handle_PostFXToggle(const CEGUI::EventArgs &e) {
    if ( _toggleButtons[TOGGLE_POST_FX]->isSelected() ) {
        D_PRINT_FN("[Editor]: PostFX enabled!");
    } else {
        D_PRINT_FN("[Editor]: PostFX disabled!");
    }
    return true;
}

bool GUIEditor::Handle_BoundingBoxesToggle(const CEGUI::EventArgs &e) {
    if ( _toggleButtons[TOGGLE_BOUNDING_BOXES]->isSelected() ) {
        D_PRINT_FN("[Editor]: Bounding Box rendering enabled!");
    } else {
        D_PRINT_FN("[Editor]: Bounding Box rendering disabled!");
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
    return true;
}

bool GUIEditor::Handle_PositionXChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position X changed via text edit!");
    return true;
}

bool GUIEditor::Handle_PositionYChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position X changed via text edit!");
    return true;
}

bool GUIEditor::Handle_PositionZChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position X changed via text edit!");
    return true;
}

bool GUIEditor::Handle_PositionGranularityChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position X changed via text edit!");
    return true;
}

bool GUIEditor::Handle_RotationXChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position X changed via text edit!");
    return true;
}

bool GUIEditor::Handle_RotationYChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position X changed via text edit!");
    return true;
}

bool GUIEditor::Handle_RotationZChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position X changed via text edit!");
    return true;
}

bool GUIEditor::Handle_RotationGranularityChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position X changed via text edit!");
    return true;
}

bool GUIEditor::Handle_ScaleXChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position X changed via text edit!");
    return true;
}

bool GUIEditor::Handle_ScaleYChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position X changed via text edit!");
    return true;
}

bool GUIEditor::Handle_ScaleZChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position X changed via text edit!");
    return true;
}

bool GUIEditor::Handle_ScaleGranularityChange(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position X changed via text edit!");
    return true;
}

bool GUIEditor::Handle_IncrementPositionX(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position X incremented via button!");
    return true;
}

bool GUIEditor::Handle_DecrementPositionX(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position X decremented via button!");
    return true;
}

bool GUIEditor::Handle_IncrementPositionY(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position Y incremented via button!");
    return true;
}

bool GUIEditor::Handle_DecrementPositionY(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position Y decremented via button!");
    return true;
}

bool GUIEditor::Handle_IncrementPositionZ(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position Z incremented via button!");
    return true;
}

bool GUIEditor::Handle_DecrementPositionZ(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position Z decremented via button!");
    return true;
}

bool GUIEditor::Handle_IncrementPositionGranularity(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position Granularity incremented via button!");
    return true;
}

bool GUIEditor::Handle_DecrementPositionGranularity(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Position Granularity decremented via button!");
    return true;
}

bool GUIEditor::Handle_IncrementRotationX(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Rotation X incremented via button!");
    return true;
}

bool GUIEditor::Handle_DecrementRotationX(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Rotation X decremented via button!");
    return true;
}

bool GUIEditor::Handle_IncrementRotationY(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Rotation Y incremented via button!");
    return true;
}

bool GUIEditor::Handle_DecrementRotationY(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Rotation Y decremented via button!");
    return true;
}

bool GUIEditor::Handle_IncrementRotationZ(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Rotation Z incremented via button!");
    return true;
}

bool GUIEditor::Handle_DecrementRotationZ(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Rotation Z decremented via button!");
    return true;
}

bool GUIEditor::Handle_IncrementRotationGranularity(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Rotation Granularity incremented via button!");
    return true;
}

bool GUIEditor::Handle_DecrementRotationGranularity(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Rotation Granularity decremented via button!");
    return true;
}

bool GUIEditor::Handle_IncrementScaleX(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Scale X incremented via button!");
    return true;
}

bool GUIEditor::Handle_DecrementScaleX(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Scale X decremented via button!");
    return true;
}

bool GUIEditor::Handle_IncrementScaleY(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Scale Y incremented via button!");
    return true;
}

bool GUIEditor::Handle_DecrementScaleY(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Scale Y decremented via button!");
    return true;
}

bool GUIEditor::Handle_IncrementScaleZ(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Scale Z incremented via button!");
    return true;
}

bool GUIEditor::Handle_DecrementScaleZ(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Scale Z decremented via button!");
    return true;
}

bool GUIEditor::Handle_IncrementScaleGranularity(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Scale Granularity incremented via button!");
    return true;
}

bool GUIEditor::Handle_DecrementScaleGranularity(const CEGUI::EventArgs &e) {
    D_PRINT_FN("[Editor]: Scale Granularity decremented via button!");
    return true;
}