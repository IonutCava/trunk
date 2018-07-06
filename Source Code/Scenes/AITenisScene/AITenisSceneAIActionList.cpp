#include "Headers/AITenisSceneAIActionList.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "AI/Sensors/Headers/VisualSensor.h"

AITenisSceneAIActionList::AITenisSceneAIActionList(SceneGraphNode* target) : ActionList(),
																			 _node(NULL),
																			 _target(target),
																			 _atacaMingea(false),
																			 _mingeSpreEchipa2(true),
																			 _stopJoc(true),
																			 _tickCount(0)
{
}

void AITenisSceneAIActionList::addEntityRef(AIEntity* entity){
	if(!entity) return;
	_entity = entity;
	VisualSensor* visualSensor = dynamic_cast<VisualSensor*>(_entity->getSensor(VISUAL_SENSOR));
	if(visualSensor){
		_pozitieInitiala = visualSensor->getSpatialPosition();
	}
}

//Process message from sender to receiver
void AITenisSceneAIActionList::processMessage(AIEntity* sender, AI_MSG msg, const boost::any& msg_content){
	switch(msg){
		case REQUEST_DISTANCE_TO_TARGET:
				updatePositions();
				_entity->sendMessage(sender, RECEIVE_DISTANCE_TO_TARGET, _pozitieEntitate.distance(_pozitieMinge));
			break;
		case RECEIVE_DISTANCE_TO_TARGET:
			_membruDistanta[sender] = boost::any_cast<F32>(msg_content);
			break;
		case LOVESTE_MINGEA:
			for_each(AICoordination::teamMap::value_type& member, _entity->getTeam()){
				if(_entity->getGUID() != member.second->getGUID()){
					_entity->sendMessage(member.second, NU_LOVI_MINGEA, 0);
				}
			}
			if(_mingeSpreEchipa2){
				if(_entity->getTeamID() == 2){
					_atacaMingea = true;
				}else{
					_atacaMingea = false;
				}
			}else{
				if(_entity->getTeamID() == 1){
					_atacaMingea = true;
				}else{
					_atacaMingea = false;
				}
			}
				break;
		case NU_LOVI_MINGEA: 
			_atacaMingea = false;
			break;

	};
}

void AITenisSceneAIActionList::updatePositions(){
	VisualSensor* visualSensor = dynamic_cast<VisualSensor*>(_entity->getSensor(VISUAL_SENSOR));
	if(visualSensor){
		_tickCount++;
		if(_tickCount == 512){
			_pozitieMingeAnterioara = _pozitieMinge;
			_tickCount = 0;
		}
		_pozitieMinge = visualSensor->getPositionOfObject(_target);
		_pozitieEntitate = visualSensor->getSpatialPosition();
		if(_pozitieMingeAnterioara.z != _pozitieMinge.z){
			_pozitieMingeAnterioara.z < _pozitieMinge.z ? _mingeSpreEchipa2 = false : _mingeSpreEchipa2 = true;
			_stopJoc = false;
		}else{
			_stopJoc = true;
		}
	}
}

//Colectam toate datele necesare
//Aflam pozitia noastra si pozitia mingii 
void AITenisSceneAIActionList::processInput(){
	updatePositions();
	AICoordination::teamMap& team = _entity->getTeam();
	_membruDistanta.clear();
	for_each(AICoordination::teamMap::value_type& member, team){
		//Cerem tuturor coechipierilor sa ne transmita pozitia lor actuala
		//Pentru fiecare membru din echipa, ii trimitem un request sa ne returneze pozitia
		if(_entity->getGUID() != member.second->getGUID()){
			_entity->sendMessage(member.second, REQUEST_DISTANCE_TO_TARGET, 0);
		}
	}
}

void AITenisSceneAIActionList::processData(){
	AIEntity* celMaiApropiat = _entity;
	F32 distanta = _pozitieEntitate.distance(_pozitieMinge);
	for_each(membruDistantaMap::value_type& pereche, _membruDistanta){
		if(pereche.second < distanta){
			distanta = pereche.second;
			celMaiApropiat = pereche.first;
		}
	}

	_entity->sendMessage(celMaiApropiat, LOVESTE_MINGEA, distanta);
}

void AITenisSceneAIActionList::update(SceneGraphNode* node){
	if(!_node){
		_node = node;
	}
	
	Sensor* visualSensor = _entity->getSensor(VISUAL_SENSOR);
	updatePositions();

	if(_atacaMingea && !_stopJoc){
		node->getTransform()->translateX((_pozitieMinge.x - _pozitieEntitate.x)/100.0f);
		//node->getTransform()->translateZ((_pozitieMinge.z - _pozitieEntitate.z)/5250.0f);
	}else{
		node->getTransform()->translateX((_pozitieInitiala.x - _pozitieEntitate.x)/100.0f);
		//node->getTransform()->translateZ((_pozitieInitiala.z - _pozitieEntitate.z)/1200.0f);
	}

	if(visualSensor){
		visualSensor->updatePosition(node->getTransform()->getPosition());
	}
}