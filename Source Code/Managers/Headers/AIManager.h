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

#ifndef _AI_MANAGER_H_
#define _AI_MANAGER_G_

#include "core.h"
#include "AI/Headers/AIEntity.h"
#include "AI/PathFinding/NavMeshes/Headers/NavMesh.h"

#include <boost/atomic.hpp>
DEFINE_SINGLETON(AIManager)
    typedef Unordered_map<I64, AIEntity*> AIEntityMap;
    typedef Unordered_map<U32, AICoordination*> AITeamMap;

public:
    /// Destroy all entities
    void Destroy();

    U8 update();
    ///Handle any debug information rendering (nav meshes, AI paths, etc);
    ///Called by Scene::postRender after depth map preview call
    void debugDraw(bool forceAll = true);
    ///Add an AI Entity from the manager
    bool addEntity(AIEntity* entity);
    ///Remove an AI Entity from the manager
    void destroyEntity(U32 guid);
    ///Register an AI Team
    inline void registerTeam(AICoordination* const team) {_aiTeams[team->getTeamID()] = team;}
    ///Add a nav mesh
    bool addNavMesh(Navigation::NavigationMesh* const navMesh);
    Navigation::NavigationMesh* getNavMesh(U16 index) const {return (index >= _navMeshes.size() ? NULL : _navMeshes[index]);}
    ///Remove a nav mesh
    void destroyNavMesh(Navigation::NavigationMesh* const navMesh);
 
    inline void setSceneCallback(boost::function0<void> callback) {WriteLock w_lock(_updateMutex); _sceneCallback = callback;}
    inline void pauseUpdate(bool state) {_pauseUpdate = state;}

    ///Toggle NavMesh debugDraw
    void toggleNavMeshDebugDraw(bool state);

protected:
    AIManager();
    ~AIManager();

private:
    void processInput();  ///< sensors
    void processData();   ///< think
    void updateEntities(const U64 deltaTime);///< react

private:
    U64 _deltaTime,_currentTime, _previousTime;
    boost::atomic<bool> _navMeshDebugDraw;
    boost::atomic<bool> _pauseUpdate;
    boost::atomic<bool> _updateNavMeshes;
    AIEntityMap _aiEntities;
    AITeamMap   _aiTeams;
    mutable SharedLock _updateMutex;
    mutable SharedLock _navMeshMutex;
    boost::function0<void> _sceneCallback;
    vectorImpl<Navigation::NavigationMesh* > _navMeshes;
END_SINGLETON

#endif