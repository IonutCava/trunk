/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _UNIT_H_
#define _UNIT_H_

#include "Core/Math/Headers/MathMatrices.h"
#include "Platform/Threading/Headers/SharedMutex.h"
#include "Rendering/Headers/FrameListener.h"

namespace Divide {

class SceneGraphNode;
///Unit interface
class Unit  : public FrameListener {
public:
    /// Currently supported unit types
    enum UnitType{
        /// "Living beings"
        UNIT_TYPE_CHARACTER,
        /// e.g. Cars, planes, ships etc
        UNIT_TYPE_VEHICLE,
        /// add more types above this
        UNIT_TYPE_PLACEHOLDER
    };

    Unit(UnitType type, SceneGraphNode* const node);
    virtual ~Unit();

    /// moveTo makes the unit follow a path from it's current position to the targets position
    virtual bool moveTo(const vec3<F32>& targetPosition);
    virtual bool moveToX(const F32 targetPosition);
    virtual bool moveToY(const F32 targetPosition);
    virtual bool moveToZ(const F32 targetPosition);
    /// teleportTo instantly places the unit at the desired position
    virtual bool teleportTo(const vec3<F32>& targetPosition);

    /// Accesors
    /// Get the units position in the world
    inline const vec3<F32>& getCurrentPosition() const {return _currentPosition;}
    /// Get the units target coordinates in the world
    inline const vec3<F32>& getTargetPosition()  const {return _currentTargetPosition;}
    /// Set the units movement speed in metres per second (minimum is 0 = the unit does not move / is rooted)
    virtual void setMovementSpeed(F32 movementSpeed) {_moveSpeed = movementSpeed; CLAMP<F32>(_moveSpeed,0.0f,100.0f);}
    /// Get the units current movement speed
    virtual F32  getMovementSpeed()              const {return _moveSpeed;}
    /// Set destination tolerance
    inline void setMovementTolerance(F32 movementTolerance) {_moveTolerance = std::max(0.1f,movementTolerance);}
    /// Get the units current movement tolerance
    inline F32  getMovementTolerance()          const {return _moveTolerance;}
    /// Set unit type
    inline void setUnitType(UnitType type) {_type = type;}
    /// Get unit type
    inline UnitType getUnitType()               const {return _type;}
    /// Get bound node
    inline SceneGraphNode* const getBoundNode() const {return _node;} 
    /// Just before we render the frame
    virtual bool frameRenderingQueued(const FrameEvent& evt) {return true;}
    /// If the parent node is deleted, this gets called automatically
    inline void nodeDeleted() {  _node = nullptr; }
protected:
    /// Unit type
    UnitType _type;
    /// Movement speed (per second)
    F32 _moveSpeed;
    /// acceptable distance from target
    F32 _moveTolerance;
    /// previous time, in milliseconds when last move was applied
    D32 _prevTime;
    /// Unit position in world
    vec3<F32>  _currentPosition;
    /// Current destination point cached
    vec3<F32> _currentTargetPosition;
    /// SceneGraphNode the unit is managing (used for updating positions and checking collisions
    SceneGraphNode* _node;
    mutable SharedLock _unitUpdateMutex;
};

}; //namespace Divide

#endif