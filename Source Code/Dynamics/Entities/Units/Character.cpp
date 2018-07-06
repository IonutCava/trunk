#include "Headers/Character.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Core/Math/Headers/Transform.h"
#include "AI/PathFinding/Headers/DivideCrowd.h"
#include "AI/PathFinding/Headers/DivideRecast.h"
#include "AI/PathFinding/NavMeshes/Headers/NavMesh.h"

static const D32 DESTINATION_RADIUS = 1 * 1;

Character::Character(CharacterType type, SceneGraphNode* const node) : Unit(Unit::UNIT_TYPE_CHARACTER, node),
                                                                       _type(type),
                                                                       _agent(NULL),
                                                                       _agentID(-1),
                                                                       _stopped(false),
                                                                       _agentControlled(true)
{
    _lookingDirection.set(1,0,0);
}

Character::~Character()
{
    if(_detourCrowd)
        _detourCrowd->removeAgent(getAgentID());
    _agentID = -1;
    _agent = NULL;
}

void Character::resetCrowd(Navigation::DivideDtCrowd* const crowd){
    _detourCrowd = crowd;
    if(_detourCrowd)
        _destination = _detourCrowd->getLastDestination();
}

D32 Character::getAgentHeight() const {
    return _detourCrowd ? _detourCrowd->getAgentHeight() : 0.0;
}

D32 Character::getAgentRadius() const {
    return _detourCrowd ? _detourCrowd->getAgentRadius() : 0.0;
}

void Character::update(const D32 deltaTime) {
}

void Character::updateDestination(const vec3<F32>& destination, bool updatePreviousPath ){
    if(!_agentControlled || !isLoaded())
        return;
    vec3<F32> result;
    // Find position on navmesh
    if(!Navigation::DivideRecast::getInstance().findNearestPointOnNavmesh(_detourCrowd->getNavMesh() , destination, result))
        return;

    _detourCrowd->setMoveTarget(_agentID, result, updatePreviousPath);
    _destination = result;
    _stopped = false;
    _manualVelocity.reset();
}

vec3<F32> Character::getDestination() const {
    if (_agentControlled && isLoaded())
        return _destination;

    return vec3<F32>();     // TODO this is not ideal
}

void Character::setPosition(const vec3<F32> position) {
    if(!_agentControlled || !isLoaded()) {
        _node->getTransform()->setPosition(position);
        return;
    }
    vec3<F32> result;
    // Find position on navmesh
    if (!Navigation::DivideRecast::getInstance().findNearestPointOnNavmesh(_detourCrowd->getNavMesh(), position, result))
        return;

    // Remove agent from crowd and re-add at position
    _detourCrowd->removeAgent(_agentID);
    _agentID = _detourCrowd->addAgent(result);
    _agent = _detourCrowd->getAgent(_agentID);

    _node->getTransform()->setPosition(position);
}

vec3<F32> Character::getPosition() const {
    return _node->getTransform()->getPosition();
}

bool Character::destinationReached(){
    if(!isLoaded())
        return false;

    if(getPosition().distanceSquared(getDestination()) <= DESTINATION_RADIUS)
        return true;


    return _detourCrowd->destinationReached(getAgent(), DESTINATION_RADIUS);
}

void Character::setDestination(const vec3<F32>& destination) {
    if (!_agentControlled || !isLoaded())
        return;

    _destination = destination;
    _manualVelocity.reset();
    _stopped = false;
}

void Character::moveForward(){
    vec3<F32> lookDirection = getLookingDirection();
    lookDirection.normalize();

    setVelocity(lookDirection * getMaxSpeed());
}

void Character::setVelocity(const vec3<F32>& velocity){
    _manualVelocity = velocity;
    _stopped = false;
    _destination.reset();

    if(_agentControlled && isLoaded())
        _detourCrowd->requestVelocity(getAgentID(), _manualVelocity);
}

void Character::stop(){
    if(!_agentControlled || !isLoaded()) {
        _manualVelocity.reset();
        _stopped = true;
        return;
    }

    if(_detourCrowd->stopAgent(getAgentID())) {
        _destination.reset();
        _manualVelocity.reset();
        _stopped = true;
    }
}

vec3<F32> Character::getVelocity() const {
    if(!isLoaded())
        return vec3<F32>();

    return (_agentControlled ? vec3<F32>(getAgent()->nvel) : _manualVelocity);
}

D32 Character::getMaxSpeed(){
    return (isLoaded() ? getAgent()->params.maxSpeed : 0.0);
}

D32 Character::getMaxAcceleration(){
    return (isLoaded()  ? getAgent()->params.maxAcceleration : 0.0);
}

vec3<F32> Character::getLookingDirection(){
    return _node->getTransform()->getOrientation() * getRelativeLookingDirection();
}

void Character::setAgentControlled(bool agentControlled){
    if(!_detourCrowd)
        return;

    if (_agentControlled != agentControlled) {
        if (agentControlled) {
            _agentID = _detourCrowd->addAgent(getPosition());
            _destination = _detourCrowd->getLastDestination();
            _manualVelocity.reset();
            _stopped = true;
        } else {
            _detourCrowd->removeAgent(_agentID);
            _agentID = -1;
            _destination.reset();
            _stopped = false;
        }
        _agentControlled = agentControlled;
    }
}

