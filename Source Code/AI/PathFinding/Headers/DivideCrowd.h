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

/*
    OgreCrowd
    ---------

    Copyright (c) 2012 Jonas Hauquier

    Additional contributions by:

    - mkultra333
    - Paul Wilson

    Sincere thanks and to:

    - Mikko Mononen (developer of Recast navigation libraries)

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.

*/

#ifndef _DIVIDE_CROWD_H_
#define _DIVIDE_CROWD_H_

#include "Core/Math/Headers/MathClasses.h"
#include "DetourCrowd/Include/DetourCrowd.h"
#include "../NavMeshes/Headers/NavMesh.h"
#include <vector>


/**
  * Divide wrapper around Ogre wrapper around DetourCrowd.
  * Controls a crowd of agents that can steer to avoid each other and follow
  * individual paths.
  *
  * This class is largely based on the CrowdTool used in the original recastnavigation
  * demo.
  **/
namespace AI {

namespace Navigation {

    class NavigationMesh;

    class DivideDtCrowd {
    public:
        /**
          * Initialize a detour crowd that will manage agents on the specified
          * recast navmesh. It does not matter how this navmesh is constructed
          * (either with OgreRecast directly or with DetourTileCache).
          * Parameters such as agent dimensions will be taken from the specified
          * recast component.
          **/
        DivideDtCrowd(NavigationMesh *navMesh);
        ~DivideDtCrowd();
        /// Add an agent to the crowd:  Returns ID of created agent (-1 if maximum agents is already created)
        I32 addAgent(const vec3<F32>& position, F32 maxSpeed, F32 acceleration);
        /// Retrieve agent with specified ID from the crowd.
        inline const dtCrowdAgent* getAgent(I32 id) const { return _crowd->getAgent(id); }

        /// Remove agent with specified ID from the crowd.
        void removeAgent(const I32 idx);
        /**
          * Set global destination or target for all agents in the crowd.
          * Setting adjust to true will try to adjust the current calculated path
          * of the agents slightly to end at the new destination, avoiding the need
          * to calculate a completely new path. This only works if the destination is
          * close to the previously set one, for example when chasing a moving entity.
          **/
        void setMoveTarget(const vec3<F32>& position, bool adjust);
        /**
          * Set target or destination for an individual agent.
          * Setting adjust to true will try to adjust the current calculated path
          * of the agent slightly to end at the new destination, avoiding the need
          * to calculate a completely new path. This only works if the destination is
          * close to the previously set one, for example when chasing a moving entity.
          **/
        void setMoveTarget(I32 agentId, const vec3<F32>&position, bool adjust);
        /**
          * Request a specified velocity for the agent with specified index.
          * Requesting a velocity means manually controlling an agent.
          * Returns true if the request was successful.
          **/
        bool requestVelocity(I32 agentId, const vec3<F32>& velocity);
        /// Cancels any request for the specified agent, making it stop.
        /// Returns true if the request was successul.
        bool stopAgent(I32 agentId);
        /**
          * Helper that calculates the needed velocity to steer an agent to a target destination.
          * Parameters:
          *     position    is the current position of the agent
          *     target      is the target destination to reach
          *     speed       is the (max) speed the agent can travel at
          * Returns the calculated velocity.
          *
          * This function can be used together with requestMoveVelocity to achieve the functionality
          * of adjustMoveTarget function.
          **/
        static vec3<F32> calcVel(const vec3<F32>& position, const vec3<F32>& target, D32 speed);
        static F32       getDistanceToGoal(const dtCrowdAgent* agent, const F32 maxRange);
        static bool      destinationReached(const dtCrowdAgent* agent, const F32 maxDistanceFromTarget);
        /**
          * Update method for the crowd manager. Will calculate new positions for moving agents.
          * Call this method in your frameloop every frame to make your agents move.
          *
          * DetourCrowd uses sampling based local steering to calculate a velocity vector for each
          * agent. The calculation time of this is limited to the number of agents in the crowd and
          * the sampling amount (which is a constant).
          *
          * Additionally pathfinding tasks are queued and the number of computations is limited, to
          * limit the maximum amount of time spent for preparing a frame. This can have as consequence
          * that some agents will only start to move after a few frames, when their paths are calculated.
          **/
        void update(const U64 deltaTime);
        /// The height of agents in this crowd. All agents in a crowd have the same height, and height is
        /// determined by the agent height parameter with which the navmesh is build.
        inline D32 getAgentHeight() const { return _recast->getConfigParams().getAgentHeight(); }
        /// The radius of agents in this crowd. All agents in a crowd have the same radius, and radius
        /// determined by the agent radius parameter with which the navmesh is build.
        inline D32 getAgentRadius() const { return _recast->getConfigParams().getAgentRadius(); }
        /// The number of (active) agents in this crowd.
        inline I32 getNbAgents()    const { return _activeAgents; }
        /// Get the navigation mesh associated with this crowd
        inline const NavigationMesh& getNavMesh() {return *_recast;}
        /// Check if the navMesh is valid
        bool isValidNavMesh() const;
        /// Change the navigation mesh for this crowd
        inline void setNavMesh(NavigationMesh* navMesh) {_recast = navMesh;}
        /// The maximum number of agents that are allowed in this crowd.
        inline I32 getMaxNbAgents() const { return _crowd->getAgentCount(); }
        /// Get all (active) agents in this crowd.
        vectorImpl<dtCrowdAgent* > getActiveAgents(void);
        /// Get the IDs of all (active) agents in this crowd.
        vectorImpl<I32 > getActiveAgentIds(void);
        /// The last set destination for the crowd. This is the destination that will be assigned to newly added agents.
        inline vec3<F32> DivideDtCrowd::getLastDestination() const { return vec3<F32>(_targetPos); }
        /// Reference to the DetourCrowd object that is wrapped.
        dtCrowd* _crowd;
        /// Reference to the Recast/Detour wrapper object for Divide.
        NavigationMesh* _recast;
        /// The latest set target or destination section in the recast navmesh.
        dtPolyRef _targetRef;
        /// The latest set target or destination position.
        F32 _targetPos[3];
        /// Max pathlength for calculated paths.
        static const I32 AGENT_MAX_TRAIL = 64;
        /// Max number of agents allowed in this crowd.
        static const I32 MAX_AGENTS = 128;
        /// Stores the calculated paths for each agent in the crowd.
        struct AgentTrail {
            F32 trail[AGENT_MAX_TRAIL * 3];
            I32 htrail;
        };
        AgentTrail _trails[MAX_AGENTS];
        /// Debug info object used in the original recast/detour demo, not used in this application.
        dtCrowdAgentDebugInfo _agentDebug;
        /// Parameters for obstacle avoidance of DetourCrowd steering.
        dtObstacleAvoidanceDebugData* _vod;
        /// Agent configuration parameters
        bool _anticipateTurns;
        bool _optimizeVis;
        bool _optimizeTopo;
        bool _obstacleAvoidance;
        bool _separation;
        F32  _obstacleAvoidanceType;
        F32  _separationWeight;

    protected:
        /**
          * Helper to calculate the needed velocity to steer an agent to a target destination.
          * Parameters:
          *     velocity    is the return parameter, the calculated velocity
          *     position    is the current position of the agent
          *     target      is the target destination to reach
          *     speed       is the (max) speed the agent can travel at
          *
          * This function can be used together with requestMoveVelocity to achieve the functionality
          * of the old adjustMoveTarget function.
          **/
        static void calcVel(F32* velocity, const F32* position, const F32* target, const F32 speed);


    private:
        /// Number of (active) agents in the crowd.
        I32 _activeAgents;
    }; // DivideDtCrowd
}; // Navigation

}; //namespace AI
#endif