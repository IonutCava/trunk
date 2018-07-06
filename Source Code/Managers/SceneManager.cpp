#include "Headers/SceneManager.h"
#include "Headers/AIManager.h"

#include "SceneList.h"
#include "Geometry/Importer/Headers/DVDConverter.h"

SceneManager::SceneManager() : FrameListener(),
							   _activeScene(NULL),
							   _init(false)
{
	DVDConverter::createInstance();
	AIManager::createInstance();
}

SceneManager::~SceneManager(){
	PRINT_FN(Locale::get("SCENE_MANAGER_DELETE"));
	PRINT_FN(Locale::get("SCENE_MANAGER_REMOVE_SCENES"));
	for_each(SceneMap::value_type& it, _sceneMap){
		it.second->unload();
		SAFE_DELETE(it.second);
	}
	_sceneMap.clear();
	//Destroy the model loader;
	DVDConverter::getInstance().DestroyInstance();
}

bool SceneManager::init(){
	REGISTER_FRAME_LISTENER(&(this->getInstance()));
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
	_activeScene->preLoad();
	_activeScene->loadXMLAssets();
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

void SceneManager::render(const RenderStage& stage) {
	assert(_activeScene != NULL);

	if(_renderFunction.empty()){
		if(_activeScene->renderCallback().empty()){
			_renderFunction = DELEGATE_BIND(&SceneGraph::update, _activeScene->getSceneGraph());
		}else{
			_renderFunction = _activeScene->renderCallback();
		}
	}

	GFXDevice& GFX = GFX_DEVICE;
	GFX.setRenderStage(stage);
	GFX.render(_renderFunction,_activeScene->renderState());

	if(bitCompare(stage,FINAL_STAGE) || bitCompare(stage,DEFERRED_STAGE)){
		// Draw bounding boxes, skeletons, axis gizmo, etc.
		GFX.debugDraw();
		// Preview depthmaps if needed
		LightManager::getInstance().previewShadowMaps();
		// Show navmeshes
		AIManager::getInstance().debugDraw(false);
	}
}