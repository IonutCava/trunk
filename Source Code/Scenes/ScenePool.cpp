#include "stdafx.h"

#include "Headers/ScenePool.h"

#include "SceneList.h"
#include "Managers/Headers/SceneManager.h"
#include "Scenes/DefaultScene/Headers/DefaultScene.h"

namespace Divide {

INIT_SCENE_FACTORY

ScenePool::ScenePool(SceneManager& parentMgr)
  : _parentMgr(parentMgr),
    _activeScene(nullptr),
    _loadedScene(nullptr),
    _defaultScene(nullptr)
{
    assert(!g_sceneFactory.empty());
}

ScenePool::~ScenePool()
{
    vector<Scene*> tempScenes;
    {   
        SharedLock r_lock(_sceneLock);
        tempScenes.insert(std::cend(tempScenes),
                          std::cbegin(_createdScenes),
                          std::cend(_createdScenes));
    }

    for (Scene* scene : tempScenes) {
        Attorney::SceneManagerScenePool::unloadScene(_parentMgr, scene);
        deleteScene(scene);
    }

    {
        UniqueLockShared w_lock(_sceneLock);
        _createdScenes.clear();
    }
}

bool ScenePool::defaultSceneActive() const {
    return (!_defaultScene || !_activeScene) ||
            _activeScene->getGUID() == _defaultScene->getGUID();
}

Scene& ScenePool::activeScene() {
    return *_activeScene;
}

const Scene& ScenePool::activeScene() const {
    return *_activeScene;
}

void ScenePool::activeScene(Scene& scene) {
    _activeScene = &scene;
}

Scene& ScenePool::defaultScene() {
    return *_defaultScene;
}

const Scene& ScenePool::defaultScene() const {
    return *_defaultScene;
}


void ScenePool::init() {
}


Scene* ScenePool::getOrCreateScene(PlatformContext& context, ResourceCache& cache, SceneManager& parent, const stringImpl& name, bool& foundInCache) {
    assert(!name.empty());

    foundInCache = false;
    Scene* ret = nullptr;

    UniqueLockShared lock(_sceneLock);
    for (Scene* scene : _createdScenes) {
        if (scene->name().compare(name) == 0) {
            ret = scene;
            foundInCache = true;
            break;
        }
    }

    if (ret == nullptr) {
        ret = g_sceneFactory[name](context, cache, parent, name);

        // Default scene is the first scene we load
        if (!_defaultScene) {
            _defaultScene = ret;
        }

        if (ret != nullptr) {
            _createdScenes.push_back(ret);
        }
    }
    
    return ret;
}

bool ScenePool::deleteScene(Scene*& scene) {
    if (scene != nullptr) {
        I64 targetGUID = scene->getGUID();
        I64 defaultGUID = _defaultScene ? _defaultScene->getGUID() : 0;
        I64 activeGUID = _activeScene ? _activeScene->getGUID() : 0;

        if (targetGUID != defaultGUID) {
            if (targetGUID == activeGUID && defaultGUID != 0) {
                _parentMgr.setActiveScene(_defaultScene);
            }
        } else {
            _defaultScene = nullptr;
        }

        {
            UniqueLockShared w_lock(_sceneLock);
            _createdScenes.erase(
                std::find_if(std::cbegin(_createdScenes),
                             std::cend(_createdScenes),
                                [&targetGUID](Scene* scene) -> bool
                            {
                                return scene->getGUID() == targetGUID;
                            }));
        }

        delete scene;
        scene = nullptr;

        return true;
    }

    return false;
}

vector<stringImpl> ScenePool::sceneNameList(bool sorted) const {
    vector<stringImpl> scenes;
    for (SceneFactory::value_type it : g_sceneFactory) {
        scenes.push_back(it.first);
    }

    if (sorted) {
        std::sort(std::begin(scenes), std::end(scenes));
    }

    return scenes;
}

}; //namespace Divide