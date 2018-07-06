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
#ifndef _AI_ENTITY_H_
#define _AI_ENTITY_H_

#include "Core/Headers/cdigginsAny.h"
#include "AI/Headers/GOAPContext.h"
#include "AI/Sensors/Headers/VisualSensor.h"
#include "AI/ActionInterface/Headers/AITeam.h"
#include "Utility/Headers/GUIDWrapper.h"
#include <AesopPlanner.h>

class AISceneImpl;
class SceneGraphNode;
class NPC;
enum  AIMsg; //< scene dependent message list
struct dtCrowdAgent;
namespace Navigation {
    class DivideRecast;
    class DivideDtCrowd;
    class NavigationMesh;
};

/// Based on OgreCrowd.
class AIEntity : public GUIDWrapper {
    friend class AIManager;

public:
    AIEntity(const vec3<F32>& currentPosition, const std::string& name);
    ~AIEntity();

    void load(const vec3<F32>& position);
    void unload();

    bool addSensor(SensorType type, Sensor* sensor);
    bool addAISceneImpl(AISceneImpl* AISceneImpl);
    bool addGOAPPlanner(const Aesop::Planner& planner, bool startOnAdd = false);
    void sendMessage(AIEntity* receiver,  AIMsg msg, const cdiggins::any& msg_content);
    void receiveMessage(AIEntity* sender, AIMsg msg, const cdiggins::any& msg_content);
    void processMessage(AIEntity* sender, AIMsg msg, const cdiggins::any& msg_content);
    Sensor* getSensor(SensorType type);

    inline AITeam* getTeam() {return _coordination; }
    inline U32  getTeamID() const    {if(_coordination != nullptr) { return _coordination->getTeamID();} return 0; }

    inline void updateGOAPPlan() { _updateGOAPPlan  = true; }
    ///Set a team for this Entity. If the entity belonged to a different team, remove it from that team first
    void setTeam(AITeam* const coordination);
    ///Add a friend to our team
    bool addFriend(AIEntity* const friendEntity);

    const std::string& getName() const {return _name;}

           void addUnitRef(NPC* const npc);
    inline NPC* getUnitRef()                {return _unitRef;}

    /// PathFinding
    /// Index ID identifying the agent of this character in the crowd
    inline I32 getAgentID() const { return _agentID; }
    /// The agent that steers this character within the crowd
    inline const dtCrowdAgent* getAgent() const { return _agent; }
    inline bool  isAgentLoaded() const { return _agentID >= 0; }
     /// Set the crowd object
    void resetCrowd(Navigation::DivideDtCrowd* const crowd = nullptr);
    /// The height of the agent for this character.
    D32 getAgentHeight() const;
    /// The radius of the agent for this character.
    D32 getAgentRadius() const;
    /**
      * Update the destination for this agent.
      * If updatePreviousPath is set to true the previous path will be reused instead
      * of calculating a completely new path, but this can only be used if the new
      * destination is close to the previous (eg. when chasing a moving entity).
    **/
    void updateDestination(const vec3<F32>& destination, bool updatePreviousPath = false);
    /// The destination set for this agent.
    const vec3<F32>& getDestination() const;
    /// Returns true when this agent has reached its set destination.
    bool destinationReached();
    /// Place agent at new position.
    void setPosition(const vec3<F32> position);
    /// The current position of the agent. Is only up to date once update() has been called in a frame.
    const vec3<F32>& getPosition() const;
    /// The maximum speed this character can attain.
    /// This parameter is configured for the agent controlling this character.
    D32 getMaxSpeed();
    /// The maximum acceleration this character has towards its maximum speed.
    /// This parameter is configured for the agent controlling this character.
    D32 getMaxAcceleration();
     /**
      * Request to set a manual velocity for this character, to control it
      * manually.
      * The set velocity will stay active, meaning that the character will
      * keep moving in the set direction, until you stop() it.
      * Manually controlling a character offers no absolute control as the
      * laws of acceleration and max speed still apply to an agent, as well
      * as the fact that it cannot steer off the navmesh or into other agents.
      * You will notice small corrections to steering when walking into objects
      * or the border of the navmesh (which is usually a wall or object).
    **/
    void setVelocity(const vec3<F32>& velocity);
    /// Manually control the character moving it forward.
    void moveForward();
    /// Manually control the character moving it backwards.
    void moveBackwards();
    /// Stop any movement this character is currently doing. This means losing the requested velocity or target destination.
    void stop();
    /// The current velocity (speed and direction) this character is traveling at.
    vec3<F32> getVelocity() const;
    /// The current speed this character is traveling at.
    D32 getSpeed()  const { return getVelocity().length(); }
    /// Returns true if this character is moving.
    bool isMoving() const  {return !_stopped || !IS_ZERO(getSpeed()); }

protected:
    /**
      * Update current position of this character to the current position of its agent.
    **/
    void updatePosition(const U64 deltaTime);
        /**
      * Set destination member variable directly without updating the agent state.
      * Usually you should call updateDestination() externally, unless you are controlling
      * the agents directly and need to update the corresponding character class to reflect
      * the change in state (see OgreRecastApplication friendship).
    **/
    void setDestination(const vec3<F32>& destination);

protected:
    friend class AIManager;
    void processInput(const U64 deltaTime);
    void processData(const U64 deltaTime);
    void update(const U64 deltaTime);

private:
    std::string           _name;
    AITeam*               _coordination;
    AISceneImpl*          _AISceneImpl;
    GOAPContext           _goapContext;
    Aesop::Planner        _goapPlanner;

    mutable SharedLock    _updateMutex;
    mutable SharedLock    _managerQueryMutex;

    typedef Unordered_map<SensorType, Sensor*> sensorMap;
    sensorMap               _sensorList;
    NPC*                    _unitRef;
    /// PathFinding
    /// ID of mAgent within the crowd.
    I32 _agentID;
    /// Crowd in which the agent of this character is.
    Navigation::DivideDtCrowd* _detourCrowd;
    /// The agent controlling this character.
    const dtCrowdAgent* _agent;
    /**
      * The current destination set for this agent.
      * Take care in properly setting this variable, as it is only updated properly when
      * using Character::updateDestination() to set an individual destination for this character.
      * After updating the destination of all agents this variable should be set externally using
      * setDestination().
    **/
    vec3<F32> _destination;
    vec3<F32> _currentPosition;  
    vec3<F32> _currentVelocity;
    F32       _distanceToTarget;
    F32       _previousDistanceToTarget;
    U64       _moveWaitTimer;
    /// True if this character is stopped.
    bool _stopped;
    /// True if the GOAP plan needs an update
    bool _updateGOAPPlan;
};

#endif