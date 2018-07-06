#include "Headers/SceneManager.h"
#include "SceneList.h"

SceneManager::SceneManager() : _activeScene(NULL){
}

SceneManager::~SceneManager(){
	PRINT_FN(Locale::get("SCENE_MANAGER_DELETE"));
	PRINT_FN(Locale::get("SCENE_MANAGER_REMOVE_SCENES"));
	SceneMap::iterator& it = _sceneMap.begin();
	for_each(SceneMap::value_type& it, _sceneMap){
		it.second->unload();
		SAFE_DELETE(it.second);
	}
	_sceneMap.clear();
}

bool SceneManager::load(const std::string& sceneName, const vec2<U16>& resolution, Camera* const camera){
	PRINT_FN(Locale::get("SCENE_MANAGER_LOAD_SCENE_DATA"));
	XML::loadScene(sceneName,*this);
	if(!_activeScene){
		return false;
	}
	cacheResolution(resolution);
	_activeScene->preLoad();
	_activeScene->loadXMLAssets(); 
	PRINT_FN(Locale::get("SCENE_MANAGER_ADD_DEFAULT_CAMERA"));
	_activeScene->updateCamera(camera);
	return _activeScene->load(sceneName);
}

Scene* SceneManager::createScene(const std::string& name){
	Scene* scene = NULL;

	if(!name.empty()){
		scene = _sceneFactory[name]();
	}
	if(scene != NULL){
		_sceneMap.insert(std::make_pair(name, scene));
	}

	return scene;
}

void SceneManager::render(RENDER_STAGE stage) {
	GFX_DEVICE.setRenderStage(stage);
	assert(_activeScene != NULL);
	_activeScene->render();
}