#include "Headers/Unit.h" 
#include "Graphs/Headers/SceneGraphNode.h" 
#include "Rendering/Headers/Framerate.h"

Unit::Unit(UnitType type, SceneGraphNode* const node) : _type(type), 
													    _node(node),
													    _moveSpeed(1 metre),
													    _moveTolerance(0.1f),
													    _prevTime(0){}
Unit::~Unit(){}

/// Pathfinding, collision detection, animation playback should all be controlled from here
bool Unit::moveTo(const vec3<F32>& targetPosition){
	WriteLock w_lock(_unitUpdateMutex);
	/// We should always have a node
	assert(_node != NULL);
	/// We receive move request every frame for now (or every event tick)
	/// Start plotting a course from our current position
	_currentPosition = _node->getTransform()->getPosition();
	_currentTargetPosition = targetPosition;
	
	if(_prevTime <= 0) _prevTime = GETMSTIME();
	/// Get current time in ms
	U32 currentTime = GETMSTIME();
	/// figure out how many milliseconds have elapsed since last move time
    U32 timeDif = currentTime - _prevTime;
	/// 'moveSpeed' m/s = '0.001 * moveSpeed' m / ms
	/// distance = timeDif * 0.001 * moveSpeed
	F32 moveDistance = _moveSpeed * (getMsToSec(timeDif));
	/// apply framerate varyance
	F32 speedFactor = FRAME_SPEED_FACTOR;
	moveDistance *= speedFactor;
	/// update previous time
	_prevTime = currentTime;

	/// Check if the current request is already processed
    F32 xDelta = _currentTargetPosition.x - _currentPosition.x;
    F32 yDelta = _currentTargetPosition.y - _currentPosition.y;
	F32 zDelta = _currentTargetPosition.z - _currentPosition.z;

	/// Compute the destination point for current frame step
	vec3<F32> interpPosition;

	if(!IS_TOLERANCE(yDelta,_moveTolerance) && !IS_ZERO(_moveSpeed)){
		if( !IS_ZERO( yDelta ) ){
			interpPosition.y = ( _currentPosition.y > _currentTargetPosition.y ? -moveDistance : moveDistance );
		}
	}

	if(!IS_TOLERANCE(xDelta,_moveTolerance) || !IS_TOLERANCE(zDelta,_moveTolerance) && !IS_ZERO(moveDistance)) {
		/// Update target
		if( IS_ZERO( xDelta ) ){
            interpPosition.z = ( _currentPosition.z > _currentTargetPosition.z ? -moveDistance : moveDistance );
		}else if( IS_ZERO( zDelta ) ){
            interpPosition.x = ( _currentPosition.x > _currentTargetPosition.x ? -moveDistance : moveDistance );
		}else if( fabs( xDelta ) > fabs( zDelta ) ) {
            F32 value = fabs( zDelta / xDelta ) * moveDistance;
            interpPosition.z = ( _currentPosition.z > _currentTargetPosition.z ? -value : value );
            interpPosition.x = ( _currentPosition.x > _currentTargetPosition.x ? -moveDistance : moveDistance );
         }else {
            F32 value = fabs( xDelta / zDelta ) * moveDistance;
            interpPosition.x = ( _currentPosition.x > _currentTargetPosition.x ? -value : value );
            interpPosition.z = ( _currentPosition.z > _currentTargetPosition.z ? -moveDistance : moveDistance );
         }
		/// commit transformations
		_node->getTransform()->translate(interpPosition);
		/// Update current position
		_currentPosition = _node->getTransform()->getPosition();
		return false; ///< no
	}else{
		return true; ///< yes
	}
}

/// Move along the X axis
bool Unit::moveToX(const F32 targetPosition){
	/// Update current position
	WriteLock w_lock(_unitUpdateMutex);
	_currentPosition = _node->getTransform()->getPosition();
	w_lock.unlock();
	return moveTo(vec3<F32>(targetPosition,_currentPosition.y,_currentPosition.z));
}
/// Move along the Y axis
bool Unit::moveToY(const F32 targetPosition){
	/// Update current position
	WriteLock w_lock(_unitUpdateMutex);
	_currentPosition = _node->getTransform()->getPosition();
	w_lock.unlock();
	return moveTo(vec3<F32>(_currentPosition.x,targetPosition,_currentPosition.z));
}
/// Move along the Z axis
bool Unit::moveToZ(const F32 targetPosition){
	/// Update current position
	WriteLock w_lock(_unitUpdateMutex);
	_currentPosition = _node->getTransform()->getPosition();
	w_lock.unlock();
	return moveTo(vec3<F32>(_currentPosition.x,_currentPosition.y,targetPosition));
}

/// Further improvements may imply a cooldown and collision detection at destination (thus the if-check at the end)
bool Unit::teleportTo(const vec3<F32>& targetPosition){
	/// We should always have a node
	assert(_node != NULL);
	WriteLock w_lock(_unitUpdateMutex);
	/// We receive move request every frame for now (or every event tick)
	/// Check if the current request is already processed
	if(!_currentTargetPosition.compare(targetPosition,0.00001f)){
		/// Update target
		_currentTargetPosition = targetPosition;
	}

	/// Start plotting a course from our current position
	_currentPosition = _node->getTransform()->getPosition();
	/// teleport to desired position
	_node->getTransform()->setPosition(_currentTargetPosition);
	/// Update current position
	_currentPosition = _node->getTransform()->getPosition();
	/// And check if we arrived
	if(_currentTargetPosition.compare(_currentPosition,0.0001f)){
		return true; ///< yes
	}

	return false; ///< no
}