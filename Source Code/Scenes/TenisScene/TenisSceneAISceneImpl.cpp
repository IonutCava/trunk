#include "Headers/TenisSceneAISceneImpl.h"

#include "AI/Sensors/Headers/VisualSensor.h"
#include "AI/ActionInterface/Headers/AITeam.h"

#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

namespace Divide {

using namespace AI;

TenisSceneAISceneImpl::TenisSceneAISceneImpl(SceneGraphNode* target) : AISceneImpl(),
                                                                       _target(target),
                                                                       _attackBall(false),
                                                                       _ballToTeam2(true),
                                                                       _gameStop(true),
                                                                       _tickCount(0)
{
}

void TenisSceneAISceneImpl::addEntityRef(AIEntity* entity){
    assert(entity != nullptr);
    _entity = entity;
    if (_entity->getUnitRef()) {
        _initialPosition = _entity->getUnitRef()->getCurrentPosition();
    }
}

//Process message from sender to receiver
void TenisSceneAISceneImpl::processMessage(AIEntity* sender, AIMsg msg, const cdiggins::any& msg_content){
    AITeam* currentTeam = nullptr;
    switch(msg){
        case REQUEST_DISTANCE_TO_TARGET:
                updatePositions();
                _entity->sendMessage(sender, RECEIVE_DISTANCE_TO_TARGET, distanceToBall(_initialPosition,_ballPosition));
            break;
        case RECEIVE_DISTANCE_TO_TARGET: {
            assert(_entity->getTeam());
            bool success = false;
            _entity->getTeam()->getMemberVariable()[sender] = msg_content.constant_cast<F32>(success);
            assert(success);
            }break;
        case ATTACK_BALL:
            currentTeam = _entity->getTeam();
            assert(currentTeam);
            for (const AITeam::TeamMap::value_type& member : currentTeam->getTeamMembers()) {
                if ( _entity->getGUID() != member.second->getGUID() ) {
                    _entity->sendMessage( member.second, DONT_ATTACK_BALL, 0 );
                }
            }
            if(_ballToTeam2){
                if(_entity->getTeamID() == 2){
                    _attackBall = true;
                }else{
                    _attackBall = false;
                }
            }else{
                if(_entity->getTeamID() == 1){
                    _attackBall = true;
                }else{
                    _attackBall = false;
                }
            }
            break;
        case DONT_ATTACK_BALL:
            _attackBall = false;
            break;
    };
}

void TenisSceneAISceneImpl::updatePositions(){
    _tickCount++;
    if (_tickCount == 96) {
        _prevBallPosition = _ballPosition;
        _tickCount = 0;
    }
    _ballPosition = _target->getComponent<PhysicsComponent>()->getPosition();
    _entityPosition = _entity->getUnitRef()->getCurrentPosition();
    if (_prevBallPosition.z != _ballPosition.z) {
        _prevBallPosition.z < _ballPosition.z ? _ballToTeam2 = false : _ballToTeam2 = true;
        _gameStop = false;
    } else {
        _gameStop = true;
    }
}

///Collect all of the necessary information for this current update step
void TenisSceneAISceneImpl::processInput(const U64 deltaTime){
    updatePositions();
    AITeam* currentTeam = _entity->getTeam();
    assert(currentTeam != nullptr);
    _entity->getTeam()->getMemberVariable()[_entity] = distanceToBall(_initialPosition,_ballPosition);
    for (const AITeam::TeamMap::value_type& member : currentTeam->getTeamMembers()) {
        ///Ask all of our team-mates to send us their distance to the ball
        if ( _entity->getGUID() != member.second->getGUID() ) {
            _entity->sendMessage( member.second, REQUEST_DISTANCE_TO_TARGET, 0 );
        }
    }
}

void TenisSceneAISceneImpl::processData(const U64 deltaTime){
    AIEntity* nearestEntity = _entity;
    F32 distance = _entity->getTeam()->getMemberVariable()[_entity];
    typedef hashMapImpl<AIEntity*, F32 > memberVariable;
    for (memberVariable::value_type& member : _entity->getTeam()->getMemberVariable()) {
        if ( member.second < distance && member.first->getGUID() != _entity->getGUID() ) {
            distance = member.second;
            nearestEntity = member.first;
        }
    }
    _entity->sendMessage(nearestEntity, ATTACK_BALL, distance);
}

void TenisSceneAISceneImpl::update(const U64 deltaTime, NPC* unitRef){
    if(!unitRef)
        return;

    updatePositions();

    if(_attackBall && !_gameStop){
        /// Try to intercept the ball
        unitRef->moveToX(_ballPosition.x);
    }else{
        /// Try to return to original position
        unitRef->moveToX(_initialPosition.x);
    }

    /// Update sensor information
    Sensor* visualSensor = _entity->getSensor(VISUAL_SENSOR);

    if(visualSensor){
        visualSensor->update(deltaTime);
    }
}

F32 TenisSceneAISceneImpl::distanceToBall(const vec3<F32>& entityPosition, const vec3<F32> ballPosition) {
    return entityPosition.distance(ballPosition);//std::abs(ballPosition.x - entityPosition.x);
}

};