/*
   Copyright (c) 2013 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _SCENE_MANAGER_H
#define _SCENE_MANAGER_H

#include "core.h"
#include "Scenes/Headers/Scene.h"
#include <boost/functional/factory.hpp>

enum RenderStage;
DEFINE_SINGLETON_EXT1(SceneManager, FrameListener)

public:
    ///Lookup the factory methods table and return the pointer to a newly constructed scene bound to that name
    Scene* createScene(const std::string& name);

    inline Scene* getActiveScene()                   { return _activeScene; }
    inline void   setActiveScene(Scene* const scene) { SAFE_UPDATE(_activeScene, scene); }

    bool   init();

    /*Base Scene Operations*/
    void preRender();
    void render(const RenderStage& stage);
    void postRender();

    inline void idle()                                 { _activeScene->idle(); }
    inline bool unloadCurrentScene()                   { return _activeScene->unload(); }
    bool load(const std::string& name, const vec2<U16>& resolution,  CameraManager* const cameraMgr);
    ///Check if the scene was loaded properly
    inline bool checkLoadFlag()                 const  {return _activeScene->checkLoadFlag();}
    ///Create AI entities, teams, NPC's etc
    inline bool initializeAI(bool continueOnErrors)    { return _activeScene->initializeAI(continueOnErrors); }
    ///Destroy all AI entities, teams, NPC's createa in "initializeAI"
    ///AIEntities are deleted automatically by the AIManager if they are not freed in "deinitializeAI"
    inline bool deinitializeAI(bool continueOnErrors)  { return _activeScene->deinitializeAI(continueOnErrors); }
    /// Update animations, network data, sounds, triggers etc.
    inline void updateCameras()                           { _activeScene->updateCameras();}
    inline void updateSceneState(const D32 deltaTime)     { _activeScene->updateSceneState(deltaTime); }

    ///Gather input events and process them in the current scene
    inline void processInput(const D32 deltaTime)   { _activeScene->processInput(deltaTime); }
    inline void processTasks(const D32 deltaTime)   { _activeScene->processTasks(deltaTime); }

    inline void cacheResolution(const vec2<U16>& newResolution) {_activeScene->cacheResolution(newResolution);}
    ///Get the number of frames render since the application started
    inline U32  getFrameCount() const {return _frameCount;}
    ///Insert a new scene factory method for the given name
    template<class DerivedScene>
    inline bool registerScene(const std::string& sceneName) {
        _sceneFactory.insert(std::make_pair(sceneName,boost::factory<DerivedScene*>()));
        return true;
    }

protected:
    ///This is inherited from FrameListener and is used to setup cameras before rendering the frame
    bool framePreRenderStarted(const FrameEvent& evt);

private:
    SceneManager();
    ~SceneManager();

private:
    typedef Unordered_map<std::string, Scene*> SceneMap;
    boost::function0<void> _renderFunction;
    bool _init;
    U32  _frameCount;
    ///Pointer to the currently active scene
    Scene* _activeScene;
    ///Scene pool
    SceneMap _sceneMap;
    ///Scene_Name -Scene_Factory table
    Unordered_map<std::string, boost::function<Scene*()> > _sceneFactory;

END_SINGLETON

///Return a pointer to the currently active scene
inline Scene* GET_ACTIVE_SCENE() {
    return SceneManager::getInstance().getActiveScene();
}

///Return a pointer to the curently active scene's scenegraph
inline SceneGraph* GET_ACTIVE_SCENEGRAPH() {
    return GET_ACTIVE_SCENE()->getSceneGraph();
}
#endif
