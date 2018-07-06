#include "Headers/SceneManager.h"
#include "Headers/AIManager.h"

#include "SceneList.h"
#include "Core/Headers/ParamHandler.h"
#include "Geometry/Importer/Headers/DVDConverter.h"
#include "Rendering/RenderPass/Headers/RenderPassCuller.h"

SceneManager::SceneManager() : FrameListener(),
                               _GUI(NULL),
                               _activeScene(NULL),
                               _renderPassCuller(NULL),
                               _renderPassManager(NULL),
                               _init(false),
                               _frameCount(0)
{
    DVDConverter::createInstance();
    AIManager::createInstance();
}

SceneManager::~SceneManager(){
    //PRINT_FN(Locale::get("SCENE_MANAGER_DELETE"));
    PRINT_FN(Locale::get("SCENE_MANAGER_REMOVE_SCENES"));
    for_each(SceneMap::value_type& it, _sceneMap){
        SAFE_DELETE(it.second);
    }
    SAFE_DELETE(_renderPassCuller);
    _sceneMap.clear();
    //Destroy the model loader;
    DVDConverter::getInstance().destroyInstance();
}

bool SceneManager::init(GUI* const gui){
    //Load default material
    PRINT_FN(Locale::get("LOAD_DEFAULT_MATERIAL"));
    XML::loadMaterialXML(ParamHandler::getInstance().getParam<std::string>("scriptLocation")+"/defaultMaterial");

    REGISTER_FRAME_LISTENER(&(this->getInstance()));
    _GUI = gui;
    _renderPassCuller = New RenderPassCuller();
    _renderPassManager = &RenderPassManager::getOrCreateInstance();
    _init = true;
    return true;
}

bool SceneManager::load(const std::string& sceneName, const vec2<U16>& resolution, CameraManager* const cameraMgr){
    assert(_init == true && _GUI != NULL);
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
    Scene* scene = NULL;

    if(!name.empty())
        scene = _sceneFactory[name]();

    if(scene != NULL)
        _sceneMap.insert(std::make_pair(name, scene));

    return scene;
}

bool SceneManager::unloadCurrentScene()  {  
    AIManager::getInstance().pauseUpdate(true); 
    return _activeScene->unload();
}

bool SceneManager::deinitializeAI(bool continueOnErrors)  { 
    bool state = _activeScene->deinitializeAI(continueOnErrors);
    AIManager::getInstance().destroyInstance(); 
    return state;
}

bool SceneManager::framePreRenderStarted(const FrameEvent& evt){
    _activeScene->renderState().getCamera().renderLookAt();
    return true;
}

void SceneManager::preRender() {
    _activeScene->preRender();
}

void SceneManager::renderVisibleNodes() {
    SceneGraph* sceneGraph = _activeScene->getSceneGraph();
    sceneGraph->update();
    _renderPassCuller->cullSceneGraph(sceneGraph->getRoot(), _activeScene->state());
    _renderPassManager->render(_activeScene->renderState(), sceneGraph);
}

void SceneManager::render(const RenderStage& stage, const Kernel& kernel) {
    assert(_activeScene != NULL);

    static DELEGATE_CBK renderFunction;
    if(renderFunction.empty()){
        if(_activeScene->renderCallback().empty()){
            renderFunction = DELEGATE_BIND(&SceneManager::renderVisibleNodes, DELEGATE_REF(SceneManager::getInstance()));
        }else{
            renderFunction = _activeScene->renderCallback();
        }
    }

    kernel.submitRenderCall(stage, _activeScene->renderState(), renderFunction);
}

void SceneManager::postRender(){
    // Preview depthmaps if needed
    LightManager::getInstance().previewShadowMaps();
    _activeScene->postRender();
    _frameCount++;
}

///--------------------------Input Management-------------------------------------///

bool SceneManager::onKeyDown(const OIS::KeyEvent& key) {
    if(_GUI->keyCheck(key,true)){
        return _activeScene->onKeyDown(key);
    }
    return true;
}

bool SceneManager::onKeyUp(const OIS::KeyEvent& key) {
    if(_GUI->keyCheck(key,false)){
        return _activeScene->onKeyUp(key);
    }
    return true;
}

bool SceneManager::onMouseMove(const OIS::MouseEvent& arg) {
    if(_GUI->checkItem(arg)){
        return _activeScene->onMouseMove(arg);
    }
    return true;
}

bool SceneManager::onMouseClickDown(const OIS::MouseEvent& arg,OIS::MouseButtonID button) {
    if(_GUI->clickCheck(button,true)){
        return _activeScene->onMouseClickDown(arg,button);
    }
    return true;
}

bool SceneManager::onMouseClickUp(const OIS::MouseEvent& arg,OIS::MouseButtonID button) {
    if(_GUI->clickCheck(button,false)){
        return _activeScene->onMouseClickUp(arg,button);
    }
    return true;
}

bool SceneManager::onJoystickMoveAxis(const OIS::JoyStickEvent& arg,I8 axis,I32 deadZone) {
    return _activeScene->onJoystickMoveAxis(arg,axis,deadZone);
}

bool SceneManager::onJoystickMovePOV(const OIS::JoyStickEvent& arg,I8 pov){
    return _activeScene->onJoystickMovePOV(arg,pov);
}

bool SceneManager::onJoystickButtonDown(const OIS::JoyStickEvent& arg,I8 button){
    return _activeScene->onJoystickButtonDown(arg,button);
}

bool SceneManager::onJoystickButtonUp(const OIS::JoyStickEvent& arg, I8 button){
    return _activeScene->onJoystickButtonUp(arg,button);
}

bool SceneManager::sliderMoved( const OIS::JoyStickEvent &arg, I8 index){
    return _activeScene->sliderMoved(arg,index);
}

bool SceneManager::vector3Moved( const OIS::JoyStickEvent &arg, I8 index){
    return _activeScene->vector3Moved(arg,index);
}