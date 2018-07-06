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

#include "AI/Headers/AIEntity.h"
#include "AI/PathFinding/NavMeshes/Headers/NavMesh.h"

namespace Divide {

namespace AI {
DEFINE_SINGLETON(AIManager)
public:
    typedef hashMapImpl<U32, AITeam*> AITeamMap;
    typedef hashMapImpl<AIEntity::PresetAgentRadius, Navigation::NavigationMesh*, hashAlg::hash<I32> > NavMeshMap;

    /// Clear all AI related data (teams, entities, NavMeshes, etc);
    void Destroy();
    /// Called at a fixed interval (preferably in a separate thread);
    void update();
    /// Add an AI Entity to a specific team. 
    /// Entities can be added to multiple teams. Caller is responsible for the lifetime of entity
    bool registerEntity(U32 teamId, AIEntity* entity);
    /// Remove an AI Entity from a specific teams. Entities can be added to multiple teams.
    /// Caller is responsible for the lifetime of entity
    void unregisterEntity(U32 teamId, AIEntity* entity);
    /// Remove an AI Entity from all teams. Entities can be added to multiple teams. 
    /// Caller is responsible for the lifetime of entity
    void unregisterEntity(AIEntity* entity);
    inline AITeam* const getTeamByID(I32 AITeamID) {
        if (AITeamID != -1) {
            AITeamMap::const_iterator it = _aiTeams.find(AITeamID);
            if (it != _aiTeams.end()) {
                return it->second;
            }
        }
        return nullptr;
    }
    ///Add a NavMesh
    bool addNavMesh(AIEntity::PresetAgentRadius radius, Navigation::NavigationMesh* const navMesh);
    Navigation::NavigationMesh* getNavMesh(AIEntity::PresetAgentRadius radius) const { 
        NavMeshMap::const_iterator it = _navMeshes.find(radius);
        if (it != _navMeshes.end()) {
            return it->second;
        }
        return nullptr;
    }
    ///Remove a NavMesh
    void destroyNavMesh(AIEntity::PresetAgentRadius radius);

    inline void setSceneCallback(const DELEGATE_CBK<>& callback) {WriteLock w_lock(_updateMutex); _sceneCallback = callback;}
    inline void pauseUpdate(bool state)       { _pauseUpdate = state;}
    inline bool updatePaused()          const { return _pauseUpdate; }
    inline bool updating()              const { return _updating; }
    /// Handle any debug information rendering (nav meshes, AI paths, etc). 
    /// Called by Scene::postRender after depth map preview call
    void debugDraw(bool forceAll = true);
    inline bool isDebugDraw() const { return _navMeshDebugDraw; }
    /// Toggle debug draw for all NavMeshes
    void toggleNavMeshDebugDraw(bool state);
    inline bool navMeshDebugDraw() const { return _navMeshDebugDraw; }

   
protected:
    AIManager();
    ~AIManager();

protected:
    friend class Scene;
    void signalInit();

protected:
    friend class AITeam;
    /// Register an AI Team
    void registerTeam(AITeam* const team);
    /// Unregister an AI Team
    void unregisterTeam(AITeam* const team);

private:
    void processInput(const U64 deltaTime);  ///< sensors
    void processData(const U64 deltaTime);   ///< think
    void updateEntities(const U64 deltaTime);///< react

private:
    U64 _deltaTime,_currentTime, _previousTime;
    std::atomic<bool> _navMeshDebugDraw;
    std::atomic<bool> _pauseUpdate;
    std::atomic<bool> _updating;
    NavMeshMap      _navMeshes;
    AITeamMap       _aiTeams;
    mutable SharedLock _updateMutex;
    mutable SharedLock _navMeshMutex;
    DELEGATE_CBK<>     _sceneCallback;

END_SINGLETON

}; //namespace AI
}; //namespace Divide
#endif