#include "Headers/AIEntity.h"

#include "AI/PathFinding/Headers/DivideCrowd.h"
#include "AI/PathFinding/NavMeshes/Headers/NavMesh.h"
#include "AI/ActionInterface/Headers/AITeam.h"
#include "AI/ActionInterface/Headers/AISceneImpl.h"

#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "PathFinding/Waypoints/Headers/WaypointGraph.h"  ///< For waypoint movement
#include "PathFinding/NavMeshes/Headers/NavMesh.h"  ///< For NavMesh movement
#include "Dynamics/Entities/Units/Headers/NPC.h"
#include "Managers/Headers/AIManager.h"

namespace Divide {
using namespace AI;

static const D32 DESTINATION_RADIUS = 3 * 3;

AIEntity::AIEntity(const vec3<F32>& currentPosition, const stringImpl& name)
    : GUIDWrapper(),
      _name(name),
      _AISceneImpl(nullptr),
      _unitRef(nullptr),
      _teamPtr(nullptr),
      _detourCrowd(nullptr),
      _agent(nullptr),
      _agentID(-1),
      _distanceToTarget(-1.f),
      _previousDistanceToTarget(-1.f),
      _moveWaitTimer(0ULL),
      _stopped(false) {
    _currentPosition.set(currentPosition);
    _agentRadiusCategory = PresetAgentRadius::AGENT_RADIUS_SMALL;
}

AIEntity::~AIEntity() {
    if (_detourCrowd) {
        _detourCrowd->removeAgent(getAgentID());
    }

    _agentID = -1;
    _agent = nullptr;

    MemoryManager::DELETE(_AISceneImpl);
    MemoryManager::DELETE_HASHMAP(_sensorList);
}

void AIEntity::load(const vec3<F32>& position) {
    setPosition(position);

    if (!isAgentLoaded() && _detourCrowd) {
        _agentID = _detourCrowd->addAgent(
            position, _unitRef ? _unitRef->getMovementSpeed()
                               : (_detourCrowd->getAgentHeight() / 2) * 3.5f,
            10.0f);
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

void AIEntity::sendMessage(AIEntity* receiver, AIMsg msg,
                           const cdiggins::any& msg_content) {
    assert(receiver != nullptr);
    if (getGUID() != receiver->getGUID()) {
        receiver->receiveMessage(this, msg, msg_content);
    }
}

void AIEntity::receiveMessage(AIEntity* sender, AIMsg msg,
                              const cdiggins::any& msg_content) {
    this->processMessage(sender, msg, msg_content);
}

void AIEntity::processMessage(AIEntity* sender, AIMsg msg,
                              const cdiggins::any& msg_content) {
    assert(_AISceneImpl);

    ReadLock r_lock(_updateMutex);
    _AISceneImpl->processMessage(sender, msg, msg_content);
}

Sensor* AIEntity::getSensor(SensorType type) {
    ReadLock r_lock(_updateMutex);
    if (_sensorList.find(type) != std::end(_sensorList)) {
        return _sensorList[type];
    }
    return nullptr;
}

bool AIEntity::addSensor(SensorType type) {
    WriteLock w_lock(_updateMutex);
    Sensor* sensor = nullptr;
    switch (type) {
        case SensorType::AUDIO_SENSOR: {
            sensor = Attorney::AudioSensorConstructor::construct(this);
        } break;
        case SensorType::VISUAL_SENSOR: {
            sensor = Attorney::VisualSensorConstructor::construct(this);
        } break;
    };

    if (sensor) {
        SensorMap::iterator it = _sensorList.find(type);
        if (it != std::end(_sensorList)) {
            MemoryManager::SAFE_UPDATE(it->second, sensor);
        } else {
            hashAlg::emplace(_sensorList, type, sensor);
        }
        return true;
    }

    return false;
}

bool AIEntity::addAISceneImpl(AISceneImpl* AISceneImpl) {
    assert(AISceneImpl);

    WriteLock w_lock(_updateMutex);
    MemoryManager::SAFE_UPDATE(_AISceneImpl, AISceneImpl);
    _AISceneImpl->addEntityRef(this);
    return true;
}

void AIEntity::processInput(const U64 deltaTime) {
    if (_AISceneImpl) {
        _AISceneImpl->init();
        _AISceneImpl->processInput(deltaTime);
    }
}

void AIEntity::processData(const U64 deltaTime) {
    if (_AISceneImpl) {
        _AISceneImpl->processData(deltaTime);
    }
}

void AIEntity::update(const U64 deltaTime) {
    if (_AISceneImpl) {
        _AISceneImpl->update(deltaTime, _unitRef);
    }
    if (_unitRef) {
        _unitRef->update(deltaTime);
        if (!_detourCrowd) {
            resetCrowd();
        }
    }
    updatePosition(deltaTime);
}

I32 AIEntity::getTeamID() const {
    if (_teamPtr != nullptr) {
        return _teamPtr->getTeamID();
    }
    return -1;
}

void AIEntity::setTeamPtr(AITeam* const teamPtr) {
    if (_teamPtr != nullptr) {
        _teamPtr->removeTeamMember(this);
    }
    _teamPtr = teamPtr;
}

void AIEntity::addUnitRef(NPC* const npc) {
    _unitRef = npc;
    if (_unitRef) {
        load(_unitRef->getPosition());
    }
}

D32 AIEntity::getAgentHeight() const {
    return _detourCrowd ? _detourCrowd->getAgentHeight() : 0.0;
}

D32 AIEntity::getAgentRadius() const {
    return _detourCrowd ? _detourCrowd->getAgentRadius() : 0.0;
}

void AIEntity::resetCrowd() {
    if (_detourCrowd) {
        unload();
    }

    _detourCrowd = _teamPtr->getCrowd(getAgentRadiusCategory());

    if (_detourCrowd) {
        _destination = _detourCrowd->getLastDestination();
        load(_unitRef != nullptr ? _unitRef->getPosition() : vec3<F32>());
    }
}

bool AIEntity::setPosition(const vec3<F32> position) {
    if (!isAgentLoaded()) {
        if (_unitRef) {
            _unitRef->setPosition(position);
        }
        return false;
    }
    if (!_detourCrowd || !_detourCrowd->isValidNavMesh()) {
        return false;
    }

    vec3<F32> result;
    // Find position on NavMesh
    if (!_detourCrowd->getNavMesh().getClosestPosition(
            position, vec3<F32>(5), DESTINATION_RADIUS, result)) {
        // return false;
    }
    // Remove agent from crowd and re-add at position
    _detourCrowd->removeAgent(_agentID);
    _agentID = _detourCrowd->addAgent(
        result, (_detourCrowd->getAgentHeight() / 2) * 3.5f, 10.0f);
    _agent = _detourCrowd->getAgent(_agentID);

    if (_unitRef) {
        _unitRef->setPosition(position);
    }
    return true;
}

void AIEntity::updatePosition(const U64 deltaTime) {
    if (isAgentLoaded() && getAgent()->active) {
        _previousDistanceToTarget = _distanceToTarget;
        _distanceToTarget = _currentPosition.distanceSquared(_destination);

        // if we are walking but did not change distance
        if (std::abs(_previousDistanceToTarget - _distanceToTarget) <
            DESTINATION_RADIUS) {
            _moveWaitTimer += deltaTime;
            if (Time::MicrosecondsToSeconds<D32>(_moveWaitTimer) > 5) {
                _moveWaitTimer = 0;
                // updateDestination(_detourCrowd->getNavMesh().getRandomPosition());
                // return;
            }
        } else {
            _moveWaitTimer = 0;
        }
        _currentPosition.setV(getAgent()->npos);
        _currentVelocity.setV(getAgent()->nvel);
    }

    if (_unitRef) {
        _unitRef->setPosition(_currentPosition);
        _unitRef->setVelocity(_currentVelocity);
    }
}

bool AIEntity::updateDestination(const vec3<F32>& destination,
                                 bool updatePreviousPath) {
    if (!isAgentLoaded()) {
        return false;
    }
    vec3<F32> result;
    // Find position on navmesh
    if (!_detourCrowd->getNavMesh().getRandomPositionInCircle(
            destination, DESTINATION_RADIUS, vec3<F32>(5), result, 10)) {
        if (!_detourCrowd->getNavMesh().getClosestPosition(
                destination, vec3<F32>(5), DESTINATION_RADIUS, result)) {
            return false;
        }
    }
    _detourCrowd->setMoveTarget(_agentID, result, updatePreviousPath);
    _destination = result;
    _stopped = false;

    return true;
}

const vec3<F32>& AIEntity::getPosition() const { return _currentPosition; }

const vec3<F32>& AIEntity::getDestination() const {
    if (isAgentLoaded()) return _destination;

    return getPosition();  // TODO this is not ideal
}

bool AIEntity::destinationReached() {
    if (!isAgentLoaded()) return false;

    if (_currentPosition.distanceSquared(getDestination()) <=
        DESTINATION_RADIUS)
        return true;

    return _detourCrowd->destinationReached(getAgent(), DESTINATION_RADIUS);
}

void AIEntity::setDestination(const vec3<F32>& destination) {
    if (!isAgentLoaded()) return;

    _destination = destination;
    _stopped = false;
}

void AIEntity::moveForward() {
    vec3<F32> lookDirection = _unitRef != nullptr
                                  ? _unitRef->getLookingDirection()
                                  : WORLD_Z_NEG_AXIS;
    lookDirection.normalize();

    setVelocity(lookDirection * getMaxSpeed());
}

void AIEntity::moveBackwards() {
    vec3<F32> lookDirection = _unitRef != nullptr
                                  ? _unitRef->getLookingDirection()
                                  : WORLD_Z_NEG_AXIS;
    lookDirection.normalize();

    setVelocity(lookDirection * getMaxSpeed() * -1.0f);
}

void AIEntity::setVelocity(const vec3<F32>& velocity) {
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

D32 AIEntity::getMaxSpeed() {
    return (isAgentLoaded() ? getAgent()->params.maxSpeed : 0.0);
}

D32 AIEntity::getMaxAcceleration() {
    return (isAgentLoaded() ? getAgent()->params.maxAcceleration : 0.0);
}

};  // namespace Divide