#include "Headers/SceneManager.h"
#include "Headers/AIManager.h"

#include "SceneList.h"
#include "Core/Headers/ParamHandler.h"
#include "Geometry/Importer/Headers/DVDConverter.h"
#include "Rendering/RenderPass/Headers/RenderPassCuller.h"

SceneManager::SceneManager() : FrameListener(),
                               _activeScene(NULL),
                               _renderPassCuller(NULL),
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

bool SceneManager::init(){
    //Load default material
    PRINT_FN(Locale::get("LOAD_DEFAULT_MATERIAL"));
    XML::loadMaterialXML(ParamHandler::getInstance().getParam<std::string>("scriptLocation")+"/defaultMaterial");

    REGISTER_FRAME_LISTENER(&(this->getInstance()));

    _renderPassCuller = New RenderPassCuller();
    _init = true;
    return true;
}

bool SceneManager::load(const std::string& sceneName, const vec2<U16>& resolution, CameraManager* const cameraMgr){
    assert(_init == true);
    PRINT_FN(Locale::get("SCENE_MANAGER_LOAD_SCENE_DATA"));
    //Initialize the model importer:
    if(!DVDConverter::getInstance().init()){
        return false;
    }
    XML::loadScene(sceneName,*this);
    if(!_activeScene){
        return false;
    }
    cacheResolution(resolution);
    return _activeScene->load(sceneName, cameraMgr);
}

Scene* SceneManager::createScene(const std::string& name){
    Scene* scene = NULL;

    if(!name.empty())
        scene = _sceneFactory[name]();

    if(scene != NULL)
        _sceneMap.insert(std::make_pair(name, scene));

    return scene;
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
    RenderPassManager::getInstance().render(_activeScene->renderState());
}

void SceneManager::render(const RenderStage& stage) {
    assert(_activeScene != NULL);

    if(_renderFunction.empty()){
        if(_activeScene->renderCallback().empty()){
            _renderFunction = DELEGATE_BIND(&SceneManager::renderVisibleNodes, DELEGATE_REF(SceneManager::getInstance()));
        }else{
            _renderFunction = _activeScene->renderCallback();
        }
    }

    GFXDevice& GFX = GFX_DEVICE;
    GFX.setRenderStage(stage);
    GFX.render(_renderFunction, _activeScene->renderState());

    if(bitCompare(stage,FINAL_STAGE) || bitCompare(stage,DEFERRED_STAGE)){
        // Draw bounding boxes, skeletons, axis gizmo, etc.
        GFX.debugDraw();
        // Show navmeshes
        AIManager::getInstance().debugDraw(false);
    }
}

void SceneManager::postRender(){
    // Preview depthmaps if needed
    LightManager::getInstance().previewShadowMaps();
    _activeScene->postRender();
    _frameCount++;
}