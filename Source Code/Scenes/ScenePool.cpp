#include "Headers/ScenePool.h"
#include "Managers/Headers/SceneManager.h"
#include "Scenes/DefaultScene/Headers/DefaultScene.h"

namespace Divide {

ScenePool::ScenePool(SceneManager& parentMgr)
  : _parentMgr(parentMgr),
    _activeScene(nullptr),
    _loadedScene(nullptr),
    _defaultScene(nullptr)
{
}

ScenePool::~ScenePool()
{
    if (_defaultScene != nullptr) {
        Scene* defaultScene = _defaultScene;
        _parentMgr.unloadScene(defaultScene);
        MemoryManager::DELETE(_defaultScene);
    }

    for (Scene* scene : _loadedScenes) {
        _parentMgr.unloadScene(scene);
    }
}

Scene& ScenePool::getActiveScene() {
    return _activeScene == nullptr 
                         ? *_defaultScene
                         : *_activeScene;
}

void ScenePool::init() {
    _defaultScene = MemoryManager_NEW DefaultScene();
}

}; //namespace Divide