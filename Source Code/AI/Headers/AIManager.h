/*
   Copyright (c) 2018 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#pragma once
#ifndef _AI_MANAGER_H_
#define _AI_MANAGER_G_

#include "AI/Headers/AIEntity.h"
#include "Scenes/Headers/SceneComponent.h"

namespace Divide {

class Scene;
struct Task;
class TaskPool;

namespace GFX {
    class CommandBuffer;
}; //namespace GFX

namespace AI {

namespace Navigation {
    class NavigationMesh;
}; //namespace Navigation

class AIManager : public SceneComponent
{
  public:
    using AITeamMap = hashMap<U32, AITeam*>;
    using NavMeshMap = hashMap<AIEntity::PresetAgentRadius, Navigation::NavigationMesh*>;

    explicit AIManager(Scene& parentScene, TaskPool& pool);
    ~AIManager();

    /// Clear all AI related data (teams, entities, NavMeshes, etc);
    void destroy();
    /// Called at a fixed interval (preferably in a separate thread);
    void update(const U64 deltaTimeUS);
    /// Add an AI Entity to a specific team.
    /// Entities can be added to multiple teams. Caller is responsible for the
    /// lifetime of entity
    bool registerEntity(U32 teamID, AIEntity* entity);
    /// Remove an AI Entity from a specific teams. Entities can be added to multiple teams.
    /// Caller is responsible for the lifetime of entity
    void unregisterEntity(U32 teamID, AIEntity* entity);
    /// Remove an AI Entity from all teams. Entities can be added to multiple teams.
    /// Caller is responsible for the lifetime of entity
    void unregisterEntity(AIEntity* entity);
    inline AITeam* const getTeamByID(I32 AITeamID) {
        if (AITeamID != -1) {
            const AITeamMap::const_iterator it = _aiTeams.find(AITeamID);
            if (it != std::end(_aiTeams)) {
                return it->second;
            }
        }
        return nullptr;
    }
    /// Add a NavMesh
    bool addNavMesh(AIEntity::PresetAgentRadius radius, Navigation::NavigationMesh* const navMesh);

    Navigation::NavigationMesh* getNavMesh(AIEntity::PresetAgentRadius radius) const {
        const NavMeshMap::const_iterator it = _navMeshes.find(radius);
        if (it != std::end(_navMeshes)) {
            return it->second;
        }
        return nullptr;
    }
    /// Remove a NavMesh
    void destroyNavMesh(AIEntity::PresetAgentRadius radius);

    inline void setSceneCallback(const DELEGATE<void>& callback) {
        UniqueLock<Mutex> w_lock(_updateMutex);
        _sceneCallback = callback;
    }
    inline void pauseUpdate(bool state) noexcept { _pauseUpdate = state; }
    inline bool updatePaused() const noexcept { return _pauseUpdate; }
    inline bool updating() const noexcept { return _updating; }
    /// Handle any debug information rendering (nav meshes, AI paths, etc).
    /// Called by Scene::postRender after depth map preview call
    void debugDraw(GFX::CommandBuffer& bufferInOut, bool forceAll = true);
    inline bool isDebugDraw() const noexcept { return _navMeshDebugDraw; }
    /// Toggle debug draw for all NavMeshes
    void toggleNavMeshDebugDraw(bool state);
    inline bool navMeshDebugDraw() const noexcept { return _navMeshDebugDraw; }

    inline void stop() noexcept {
        _shouldStop = true;
    }

    inline bool running() const noexcept {
        return _running;
    }

  protected:
    friend class AITeam;
    /// Register an AI Team
    void registerTeam(AITeam* const team);
    /// Unregister an AI Team
    void unregisterTeam(AITeam* const team);

    bool shouldStop() const;

  private:
    bool processInput(const U64 deltaTimeUS);    ///< sensors
    bool processData(const U64 deltaTimeUS);     ///< think
    bool updateEntities(const U64 deltaTimeUS);  ///< react

  private:
    TaskPool& _parentPool;
    Task* _activeTask;
    U64 _deltaTimeUS, _currentTimeUS, _previousTimeUS;
    std::atomic_bool _navMeshDebugDraw;
    std::atomic_bool _pauseUpdate;
    std::atomic_bool _updating;
    std::atomic_bool _shouldStop;
    std::atomic_bool _running;
    NavMeshMap _navMeshes;
    AITeamMap _aiTeams;
    mutable Mutex _updateMutex;
    mutable SharedMutex _navMeshMutex;
    DELEGATE<void> _sceneCallback;

};

};  // namespace AI
};  // namespace Divide
#endif