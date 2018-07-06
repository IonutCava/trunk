#include "Headers/Trigger.h"
#include "Utility/Headers/Event.h"
#include "Managers/Headers/CameraManager.h"
#include "Dynamics/Entities/Headers/Impostor.h"
#include "Dynamics/Entities/Units/Headers/Unit.h"

Trigger::Trigger() : SceneNode(TYPE_TRIGGER), _drawImpostor(false), _triggerImpostor(NULL),
											  _enabled(true),		_impostorSGN(NULL)
{
}

Trigger::~Trigger()
{

}

void Trigger::setParams( Event_ptr triggeredEvent, const vec3<F32>& triggerPosition, F32 radius){
	/// Check if position has changed
   if(!_triggerPosition.compare(triggerPosition)){
	   _triggerPosition = triggerPosition;
	   if(_triggerImpostor){
			/// update dummy position if it is so
			_impostorSGN->getTransform()->setPosition(_triggerPosition);
	   }
   }
   /// Check if radius has changed
   if(!FLOAT_COMPARE(_radius,radius)){
	   	_radius = radius;
		 if(_triggerImpostor){
			/// update dummy radius if so
			_triggerImpostor->setRadius(radius);
		  }
   }
   /// swap event anyway
   	_triggeredEvent.swap(triggeredEvent);
}

bool Trigger::load(const std::string& name){
	setName(name);
	return true;
}

bool Trigger::unload(){
	if(_triggerImpostor){
		_triggerSGN->removeNode(_impostorSGN);
	}
	SAFE_DELETE(_triggerImpostor);
	return SceneNode::unload();
}

void Trigger::postLoad(SceneGraphNode* const sgn) {
	//Hold a pointer to the trigger's location in the SceneGraph
	_triggerSGN = sgn;
}	

void Trigger::render(SceneGraphNode* const sgn){
	///The isInView call should stop impostor rendering if needed
	if(!_triggerImpostor){
		_triggerImpostor = New Impostor(_name,_radius);
		_impostorSGN = _triggerSGN->addNode(_triggerImpostor->getDummy()); 
	}
	_triggerImpostor->render(_impostorSGN);
}

bool Trigger::check(Unit* const unit){
	if(!_enabled) return false;

	vec3<F32> position;
	if(!unit){ ///< use camera position
		position = CameraManager::getInstance().getActiveCamera()->getEye();
	}else{ ///< use unit position
		position = unit->getCurrentPosition();
	}
	/// Should we trigger the event?
	if(position.compare(_triggerPosition, _radius)){
		/// Check if the event is valid, and trigger if it is
		return trigger();
	}
	/// Not triggered
	return false;
}

bool Trigger::trigger(){
	if(!_triggeredEvent){
		return false;
	}
	_triggeredEvent.get()->startEvent();
	return true;
}
