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

#ifndef _CHARACTER_H_
#define _CHARACTER_H_

#include "Unit.h"

struct dtCrowdAgent;
namespace Navigation {
    class DivideDtCrowd;
}
/// Basic character class. Based on OgreCrowd.
/// Use the internally calculated position to update the attached node's transform!
#pragma message("ToDo: Move most calculations from Character to Unit! -Ionut")
class Character : public Unit {
public:
    /// Currently supported character types
    enum CharacterType{
        /// user controlled character
        CHARACTER_TYPE_PLAYER,
        /// non-user(player) character
        CHARACTER_TYPE_NPC,
        /// placeholder
        CHARACTER_TYPE_PLACEHOLDER
    };

    Character(CharacterType type, SceneGraphNode* const node);
    ~Character();

    void load();
    void load(const vec3<F32>& position);
    void unload();

    /// Set unit type
    inline void setCharacterType(CharacterType type)      {_type = type;}
    /// Get unit type
    inline CharacterType getCharacterType()		   const  {return _type;}
    /// Set the crowd object
    void   resetCrowd(Navigation::DivideDtCrowd* const crowd = NULL);
    /// The height of the agent for this character.
    virtual D32 getAgentHeight() const;
    /// The radius of the agent for this character.
    virtual D32 getAgentRadius() const;
    /**
      * Update this character for drawing a new frame.
      * Updates one tick in the render loop.
      * In order for the agents to be updated, you first need to call the detourCrowd
      * update function.
      * What is updated specifically is up to a specific implementation of Character,
      * but at the very least the position in the scene should be updated to reflect
      * the detour agent position (possibly with additional physics engine clipping
      * and collision testing).
      **/
    virtual void update(const U64 deltaTime);
    /**
      * Update the destination for this agent.
      * If updatePreviousPath is set to true the previous path will be reused instead
      * of calculating a completely new path, but this can only be used if the new
      * destination is close to the previous (eg. when chasing a moving entity).
    **/
    virtual void updateDestination(const vec3<F32>& destination, bool updatePreviousPath = false);
    /// The destination set for this character.
    virtual vec3<F32> getDestination() const;
    /** Place character at new position.
      * The character will start following the globally set destination in the detourCrowd,
      * unless you give it an individual destination using updateDestination().
    **/
    virtual void setPosition(const vec3<F32> position);
    /// The current position of the agent. Is only up to date once update() has been called in a frame.
    virtual vec3<F32> getPosition() const;
    /// Index ID identifying the agent of this character in the crowd
    virtual I32 getAgentID() const { return _agentID; }
    /// The agent that steers this character within the crowd
    virtual const dtCrowdAgent* getAgent() const { return _agent; }
    /// Returns true when this character has reached its set destination.
    virtual bool destinationReached();
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
    virtual void moveForward();
    /// Stop any movement this character is currently doing. This means losing the requested velocity or target destination.
    virtual void stop();
    /// The current velocity (speed and direction) this character is traveling at.
    virtual vec3<F32> getVelocity() const;
    /// The current speed this character is traveling at.
    virtual D32 getSpeed() const { return getVelocity().length(); }
    /// The maximum speed this character can attain.
    /// This parameter is configured for the agent controlling this character.
    virtual D32 getMaxSpeed();
    /// The maximum acceleration this character has towards its maximum speed.
    /// This parameter is configured for the agent controlling this character.
    virtual D32 getMaxAcceleration();
    /// Returns true if this character is moving.
    virtual bool isMoving() const  {return !_stopped || !IS_ZERO(getSpeed()); }
    /// The direction in which the character is currently looking.
    virtual vec3<F32> getLookingDirection();
    /**
      * Set whether this character is controlled by an agent or whether it
      * will position itself independently based on the requested velocity.
      * Set to true to let the character be controlled by an agent.
      * Set to false to manually control it without agent, you need to set
      * detourTileCache first.
    **/
    void setAgentControlled(bool agentControlled);
    /// Determines whether this character is controlled by an agent.
            bool      isAgentControlled()                                     const { return _agentControlled;  } 
    virtual vec3<F32> getRelativeLookingDirection()                           const { return _lookingDirection; }
    virtual void      setRelativeLookingDirection(const vec3<F32>& direction)       { _lookingDirection = direction; }

protected:
    /**
      * Set destination member variable directly without updating the agent state.
      * Usually you should call updateDestination() externally, unless you are controlling
      * the agents directly and need to update the corresponding character class to reflect
      * the change in state (see OgreRecastApplication friendship).
    **/
    void setDestination(const vec3<F32>& destination);
    /**
      * Update current position of this character to the current position of its agent.
    **/
    virtual void updatePosition(const U64 deltaTime);
    inline bool isLoaded() const { return _agentID >= 0; }

private:
    CharacterType _type;
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
    /**
      * Velocity set for this agent for manually controlling it.
      * If this is not zero then a manually set velocity is currently controlling the movement
      * of this character (not pathplanning towards a set destination).
      **/
    vec3<F32> _manualVelocity;
    vec3<F32> _lookingDirection;
    /// True if this character is stopped.
    bool _stopped;
    /// True if character is controlled by agent.
    /// False if character is manually controlled without agent.
    bool _agentControlled;
};
#endif