#include "Headers/Unit.h" 
#include "Graphs/Headers/SceneGraphNode.h" 
#include "Rendering/Headers/Framerate.h"

Unit::Unit(UNIT_TYPE type, SceneGraphNode* const node) : _type(type), 
													   _node(node),
													   _moveSpeed(1) {}
Unit::~Unit(){}

/// Pathfinding, collision detection, animation playback should all be controlled from here
bool Unit::moveTo(const vec3& targetPosition){
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
	/// Compute the destination point for current frame step
	vec3 interpPosition = (_currentTargetPosition - _currentPosition);
	/// update position based on move speed and framerate
	_node->getTransform()->translate((interpPosition / _moveSpeed) * Framerate::getInstance().getSpeedfactor());
	/// Update current position
	_currentPosition = _node->getTransform()->getPosition();
	/// And check if we arrived
	if(_currentTargetPosition.compare(_currentPosition,0.0001f)){
		return true; ///< yes
	}

	return false; ///< no
}

/// Further improvements may imply a cooldown and collision detection at destination (thus the if-check at the end)
bool Unit::teleportTo(const vec3& targetPosition){
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