#include "stdafx.h"

#include "Headers/Unit.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/FrameListenerManager.h"

namespace Divide {

Unit::Unit(UnitType type)
    : FrameListener(),
      _type(type),
      _moveSpeed(Metric::Base(1.0f)),
      _acceleration(Metric::Base(1.0f)),
      _moveTolerance(0.1f),
      _prevTime(0)
{
    REGISTER_FRAME_LISTENER(this, 5);
}

Unit::~Unit()
{ 
    UNREGISTER_FRAME_LISTENER(this);
}

void Unit::setParentNode(SceneGraphNode_ptr node) {
    _node = node;
    _currentPosition = _node.lock()->get<TransformComponent>()->getPosition();
}

/// Pathfinding, collision detection, animation playback should all be
/// controlled from here
bool Unit::moveTo(const vec3<F32>& targetPosition) {
    // We should always have a node
    SceneGraphNode_ptr sgn = _node.lock();
    if (!sgn) {
        return false;
    }
    WriteLock w_lock(_unitUpdateMutex);
    // We receive move request every frame for now (or every task tick)
    // Start plotting a course from our current position
    _currentPosition = sgn->get<TransformComponent>()->getPosition();
    _currentTargetPosition = targetPosition;

    if (_prevTime <= 0) {
        _prevTime = Time::ElapsedMilliseconds();
    }
    // get current time in ms
    D64 currentTime = Time::ElapsedMilliseconds();
    // figure out how many milliseconds have elapsed since last move time
    D64 timeDif = currentTime - _prevTime;
    CLAMP<D64>(timeDif, 0, timeDif);
    // update previous time
    _prevTime = currentTime;
    // 'moveSpeed' m/s = '0.001 * moveSpeed' m / ms
    // distance = timeDif * 0.001 * moveSpeed
    F32 moveDistance = std::min(to_F32(_moveSpeed * (Time::MillisecondsToSeconds(timeDif))),
                                0.0f);

    bool returnValue = IS_TOLERANCE(moveDistance, Metric::Centi(1.0f));

    if (!returnValue) {
        F32 xDelta = _currentTargetPosition.x - _currentPosition.x;
        F32 yDelta = _currentTargetPosition.y - _currentPosition.y;
        F32 zDelta = _currentTargetPosition.z - _currentPosition.z;
        bool xTolerance = IS_TOLERANCE(xDelta, _moveTolerance);
        bool yTolerance = IS_TOLERANCE(yDelta, _moveTolerance);
        bool zTolerance = IS_TOLERANCE(zDelta, _moveTolerance);

        // Compute the destination point for current frame step
        vec3<F32> interpPosition;
        if (!yTolerance && !IS_ZERO(yDelta)) {
            interpPosition.y =
                (_currentPosition.y > _currentTargetPosition.y ? -moveDistance
                                                               : moveDistance);
        }
        if ((!xTolerance || !zTolerance)) {
            // Update target
            if (IS_ZERO(xDelta)) {
                interpPosition.z =
                    (_currentPosition.z > _currentTargetPosition.z
                         ? -moveDistance
                         : moveDistance);
            } else if (IS_ZERO(zDelta)) {
                interpPosition.x =
                    (_currentPosition.x > _currentTargetPosition.x
                         ? -moveDistance
                         : moveDistance);
            } else if (std::fabs(xDelta) > std::fabs(zDelta)) {
                F32 value = std::fabs(zDelta / xDelta) * moveDistance;
                interpPosition.z =
                    (_currentPosition.z > _currentTargetPosition.z ? -value
                                                                   : value);
                interpPosition.x =
                    (_currentPosition.x > _currentTargetPosition.x
                         ? -moveDistance
                         : moveDistance);
            } else {
                F32 value = std::fabs(xDelta / zDelta) * moveDistance;
                interpPosition.x =
                    (_currentPosition.x > _currentTargetPosition.x ? -value
                                                                   : value);
                interpPosition.z =
                    (_currentPosition.z > _currentTargetPosition.z
                         ? -moveDistance
                         : moveDistance);
            }
            // commit transformations
            sgn->get<TransformComponent>()->translate(interpPosition);
        }
    }

    return returnValue;
}

/// Move along the X axis
bool Unit::moveToX(const F32 targetPosition) {
    SceneGraphNode_ptr sgn = _node.lock();
    if (!sgn) {
        return false;
    }
    {
        /// Update current position
        WriteLock w_lock(_unitUpdateMutex);
        _currentPosition = sgn->get<TransformComponent>()->getPosition();
    }
    return moveTo(vec3<F32>(targetPosition,
                            _currentPosition.y,
                            _currentPosition.z));
}

/// Move along the Y axis
bool Unit::moveToY(const F32 targetPosition) {
    SceneGraphNode_ptr sgn = _node.lock();
    if (!sgn) {
        return false;
    }
    {
        /// Update current position
        WriteLock w_lock(_unitUpdateMutex);
        _currentPosition = sgn->get<TransformComponent>()->getPosition();
    }
    return moveTo(vec3<F32>(_currentPosition.x,
                            targetPosition,
                            _currentPosition.z));
}

/// Move along the Z axis
bool Unit::moveToZ(const F32 targetPosition) {
    SceneGraphNode_ptr sgn = _node.lock();
    if (!sgn) {
        return false;
    }
    {
        /// Update current position
        WriteLock w_lock(_unitUpdateMutex);
        _currentPosition = sgn->get<TransformComponent>()->getPosition();
    }
    return moveTo(vec3<F32>(_currentPosition.x,
                            _currentPosition.y,
                            targetPosition));
}

/// Further improvements may imply a cooldown and collision detection at
/// destination (thus the if-check at the end)
bool Unit::teleportTo(const vec3<F32>& targetPosition) {
    SceneGraphNode_ptr sgn = _node.lock();
    if (!sgn) {
        return false;
    }
    WriteLock w_lock(_unitUpdateMutex);
    /// We receive move request every frame for now (or every task tick)
    /// Check if the current request is already processed
    if (!_currentTargetPosition.compare(targetPosition, 0.00001f)) {
        /// Update target
        _currentTargetPosition = targetPosition;
    }
    TransformComponent* nodeTransformComponent =
        sgn->get<TransformComponent>();
    /// Start plotting a course from our current position
    _currentPosition = nodeTransformComponent->getPosition();
    /// teleport to desired position
    nodeTransformComponent->setPosition(_currentTargetPosition);
    /// Update current position
    _currentPosition = nodeTransformComponent->getPosition();
    /// And check if we arrived
    if (_currentTargetPosition.compare(_currentPosition, 0.0001f)) {
        return true;  ///< yes
    }

    return false;  ///< no
}

void Unit::setAttribute(U32 attributeID, I32 initialValue) {
    _attributes[attributeID] = initialValue;
}

I32 Unit::getAttribute(U32 attributeID) const {
    AttributeMap::const_iterator it = _attributes.find(attributeID);
    if (it != std::end(_attributes)) {
        return it->second;
    }

    return -1;
}

};