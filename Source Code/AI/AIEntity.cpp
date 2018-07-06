#include "Headers/AIEntity.h"
#include "ActionInterface/Headers/AISceneImpl.h"

#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "PathFinding/Waypoints/Headers/WaypointGraph.h"  ///< For waypoint movement
#include "PathFinding/NavMeshes/Headers/NavMesh.h" ///< For NavMesh movement
#include "Dynamics/Entities/Units/Headers/NPC.h"
#include "AI/PathFinding/Headers/DivideCrowd.h"
#include "AI/PathFinding/Headers/DivideRecast.h"
#include "AI/PathFinding/NavMeshes/Headers/NavMesh.h"
#include "Managers/Headers/AIManager.h"
#include <Aesop.h>

static const D32 DESTINATION_RADIUS = 1.5 * 1.5;

AIEntity::AIEntity(const vec3<F32>& currentPosition, const std::string& name)  : GUIDWrapper(),
                                                                                _name(name),
                                                                                _AISceneImpl(nullptr),
                                                                                _updateGOAPPlan(false),
                                                                                _unitRef(nullptr),
                                                                                _teamPtr(nullptr),
                                                                                _detourCrowd(nullptr),
                                                                                _agent(nullptr),
                                                                                _agentID(-1),
                                                                                _distanceToTarget(-1.f),
                                                                                _previousDistanceToTarget(-1.f),
                                                                                _moveWaitTimer(0ULL),
                                                                                _stopped(false)
{
    _currentPosition.set(currentPosition);
    _agentRadiusCategory = AGENT_RADIUS_SMALL;
}

AIEntity::~AIEntity()
{
    if (_detourCrowd) {
        _detourCrowd->removeAgent(getAgentID());
    }

    _agentID = -1;
    _agent = nullptr;

    SAFE_DELETE(_AISceneImpl);
    FOR_EACH(sensorMap::value_type& it , _sensorList) {
        SAFE_DELETE(it.second);
    }
    _sensorList.clear();
}

void AIEntity::load(const vec3<F32>& position) {
    setPosition(position);

    if (!isAgentLoaded() && _detourCrowd) {
        _agentID = _detourCrowd->addAgent(position, _unitRef ? _unitRef->getMovementSpeed() : (_detourCrowd->getAgentHeight() / 2)*3.5f, 10.0f);
        _agent = _detourCrowd->getAgent(_agentID);
        _destination = position;
    }
    
}

void AIEntity::unload() {
    if (!isAgentLoaded()) {
        return;
    }
    if (_detourCrowd) {
        _detourCrowd->removeAgent(getAgentID());
    }
    _agentID = -1;
    _agent = nullptr;
}

void AIEntity::sendMessage(AIEntity* receiver, AIMsg msg, const cdiggins::any& msg_content) {
    assert(receiver != nullptr);

    receiver->receiveMessage(this, msg, msg_content);
}

void AIEntity::receiveMessage(AIEntity* sender, AIMsg msg, const cdiggins::any& msg_content) {
    this->processMessage(sender, msg, msg_content);
}

void AIEntity::processMessage(AIEntity* sender, AIMsg msg, const cdiggins::any& msg_content) {
    assert(_AISceneImpl);

    ReadLock r_lock(_updateMutex);
    _AISceneImpl->processMessage(sender, msg, msg_content);
}

Sensor* AIEntity::getSensor(SensorType type) {
    if (_sensorList.find(type) != _sensorList.end()) {
        return _sensorList[type];
    }
    return nullptr;
}

bool AIEntity::addSensor(SensorType type, Sensor* sensor) {
    
    sensor->updatePosition(_currentPosition);
    if (_sensorList.find(type) != _sensorList.end()) {
        SAFE_UPDATE(_sensorList[type], sensor);
    } else {
        _sensorList.insert(std::make_pair(type,sensor));
    }
    return true;
}

bool AIEntity::addAISceneImpl(AISceneImpl* AISceneImpl) {
    assert(AISceneImpl);

    WriteLock w_lock(_updateMutex);
    SAFE_UPDATE(_AISceneImpl, AISceneImpl);
    _AISceneImpl->addEntityRef(this);
    _goapContext = _AISceneImpl->getGOAPContext();
    return true;
}

void AIEntity::processInput(const U64 deltaTime) {
    ReadLock r_lock(_managerQueryMutex);
    if (_AISceneImpl) {
        _AISceneImpl->processInput(deltaTime);
    }
}

void AIEntity::processData(const U64 deltaTime){
    ReadLock r_lock(_managerQueryMutex);
    if (_AISceneImpl) {
        _AISceneImpl->processData(deltaTime);
    }
}

void AIEntity::update(const U64 deltaTime){
    ReadLock r_lock(_managerQueryMutex);
    if (_AISceneImpl) {
        if (_updateGOAPPlan){
            _goapPlanner.plan(&_goapContext);
            _updateGOAPPlan = false;
        }
        _AISceneImpl->update(_unitRef);
    }
    if (_unitRef) {
        _unitRef->update(deltaTime);
        if (!_detourCrowd) {
            resetCrowd();
        }
    }
    updatePosition(deltaTime);
}

void AIEntity::setTeam(AITeam* const teamPtr) {
    ReadLock r_lock(_updateMutex);
    if (_teamPtr) {
        //Remove from old team
        _teamPtr->removeTeamMember(this);
    }
    //Update our team
    _teamPtr = teamPtr;
    //Add our self to the new team
    _teamPtr->addTeamMember(this);

    resetCrowd();
}

void AIEntity::addUnitRef(NPC* const npc) {
    _unitRef = npc;
    if (_unitRef) {
        load(_unitRef->getPosition());
    }
}

bool AIEntity::addFriend(AIEntity* const friendEntity) {
    ReadLock r_lock(_updateMutex);
    AITeam* friendTeam = friendEntity->getTeam();
    //If no team, check if our friend has a team and add our self to it
    if (!_teamPtr) {
        //If our friend has a team ...
        if (friendTeam) {
            ///Create friendship
            friendTeam->addTeamMember(this);
            _teamPtr = friendTeam;
            return true;
        }
        return false;
    }
    //If we have team, add friend to our team
    _teamPtr->addTeamMember(friendEntity);
    //If our friend isn't on our team, add him
    if (!friendTeam) {
        friendEntity->setTeam(_teamPtr);
    }
    return true;
}

D32 AIEntity::getAgentHeight() const {
    return _detourCrowd ? _detourCrowd->getAgentHeight() : 0.0;
}

D32 AIEntity::getAgentRadius() const {
    return _detourCrowd ? _detourCrowd->getAgentRadius() : 0.0;
}

void AIEntity::resetCrowd(){
    if (_detourCrowd) {
        unload();
    }

    _detourCrowd = AIManager::getInstance().getCrowd(_teamPtr->getTeamID(), getAgentRadiusCategory());

    if (_detourCrowd) {
        _destination = _detourCrowd->getLastDestination();
        load(_unitRef != nullptr ? _unitRef->getPosition() : vec3<F32>());
    }
}

void AIEntity::setPosition(const vec3<F32> position) {
    if (!isAgentLoaded()) {
        if (_unitRef) {
            _unitRef->setPosition(position);
        }
        return;
    }
    if (!_detourCrowd) {
        return;
    }
    vec3<F32> result;

    if (!_detourCrowd->isValidNavMesh()) {
        return;
    }
    // Find position on NavMesh
    if (!Navigation::DivideRecast::getInstance().findNearestPointOnNavmesh(_detourCrowd->getNavMesh(), position, result)) {
        return;
    }
    // Remove agent from crowd and re-add at position
    _detourCrowd->removeAgent(_agentID);
    _agentID = _detourCrowd->addAgent(result, (_detourCrowd->getAgentHeight()/2)*3.5f, 10.0f);
    _agent = _detourCrowd->getAgent(_agentID);

    if (_unitRef) {
        _unitRef->setPosition(position);
    }
}

void AIEntity::updatePosition(const U64 deltaTime){
    if(isAgentLoaded() && getAgent()->active){
        _previousDistanceToTarget = _distanceToTarget;
        _distanceToTarget = _currentPosition.distanceSquared(_destination);

        // if we are walking but did not change distance
        if(abs(_previousDistanceToTarget - _distanceToTarget) < DESTINATION_RADIUS){
            _moveWaitTimer += deltaTime;
            if(getUsToSec(_moveWaitTimer) > 5){
                _moveWaitTimer = 0;
                updateDestination(_detourCrowd->getNavMesh().getRandomPosition());
                return;
            }
        }else{
            _moveWaitTimer = 0;
        }
        _currentPosition.setV(getAgent()->npos);
        _currentVelocity.setV(getAgent()->nvel);
    }
    
    if(_unitRef){          
        _unitRef->setPosition(_currentPosition);
        _unitRef->setVelocity(_currentVelocity);
    }
}

void AIEntity::updateDestination(const vec3<F32>& destination, bool updatePreviousPath ){
    if(!isAgentLoaded())
        return;

    vec3<F32> result;

    // Find position on navmesh
    if(!Navigation::DivideRecast::getInstance().findNearestPointOnNavmesh(_detourCrowd->getNavMesh() , destination, result))
        return;

    _detourCrowd->setMoveTarget(_agentID, result, updatePreviousPath);
    _destination = result;
    _stopped = false;
}

const vec3<F32>& AIEntity::getPosition() const {
    return _currentPosition;
}

const vec3<F32>& AIEntity::getDestination() const {
    if (isAgentLoaded())
        return _destination;

    return getPosition();     // TODO this is not ideal
}

bool AIEntity::destinationReached(){
    if(!isAgentLoaded())
        return false;

    if(_currentPosition.distanceSquared(getDestination()) <= DESTINATION_RADIUS)
        return true;

    return _detourCrowd->destinationReached(getAgent(), DESTINATION_RADIUS);
}

void AIEntity::setDestination(const vec3<F32>& destination) {
    if (!isAgentLoaded())
        return;

    _destination = destination;
    _stopped = false;
}

void AIEntity::moveForward() {
    vec3<F32> lookDirection = _unitRef != nullptr ? _unitRef->getLookingDirection() : WORLD_Z_NEG_AXIS;
    lookDirection.normalize();

    setVelocity(lookDirection * getMaxSpeed());
}

void AIEntity::moveBackwards(){
    vec3<F32> lookDirection = _unitRef != nullptr ? _unitRef->getLookingDirection() : WORLD_Z_NEG_AXIS;
    lookDirection.normalize();

    setVelocity(lookDirection * getMaxSpeed() * -1.0f);
}

void AIEntity::setVelocity(const vec3<F32>& velocity){
    _stopped = false;
    _destination.reset();

    if (isAgentLoaded()) {
        _detourCrowd->requestVelocity(getAgentID(), velocity);
    }
}

void AIEntity::stop() {
    if (!isAgentLoaded()) {
        _stopped = true;
        return;
    }

    if (_detourCrowd->stopAgent(getAgentID())) {
        _destination.reset();
        _stopped = true;
    }
}

vec3<F32> AIEntity::getVelocity() const {
    return (isAgentLoaded() ? vec3<F32>(getAgent()->nvel) : vec3<F32>());
}

D32 AIEntity::getMaxSpeed(){
    return (isAgentLoaded() ? getAgent()->params.maxSpeed : 0.0);
}

D32 AIEntity::getMaxAcceleration(){
    return (isAgentLoaded()  ? getAgent()->params.maxAcceleration : 0.0);
}

bool AIEntity::addGOAPPlanner(const Aesop::Planner& planner, bool startOnAdd){
    _goapPlanner = planner;

    if (startOnAdd) _updateGOAPPlan = true;
    return true;
}