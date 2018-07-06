/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _AI_MANAGER_H_
#define _AI_MANAGER_G_

#include "core.h"
#include "AI/Headers/AIEntity.h"
#include "AI/PathFinding/NavMeshes/Headers/NavMesh.h"

#include <boost/atomic.hpp>
DEFINE_SINGLETON(AIManager)
	typedef Unordered_map<I64, AIEntity*> AIEntityMap;

public:
	U8 tick();
    ///Handle any debug information rendering (nav meshes, AI paths, etc);
    ///Called by Scene::postRender after depth map preview call
    void debugDraw(bool forceAll = true);
	///Add an AI Entity from the manager
	bool addEntity(AIEntity* entity);
	///Remove an AI Entity from the manager
	void destroyEntity(U32 guid);
    ///Add a nav mesh
    bool addNavMesh(Navigation::NavigationMesh* const navMesh);
    ///Remove a nav mesh
    void destroyNavMesh(Navigation::NavigationMesh* const navMesh);
	/// Destroy all entities
	void Destroy();
	inline void setSceneCallback(boost::function0<void> callback) {WriteLock w_lock(_updateMutex); _sceneCallback = callback;}
	///Toggle NavMesh debugDraw
    void toggleNavMeshDebugDraw(bool state);

protected:
    AIManager();
private:
	void processInput();  ///< sensors
	void processData();   ///< think
	void updateEntities();///< react

private:
    boost::atomic<bool> _navMeshDebugDraw;
	AIEntityMap _aiEntities;
	mutable SharedLock _updateMutex;
    mutable SharedLock _navMeshMutex;
	boost::function0<void> _sceneCallback;
    vectorImpl<Navigation::NavigationMesh* > _navMeshes;
END_SINGLETON

#endif