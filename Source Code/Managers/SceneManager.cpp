#include "Headers/SceneManager.h"
#include "Headers/AIManager.h"

#include "SceneList.h"
#include "Core/Headers/ParamHandler.h"
#include "Geometry/Importer/Headers/DVDConverter.h"
#include "Rendering/RenderPass/Headers/RenderPassCuller.h"

namespace Divide {

SceneManager::SceneManager() : FrameListener(),
                               _GUI(nullptr),
                               _activeScene(nullptr),
                               _renderPassCuller(nullptr),
                               _renderPassManager(nullptr),
                               _defaultMaterial(nullptr),
                               _loadPreRenderComplete(false),
                               _processInput(false),
                               _init(false)

{
    DVDConverter::createInstance();
    AI::AIManager::createInstance();
}

SceneManager::~SceneManager(){
    UNREGISTER_FRAME_LISTENER(&(this->getInstance()));

    PRINT_FN(Locale::get("STOP_SCENE_MANAGER"));
    //PRINT_FN(Locale::get("SCENE_MANAGER_DELETE"));
    PRINT_FN(Locale::get("SCENE_MANAGER_REMOVE_SCENES"));
    for(SceneMap::value_type& it : _sceneMap){
        SAFE_DELETE(it.second);
    }
    SAFE_DELETE(_renderPassCuller);
    _sceneMap.clear();
    //Destroy the model loader;
    DVDConverter::getInstance().destroyInstance();
}

bool SceneManager::init(GUI* const gui){
    REGISTER_FRAME_LISTENER(&(this->getInstance()), 1);

    //Load default material
    PRINT_FN(Locale::get("LOAD_DEFAULT_MATERIAL"));
    _defaultMaterial = XML::loadMaterialXML(ParamHandler::getInstance().getParam<std::string>("scriptLocation")+"/defaultMaterial", false);
    _defaultMaterial->dumpToFile(false);

    _GUI = gui;
    _renderPassCuller = New RenderPassCuller();
    _renderPassManager = &RenderPassManager::getOrCreateInstance();
    _init = true;
    return true;
}

bool SceneManager::load(const std::string& sceneName, const vec2<U16>& resolution, CameraManager* const cameraMgr){
    assert(_init == true && _GUI != nullptr);
    PRINT_FN(Locale::get("SCENE_MANAGER_LOAD_SCENE_DATA"));
    //Initialize the model importer:
    if(!DVDConverter::getInstance().init()){
        return false;
    }
    XML::loadScene(sceneName, *this);
    if(!_activeScene){
        return false;
    }
    cacheResolution(resolution);
    return _activeScene->load(sceneName, cameraMgr, _GUI);
}

Scene* SceneManager::createScene(const std::string& name){
    Scene* scene = nullptr;

    if(!name.empty())
        scene = _sceneFactory[name]();

    if(scene != nullptr)
        _sceneMap.insert(std::make_pair(name, scene));

    return scene;
}

bool SceneManager::unloadCurrentScene()  {  
    AI::AIManager::getInstance().pauseUpdate(true); 
    RemoveResource(_defaultMaterial);
    return _activeScene->unload();
}

void SceneManager::initPostLoadState(){
    Material::serializeShaderLoad(true);
    _loadPreRenderComplete = _processInput = true;
}

bool SceneManager::deinitializeAI(bool continueOnErrors)  { 
    bool state = _activeScene->deinitializeAI(continueOnErrors);
    AI::AIManager::getInstance().destroyInstance(); 
    return state;
}

bool SceneManager::frameStarted(const FrameEvent& evt) {
    return _activeScene->frameStarted();
}

bool SceneManager::framePreRenderStarted(const FrameEvent& evt){
    return true;
}

bool SceneManager::frameEnded(const FrameEvent& evt){
    _renderPassCuller->refresh();

    if (_loadPreRenderComplete) {
        Material::unlockShaderQueue();
    }
    return _activeScene->frameEnded();
}

void SceneManager::preRender() {
    _activeScene->preRender();
}

void SceneManager::updateVisibleNodes() {
    _renderPassCuller->cullSceneGraph(_activeScene->getSceneGraph()->getRoot(), _activeScene->state());
}

void SceneManager::renderVisibleNodes() {
    updateVisibleNodes();
    _renderPassManager->render(_activeScene->renderState(), _activeScene->getSceneGraph());
}

void SceneManager::render(const RenderStage& stage, const Kernel& kernel) {
    assert(_activeScene != nullptr);

    static DELEGATE_CBK renderFunction;
    if(renderFunction.empty()){
        if(_activeScene->renderCallback().empty()){
            renderFunction = DELEGATE_BIND(&SceneManager::renderVisibleNodes, this);
        }else{
            renderFunction = _activeScene->renderCallback();
        }
    }

    _activeScene->renderState().getCamera().renderLookAt();
    kernel.submitRenderCall(stage, _activeScene->renderState(), renderFunction);
    _activeScene->debugDraw(stage);
}

void SceneManager::postRender(){
    _activeScene->postRender();
}

void SceneManager::onCameraChange(){
    _activeScene->onCameraChange();
}

///--------------------------Input Management-------------------------------------///

bool SceneManager::onKeyDown(const Input::KeyEvent& key) {
    if (!_processInput) {
        return false;
    }
    return  _activeScene->onKeyDown(key);
}

bool SceneManager::onKeyUp(const Input::KeyEvent& key) {
    if (!_processInput) {
        return false;
    }

    return _activeScene->onKeyUp(key);
}

bool SceneManager::mouseMoved(const Input::MouseEvent& arg) {
    if (!_processInput) {
        return false;
    }

    return _activeScene->mouseMoved(arg);
}

bool SceneManager::mouseButtonPressed(const Input::MouseEvent& arg, Input::MouseButton button) {
    if (!_processInput) {
        return false;
    }
    return _activeScene->mouseButtonPressed(arg,button);
}

bool SceneManager::mouseButtonReleased(const Input::MouseEvent& arg, Input::MouseButton button) {
    if (!_processInput) {
        return false;
    }

    return _activeScene->mouseButtonReleased(arg,button);
}

bool SceneManager::joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis) {
    if (!_processInput) {
        return false;
    }
    return _activeScene->joystickAxisMoved(arg,axis);
}

bool SceneManager::joystickPovMoved(const Input::JoystickEvent& arg, I8 pov){
    if (!_processInput) {
        return false;
    }
    return _activeScene->joystickPovMoved(arg,pov);
}

bool SceneManager::joystickButtonPressed(const Input::JoystickEvent& arg, I8 button){
    if (!_processInput) {
        return false;
    }
    return _activeScene->joystickButtonPressed(arg,button);
}

bool SceneManager::joystickButtonReleased(const Input::JoystickEvent& arg, I8 button){
    if (!_processInput) {
        return false;
    }
    return _activeScene->joystickButtonReleased(arg,button);
}

bool SceneManager::joystickSliderMoved( const Input::JoystickEvent &arg, I8 index){
    if (!_processInput) {
        return false;
    }
    return _activeScene->joystickSliderMoved(arg,index);
}

bool SceneManager::joystickVector3DMoved( const Input::JoystickEvent &arg, I8 index){
    if (!_processInput) {
        return false;
    }
    return _activeScene->joystickVector3DMoved(arg,index);
}

};