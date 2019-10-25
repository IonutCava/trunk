#include "stdafx.h"

#include "Headers/Trigger.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Threading/Headers/Task.h"
#include "Dynamics/Entities/Headers/Impostor.h"
#include "Dynamics/Entities/Units/Headers/Unit.h"
#include "ECS/Components/Headers/TransformComponent.h"

namespace Divide {

Trigger::Trigger(ResourceCache& parentCache, size_t descriptorHash, const Str64& name)
    : SceneNode(parentCache, descriptorHash, name, SceneNodeType::TYPE_TRIGGER),
      _drawImpostor(false),
      _triggerImpostor(nullptr),
      _enabled(true),
      _radius(1.0f)
{
}

Trigger::~Trigger()
{
}


void Trigger::sceneUpdate(const U64 deltaTimeUS, SceneGraphNode& sgn,
                          SceneState& sceneState) {
    if (_drawImpostor) {
        if (!_triggerImpostor) {
            ResourceDescriptor impostorDesc(resourceName() + "_impostor");
            _triggerImpostor = CreateResource<ImpostorSphere>(_parentCache, impostorDesc);

            SceneGraphNodeDescriptor triggerImpostorDescriptor;
            triggerImpostorDescriptor._serialize = false;
            triggerImpostorDescriptor._node = _triggerImpostor;
            triggerImpostorDescriptor._componentMask = to_base(ComponentType::TRANSFORM) |
                                                       to_base(ComponentType::BOUNDS) |
                                                       to_base(ComponentType::RENDERING) |
                                                       to_base(ComponentType::NETWORKING);
            triggerImpostorDescriptor._usageContext = NodeUsageContext::NODE_DYNAMIC;

            sgn.addChildNode(triggerImpostorDescriptor);
        }
        /// update dummy position if it is so
        sgn.getChild(0).get<TransformComponent>()->setPosition(_triggerPosition);
        _triggerImpostor->setRadius(_radius);
        _triggerImpostor->renderState().setDrawState(true);
        sgn.getChild(0).setFlag(SceneGraphNode::Flags::ACTIVE);
    } else {
        if (_triggerImpostor) {
            _triggerImpostor->renderState().setDrawState(false);
        }
    }
}

void Trigger::setParams(Task& triggeredTask,
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
    _triggeredTask = &triggeredTask;
}

bool Trigger::unload() noexcept {
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
    assert(_triggeredTask != nullptr);
    Start(*_triggeredTask);
    return true;
}
};