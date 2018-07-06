#include <CEGUI/CEGUI.h>
#include "Headers/GUIEditor.h"
#include "Headers/GUIEditorAIInterface.h"
#include "Headers/GUIEditorLightInterface.h"
#include "Headers/GUIEditorSceneGraphInterface.h"

#include "GUI/Headers/GUI.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"

GUIEditor::GUIEditor() : _init(false), _editorWindow(NULL)
{
    GUIEditorAIInterface::createInstance();
    GUIEditorLightInterface::createInstance();
    GUIEditorSceneGraphInterface::createInstance();
}

GUIEditor::~GUIEditor()
{
    _init = false;
}

bool GUIEditor::init(){
    if(_init){
        ERROR_FN(Locale::get("ERROR_EDITOR_DOUBLE_INIT"));
        return false;
    }
    PRINT_FN(Locale::get("GUI_EDITOR_INIT"));
    // Get a local pointer to the CEGUI Window Manager, Purely for convenience to reduce typing
    CEGUI::WindowManager *pWindowManager = CEGUI::WindowManager::getSingletonPtr();
    // load the editor Window from the layout file
    std::string layoutFile = ParamHandler::getInstance().getParam<std::string>("GUI.editorLayout");
    _editorWindow = pWindowManager->loadLayoutFromFile(layoutFile);

    if (_editorWindow){
         // Add the Window to the GUI Root Sheet
         CEGUI_DEFAULT_CONTEXT.getRootWindow()->addChild(_editorWindow);
         _editorWindow = static_cast<CEGUI::Window*>(_editorWindow->getChild("WorldEditor"));
         // Now register the handlers for the events (Clicking, typing, etc)
         RegisterHandlers();
    }else{
         // Loading layout from file, failed, so output an error Message.
         CEGUI::Logger::getSingleton().logEvent("Error: Unable to load the Editorv from .layout");
         ERROR_FN(Locale::get("ERROR_EDITOR_LAYOUT_FILE"),layoutFile.c_str());
         return false;
    }

    setVisible(false);

    GUIEditorAIInterface::getInstance().init(_editorWindow);
    GUIEditorLightInterface::getInstance().init(_editorWindow);
    GUIEditorSceneGraphInterface::getInstance().init(_editorWindow);
    _init = true;
    PRINT_FN(Locale::get("GUI_EDITOR_CREATED"));
    return true;
}

void GUIEditor::RegisterHandlers(){
    CEGUI::Window* AIEditor = static_cast<CEGUI::Window*>(_editorWindow->getChild("AIEditor"));
    CEGUI::Window* createNavMesh = static_cast<CEGUI::Window*>(AIEditor->getChild("NavMeshButton"));
    createNavMesh->subscribeEvent(CEGUI::PushButton::EventClicked,CEGUI::Event::Subscriber(&GUIEditorAIInterface::Handle_CreateNavMesh,&GUIEditorAIInterface::getInstance()));
}

void GUIEditor::setVisible(bool visible){
    _editorWindow->setVisible(visible);
}

bool GUIEditor::isVisible(){
    return _editorWindow->isVisible();
}

bool GUIEditor::tick(U32 deltaMsTime){
    bool state = GUIEditorAIInterface::getInstance().tick(deltaMsTime);
    if(state) state = GUIEditorLightInterface::getInstance().tick(deltaMsTime);
    if(state) state = GUIEditorSceneGraphInterface::getInstance().tick(deltaMsTime);
    return state;
}