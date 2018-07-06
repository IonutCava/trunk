/*
   Copyright (c) 2014 DIVIDE-Studio
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
class RenderPassCuller;

DEFINE_SINGLETON_EXT1(SceneManager, FrameListener)

public:
    ///Lookup the factory methods table and return the pointer to a newly constructed scene bound to that name
    Scene* createScene(const std::string& name);

    inline Scene* getActiveScene()                   { return _activeScene; }
    inline void   setActiveScene(Scene* const scene) { SAFE_UPDATE(_activeScene, scene); }

    bool   init(GUI* const gui);

    /*Base Scene Operations*/
    void preRender();
    void render(const RenderStage& stage, const Kernel& kernel);
    void postRender();
    // updates and culls the scene graph and renders the visible nodes
    void renderVisibleNodes();

    inline void idle()                                 { _activeScene->idle(); }
    bool unloadCurrentScene();
    bool load(const std::string& name, const vec2<U16>& resolution,  CameraManager* const cameraMgr);
    ///Check if the scene was loaded properly
    inline bool checkLoadFlag()                 const  {return _activeScene->checkLoadFlag();}
    ///Create AI entities, teams, NPC's etc
    inline bool initializeAI(bool continueOnErrors)    { return _activeScene->initializeAI(continueOnErrors); }
    ///Destroy all AI entities, teams, NPC's createa in "initializeAI"
    ///AIEntities are deleted automatically by the AIManager if they are not freed in "deinitializeAI"
           bool deinitializeAI(bool continueOnErrors);
    /// Update animations, network data, sounds, triggers etc.
    inline void updateCameras()                           { _activeScene->updateCameras();}
    inline void updateSceneState(const U64 deltaTime)     { _activeScene->updateSceneState(deltaTime); }

    ///Gather input events and process them in the current scene
    inline void processInput(const U64 deltaTime)   { _activeScene->processInput(deltaTime); }
    inline void processTasks(const U64 deltaTime)   { _activeScene->processTasks(deltaTime); }

    inline void cacheResolution(const vec2<U16>& newResolution) {_activeScene->cacheResolution(newResolution);}
    ///Get the number of frames render since the application started
    inline U32  getFrameCount() const {return _frameCount;}
    ///Insert a new scene factory method for the given name
    template<class DerivedScene>
    inline bool registerScene(const std::string& sceneName) {
        _sceneFactory.insert(std::make_pair(sceneName,boost::factory<DerivedScene*>()));
        return true;
    }
    inline void togglePreviewDepthBuffer() {_previewDepthBuffer = !_previewDepthBuffer;}

public: ///Input
    ///Key pressed
    bool onKeyDown(const OIS::KeyEvent& key);
    ///Key released
    bool onKeyUp(const OIS::KeyEvent& key);
    ///Joystic axis change
    bool onJoystickMoveAxis(const OIS::JoyStickEvent& arg,I8 axis,I32 deadZone);
    ///Joystick direction change
    bool onJoystickMovePOV(const OIS::JoyStickEvent& arg,I8 pov);
    ///Joystick button pressed
    bool onJoystickButtonDown(const OIS::JoyStickEvent& arg,I8 button);
    ///Joystick button released
    bool onJoystickButtonUp(const OIS::JoyStickEvent& arg, I8 button);
    bool sliderMoved( const OIS::JoyStickEvent &arg, I8 index);
    bool vector3Moved( const OIS::JoyStickEvent &arg, I8 index);
    ///Mouse moved
    bool onMouseMove(const OIS::MouseEvent& arg);
    ///Mouse button pressed
    bool onMouseClickDown(const OIS::MouseEvent& arg,OIS::MouseButtonID button);
    ///Mouse button released
    bool onMouseClickUp(const OIS::MouseEvent& arg,OIS::MouseButtonID button);

protected:
    ///This is inherited from FrameListener and is used to setup cameras before rendering the frame
    bool framePreRenderStarted(const FrameEvent& evt);
    bool frameEnded(const FrameEvent& evt);

private:
    SceneManager();
    ~SceneManager();

private:
    typedef Unordered_map<std::string, Scene*> SceneMap;
    bool _init;
    bool _previewDepthBuffer;
    U32  _frameCount;
    ///Pointer to the currently active scene
    Scene* _activeScene;
    ///Pointer to the GUI interface
    GUI*   _GUI;
    ///Pointer to the scene graph culler that's used to determine what nodes are visible in the current frame
    RenderPassCuller* _renderPassCuller;
    ///Pointer to the render pass manager
    RenderPassManager* _renderPassManager;
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
