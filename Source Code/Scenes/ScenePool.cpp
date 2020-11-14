#include "stdafx.h"

#include "Headers/ScenePool.h"

#include "SceneList.h"
#include "Managers/Headers/SceneManager.h"
#include "Scenes/DefaultScene/Headers/DefaultScene.h"

#include <boost/functional/factory.hpp>

namespace Divide {

INIT_SCENE_FACTORY

ScenePool::ScenePool(SceneManager& parentMgr)
  : _activeScene(nullptr),
    _loadedScene(nullptr),
    _defaultScene(nullptr),
    _parentMgr(parentMgr)
{
    assert(!g_sceneFactory.empty());
}

ScenePool::~ScenePool()
{
    vectorEASTL<Scene*> tempScenes;
    {   
        SharedLock<SharedMutex> r_lock(_sceneLock);
        tempScenes.insert(cend(tempScenes),
                          cbegin(_createdScenes),
                          cend(_createdScenes));
    }

    for (Scene* scene : tempScenes) {
        Attorney::SceneManagerScenePool::unloadScene(_parentMgr, scene);
        deleteScene(scene);
    }

    {
        UniqueLock<SharedMutex> w_lock(_sceneLock);
        _createdScenes.clear();
    }
}

bool ScenePool::defaultSceneActive() const {
    return !_defaultScene || !_activeScene ||
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


Scene* ScenePool::getOrCreateScene(PlatformContext& context, ResourceCache* cache, SceneManager& parent, const Str256& name, bool& foundInCache) {
    assert(!name.empty());

    foundInCache = false;
    Scene* ret = nullptr;

    UniqueLock<SharedMutex> lock(_sceneLock);
    for (Scene* scene : _createdScenes) {
        if (scene->resourceName().compare(name) == 0) {
            ret = scene;
            foundInCache = true;
            break;
        }
    }

    if (ret == nullptr) {
        ret = g_sceneFactory[_ID(name.c_str())](context, cache, parent, name);

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
        const I64 defaultGUID = _defaultScene ? _defaultScene->getGUID() : 0;
        const I64 activeGUID = _activeScene ? _activeScene->getGUID() : 0;

        if (targetGUID != defaultGUID) {
            if (targetGUID == activeGUID && defaultGUID != 0) {
                _parentMgr.setActiveScene(_defaultScene);
            }
        } else {
            _defaultScene = nullptr;
        }

        {
            UniqueLock<SharedMutex> w_lock(_sceneLock);
            _createdScenes.erase(
                eastl::find_if(cbegin(_createdScenes),
                               cend(_createdScenes),
                               [&targetGUID](Scene* s) -> bool
                               {
                                   return s->getGUID() == targetGUID;
                               }));
        }

        delete scene;
        scene = nullptr;

        return true;
    }

    return false;
}

vectorEASTL<Str256> ScenePool::sceneNameList(bool sorted) const {
    vectorEASTL<Str256> scenes;
    for (SceneNameMap::value_type it : g_sceneNameMap) {
        scenes.push_back(it.second);
    }

    if (sorted) {
        eastl::sort(begin(scenes), end(scenes), [](const Str256& a, const Str256& b)-> bool {
            return a < b;
        });
    }

    return scenes;
}

} //namespace Divide