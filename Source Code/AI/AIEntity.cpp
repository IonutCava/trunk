#include "Headers/AIEntity.h"
#include "ActionInterface/Headers/ActionList.h"

#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "PathFinding/Waypoints/Headers/WaypointGraph.h"  ///< For waypoint movement
#include "PathFinding/NavMeshes/Headers/NavMesh.h" ///< For NavMesh movement
#include "Dynamics/Entities/Units/Headers/NPC.h"
#include "AI/PathFinding/Headers/DivideCrowd.h"
#include "AI/PathFinding/Headers/DivideRecast.h"
#include "AI/PathFinding/NavMeshes/Headers/NavMesh.h"

static const D32 DESTINATION_RADIUS = 1.5 * 1.5;

AIEntity::AIEntity(const vec3<F32>& currentPosition, const std::string& name)  : GUIDWrapper(),
                                              _name(name),
                                              _actionProcessor(NULL),
                                              _unitRef(NULL),
                                              _coordination(NULL),
                                              _comInterface(NULL),
                                              _detourCrowd(NULL),
                                              _agent(NULL),
                                              _agentID(-1),
                                              _distanceToTarget(-1.f),
                                              _previousDistanceToTarget(-1.f),
                                              _moveWaitTimer(0ULL),
                                              _stopped(false)
{
    _currentPosition.set(currentPosition);
}

AIEntity::~AIEntity()
{
    if(_detourCrowd)
        _detourCrowd->removeAgent(getAgentID());

    _agentID = -1;
    _agent = NULL;

    SAFE_DELETE(_comInterface);
    SAFE_DELETE(_actionProcessor);
    for_each(sensorMap::value_type& it , _sensorList){
        SAFE_DELETE(it.second);
    }
    _sensorList.clear();
}

void AIEntity::load(const vec3<F32>& position) {
    setPosition(position);

    if(!isAgentLoaded() && _detourCrowd) {
        _agentID = _detourCrowd->addAgent(position, (_detourCrowd->getAgentHeight()/2)*3.5f, 10.0f);
        _agent = _detourCrowd->getAgent(_agentID);
        _destination = position;
    }
    
}

void AIEntity::unload() {
    if(!isAgentLoaded())
        return;

    _detourCrowd->removeAgent(getAgentID());
    _agentID = -1;
    _agent = NULL;
}

void AIEntity::sendMessage(AIEntity* receiver, AIMsg msg,const boost::any& msg_content){
    CommunicationInterface* com = getCommunicationInterface();
    if(com){
        com->sendMessageToEntity(receiver, msg,msg_content);
    }
}

void AIEntity::receiveMessage(AIEntity* sender, AIMsg msg, const boost::any& msg_content){
    CommunicationInterface* com = getCommunicationInterface();
    if(com){
        com->receiveMessageFromEntity(sender, msg,msg_content);
    }
}

void AIEntity::processMessage(AIEntity* sender, AIMsg msg, const boost::any& msg_content) {
    if(!_actionProcessor)
        return;

    ReadLock r_lock(_updateMutex);
    _actionProcessor->processMessage(sender, msg, msg_content);
}

Sensor* AIEntity::getSensor(SensorType type){
    if(_sensorList.find(type) != _sensorList.end()){
        return _sensorList[type];
    }
    return NULL;
}

bool AIEntity::addSensor(SensorType type, Sensor* sensor){
    
    sensor->updatePosition(_currentPosition);
    if(_sensorList.find(type) != _sensorList.end()){
        SAFE_UPDATE(_sensorList[type], sensor);
    }else{
        _sensorList.insert(std::make_pair(type,sensor));
    }
    return true;
}

bool AIEntity::addActionProcessor(ActionList* actionProcessor){
    WriteLock w_lock(_updateMutex);
    SAFE_UPDATE(_actionProcessor, actionProcessor);
    _actionProcessor->addEntityRef(this);
    return true;
}

void AIEntity::processInput(){
    ReadLock r_lock(_managerQueryMutex);
    if(!_actionProcessor) return;
    _actionProcessor->processInput();
}

void AIEntity::processData(){
    ReadLock r_lock(_managerQueryMutex);
    if(!_actionProcessor) return;
    _actionProcessor->processData();
}

void AIEntity::update(const U64 deltaTime){
    ReadLock r_lock(_managerQueryMutex);
    if(!_actionProcessor) return;

    _actionProcessor->update(_unitRef);

    if(_unitRef)
        _unitRef->update(deltaTime);

    updatePosition(deltaTime);
}

void AIEntity::setTeam(AICoordination* const coordination) {
    ReadLock r_lock(_updateMutex);
    if(_coordination){
        //Remove from old team
        _coordination->removeTeamMember(this);
    }
    //Update our team
    _coordination = coordination;
    //Add ourself to the new team
    _coordination->addTeamMember(this);

    resetCrowd(_coordination->getCrowdPtr());
}

void AIEntity::addUnitRef(NPC* const npc) {
    _unitRef = npc;
    if(_unitRef){
        load(_unitRef->getPosition());
    }
}

bool AIEntity::addFriend(AIEntity* const friendEntity){
    ReadLock r_lock(_updateMutex);
    AICoordination* friendTeam = friendEntity->getTeam();
    //If no team, check if our friend has a team and add ourself to it
    if(!_coordination){
        //If our friend has a team ...
        if(friendTeam){
            ///Create friendship
            friendTeam->addTeamMember(this);
            _coordination = friendTeam;
            return true;
        }
        return false;
    }
    //If we have team, add friend to our team
    _coordination->addTeamMember(friendEntity);
    //If our friend isn't on our team, add him
    if(!friendTeam){
        friendEntity->setTeam(_coordination);
    }
    return true;
}

D32 AIEntity::getAgentHeight() const {
    return _detourCrowd ? _detourCrowd->getAgentHeight() : 0.0;
}

D32 AIEntity::getAgentRadius() const {
    return _detourCrowd ? _detourCrowd->getAgentRadius() : 0.0;
}

void AIEntity::resetCrowd(Navigation::DivideDtCrowd* const crowd){
    if(_detourCrowd)
        unload();

    _detourCrowd = crowd;

    if(_detourCrowd){
        _destination = _detourCrowd->getLastDestination();
        load(_unitRef != NULL ? _unitRef->getPosition() : vec3<F32>());
    }
}

void AIEntity::setPosition(const vec3<F32> position) {
    if(!isAgentLoaded()) {
        if(_unitRef)
            _unitRef->setPosition(position);
        return;
    }

    vec3<F32> result;

    if(!_detourCrowd->isValidNavMesh())
        return;

    // Find position on navmesh
    if (!Navigation::DivideRecast::getInstance().findNearestPointOnNavmesh(_detourCrowd->getNavMesh(), position, result))
        return;

    // Remove agent from crowd and re-add at position
    _detourCrowd->removeAgent(_agentID);
    _agentID = _detourCrowd->addAgent(result, (_detourCrowd->getAgentHeight()/2)*3.5f, 10.0f);
    _agent = _detourCrowd->getAgent(_agentID);

    if(_unitRef)
        _unitRef->setPosition(position);
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
                updateDestination(Navigation::DivideRecast::getInstance().getRandomNavMeshPoint(_detourCrowd->getNavMesh()));
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

vec3<F32> AIEntity::getPosition() const {
    return _currentPosition;
}

vec3<F32> AIEntity::getDestination() const {
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

void AIEntity::moveForward(){
    vec3<F32> lookDirection = _unitRef != NULL ? _unitRef->getLookingDirection() : WORLD_Z_NEG_AXIS;
    lookDirection.normalize();

    setVelocity(lookDirection * getMaxSpeed());
}

void AIEntity::setVelocity(const vec3<F32>& velocity){
    _stopped = false;
    _destination.reset();

    if(isAgentLoaded())
        _detourCrowd->requestVelocity(getAgentID(), velocity);
}

void AIEntity::stop(){
    if( !isAgentLoaded()) {
        _stopped = true;
        return;
    }

    if(_detourCrowd->stopAgent(getAgentID())) {
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