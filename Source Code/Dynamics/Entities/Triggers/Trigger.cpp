#include "Headers/Trigger.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Core/Math/Headers/Transform.h"
#include "Platform/Threading/Headers/Task.h"
#include "Dynamics/Entities/Headers/Impostor.h"
#include "Dynamics/Entities/Units/Headers/Unit.h"

namespace Divide {

Trigger::Trigger(const stringImpl& name)
    : SceneNode(name, SceneNodeType::TYPE_TRIGGER),
      _drawImpostor(false),
      _triggerImpostor(nullptr),
      _enabled(true),
      _radius(1.0f)
{
}

Trigger::~Trigger()
{
}

bool Trigger::onRender(RenderStage currentStage) {

    return true;
}

void Trigger::sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn,
                          SceneState& sceneState) {
    if (_drawImpostor) {
        if (!_triggerImpostor) {
            static const U32 normalMask = to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                to_const_uint(SGNComponent::ComponentType::RENDERING);
            ResourceDescriptor impostorDesc(_name + "_impostor");
            _triggerImpostor = CreateResource<ImpostorSphere>(impostorDesc);
            sgn.addNode(_triggerImpostor, normalMask, PhysicsGroup::GROUP_IGNORE);
        }
        /// update dummy position if it is so
        U32 temp = 0;
        sgn.getChild(0, temp).get<PhysicsComponent>()->setPosition(_triggerPosition);
        _triggerImpostor->setRadius(_radius);
        _triggerImpostor->renderState().setDrawState(true);
        sgn.getChild(0, temp).setActive(true);
    } else {
        if (_triggerImpostor) {
            _triggerImpostor->renderState().setDrawState(false);
        }
    }
}

void Trigger::setParams(Task_ptr triggeredTask,
                        const vec3<F32>& triggerPosition,
                        F32 radius) {
    /// Check if position has changed
    if (!_triggerPosition.compare(triggerPosition)) {
        _triggerPosition = triggerPosition;
    }
    /// Check if radius has changed
    if (!COMPARE(_radius, radius)) {
        _radius = radius;
        if (_triggerImpostor) {
            /// update dummy radius if so
            _triggerImpostor->setRadius(radius);
        }
    }
    /// swap Task anyway
    _triggeredTask.swap(triggeredTask);
}

bool Trigger::unload() {
    return SceneNode::unload();
}

bool Trigger::check(Unit* const unit, const vec3<F32>& camEyePos) {
    if (!_enabled)
        return false;

    vec3<F32> position;
    if (!unit) {  ///< use camera position
        position = camEyePos;
    } else {  ///< use unit position
        position = unit->getCurrentPosition();
    }
    /// Should we trigger the Task?
    if (position.compare(_triggerPosition, _radius)) {
        /// Check if the Task is valid, and trigger if it is
        return trigger();
    }
    /// Not triggered
    return false;
}

bool Trigger::trigger() {
    if (!_triggeredTask) {
        return false;
    }
    _triggeredTask.get()->startTask(Task::TaskPriority::HIGH);
    return true;
}
};