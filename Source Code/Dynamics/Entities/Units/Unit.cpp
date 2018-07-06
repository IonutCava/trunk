#include "Headers/Unit.h" 
#include "Graphs/Headers/SceneGraphNode.h" 
#include "Rendering/Headers/Framerate.h"

Unit::Unit(UNIT_TYPE type, SceneGraphNode* const node) : _type(type), 
													   _node(node),
													   _moveSpeed(1),
													   _moveTolerance(0.1f),
													   _prevTime(GETMSTIME()){}
Unit::~Unit(){}

/// Pathfinding, collision detection, animation playback should all be controlled from here
bool Unit::moveTo(const vec3<F32>& targetPosition){

	/// We should always have a node
	assert(_node != NULL);
	/// We receive move request every frame for now (or every event tick)
	/// Start plotting a course from our current position
	_currentPosition = _node->getTransform()->getPosition();
	_currentTargetPosition = targetPosition;

	/// Get current time in ms
	F32 currentTime = GETMSTIME();
	/// figure out how many milliseconds have elapsed since last move time
    F32 timeDif = currentTime - _prevTime;
	/// 'moveSpeed' m/s = '0.001 * moveSpeed' m / ms
	/// distance = timeDif * 0.001 * moveSpeed
	F32 moveSpeed = _moveSpeed * 0.001 * timeDif;
	/// apply framerate varyance
	moveSpeed *= Framerate::getInstance().getSpeedfactor();
	/// update previous time
	_prevTime = currentTime;

	/// Check if the current request is already processed
    F32 xDelta = _currentTargetPosition.x - _currentPosition.x;
    F32 yDelta = _currentTargetPosition.y - _currentPosition.y;
	F32 zDelta = _currentTargetPosition.z - _currentPosition.z;

	/// Compute the destination point for current frame step
	vec3<F32> interpPosition;

	if(fabs(yDelta) > _moveTolerance && ( _moveSpeed > TEST_EPSILON) ){
		if( !IS_ZERO( yDelta ) ){
			interpPosition.y = ( _currentPosition.y > _currentTargetPosition.y ? -moveSpeed : moveSpeed );
		}
	}

	if((fabs(xDelta) > _moveTolerance || 
		fabs(zDelta) > _moveTolerance ) 
		&& ( _moveSpeed > TEST_EPSILON) ) {
		/// Update target


		if( IS_ZERO( xDelta ) ){
            interpPosition.z = ( _currentPosition.z > _currentTargetPosition.z ? -moveSpeed : moveSpeed );
		}else if( IS_ZERO( zDelta ) ){
            interpPosition.x = ( _currentPosition.x > _currentTargetPosition.x ? -moveSpeed : moveSpeed );
		}else if( fabs( xDelta ) > fabs( zDelta ) ) {
            F32 value = fabs( zDelta / xDelta ) * moveSpeed;
            interpPosition.z = ( _currentPosition.z > _currentTargetPosition.z ? -value : value );
            interpPosition.x = ( _currentPosition.x > _currentTargetPosition.x ? -moveSpeed : moveSpeed );
         }else {
            F32 value = fabs( xDelta / zDelta ) * moveSpeed;
            interpPosition.x = ( _currentPosition.x > _currentTargetPosition.x ? -value : value );
            interpPosition.z = ( _currentPosition.z > _currentTargetPosition.z ? -moveSpeed : moveSpeed );
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
	_currentPosition = _node->getTransform()->getPosition();
	return moveTo(vec3<F32>(targetPosition,_currentPosition.y,_currentPosition.z));
}
/// Move along the Y axis
bool Unit::moveToY(const F32 targetPosition){
	/// Update current position
	_currentPosition = _node->getTransform()->getPosition();
	return moveTo(vec3<F32>(_currentPosition.x,targetPosition,_currentPosition.z));
}
/// Move along the Z axis
bool Unit::moveToZ(const F32 targetPosition){
	/// Update current position
	_currentPosition = _node->getTransform()->getPosition();
	return moveTo(vec3<F32>(_currentPosition.x,_currentPosition.y,targetPosition));
}

/// Further improvements may imply a cooldown and collision detection at destination (thus the if-check at the end)
bool Unit::teleportTo(const vec3<F32>& targetPosition){
	/// We should always have a node
	assert(_node != NULL);
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