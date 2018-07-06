#include "Headers/Trigger.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Math/Headers/Transform.h"
#include "Hardware/Platform/Headers/Task.h"
#include "Dynamics/Entities/Headers/Impostor.h"
#include "Dynamics/Entities/Units/Headers/Unit.h"

namespace Divide {

Trigger::Trigger() : SceneNode(TYPE_TRIGGER),
                     _drawImpostor(false),
                     _triggerImpostor(nullptr),
                     _enabled(true)
{
}

Trigger::~Trigger()
{
}

void Trigger::setParams( Task_ptr triggeredTask, const vec3<F32>& triggerPosition, F32 radius){
    /// Check if position has changed
   if(!_triggerPosition.compare(triggerPosition)){
       _triggerPosition = triggerPosition;
   }
   /// Check if radius has changed
   if(!FLOAT_COMPARE(_radius,radius)){
        _radius = radius;
         if(_triggerImpostor){
            /// update dummy radius if so
            _triggerImpostor->setRadius(radius);
          }
   }
   /// swap Task anyway
    _triggeredTask.swap(triggeredTask);
}

bool Trigger::unload(){
    return SceneNode::unload();
}

void Trigger::render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState, const RenderStage& currentRenderStage){
    ///The isInView call should stop impostor rendering if needed
    if(!_triggerImpostor){
        ResourceDescriptor impostorDesc( _name + "_impostor" );
        _triggerImpostor = CreateResource<Impostor>( impostorDesc );
        sgn->addNode( _triggerImpostor );
    }
    /// update dummy position if it is so
    sgn->getChildren()[0]->getComponent<PhysicsComponent>()->setPosition( _triggerPosition );
    _triggerImpostor->setRadius( _radius );
    _triggerImpostor->getSceneNodeRenderState().setDrawState(true);
    sgn->addNode( _triggerImpostor )->setActive( true );
}

bool Trigger::check(Unit* const unit,const vec3<F32>& camEyePos){
    if(!_enabled) return false;

    vec3<F32> position;
    if(!unit){ ///< use camera position
        position = camEyePos;
    }else{ ///< use unit position
        position = unit->getCurrentPosition();
    }
    /// Should we trigger the Task?
    if(position.compare(_triggerPosition, _radius)){
        /// Check if the Task is valid, and trigger if it is
        return trigger();
    }
    /// Not triggered
    return false;
}

bool Trigger::trigger(){
    if(!_triggeredTask){
        return false;
    }
    _triggeredTask.get()->startTask();
    return true;
}

};