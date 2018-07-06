#include "stdafx.h"

#include "ECS/Events/Headers/TransformEvents.h"
#include "ECS/Components/Headers/TransformComponent.h"

#include "Headers/TransformSystem.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {

    TransformSystem::TransformSystem(ECS::ECSEngine& parentEngine)
        : ECSSystem(parentEngine)
    {

    }

    TransformSystem::~TransformSystem()
    {

    }

    void TransformSystem::PreUpdate(F32 dt) {
        // Go over all transforms and make sure we update parent states
        auto transform = _engine.GetComponentManager()->begin<TransformComponent>();
        auto transformEnd = _engine.GetComponentManager()->end<TransformComponent>();
        for (;transform != transformEnd; ++transform)
        {
           
        }
    }

    void TransformSystem::Update(F32 dt) {
        // 
    }

    void TransformSystem::PostUpdate(F32 dt) {
        // If the transform has been modified, notify listeners, 
        auto transform = _engine.GetComponentManager()->begin<TransformComponent>();
        auto transformEnd = _engine.GetComponentManager()->end<TransformComponent>();
        for (;transform != transformEnd; ++transform)
        {
            transform->snapshot();
        }
    }

    bool TransformSystem::save(const SceneGraphNode& sgn, ByteBuffer& outputBuffer) {
        TransformComponent* tComp = sgn.GetComponent<TransformComponent>();
        if (tComp != nullptr) {
            vec3<F32> localPos;
            tComp->getPosition(localPos);
            outputBuffer << localPos;

            vec3<F32> localScale;
            tComp->getScale(localScale);
            outputBuffer << localScale;

            Quaternion<F32> localRotation;
            tComp->getOrientation(localRotation);
            outputBuffer << localRotation.asVec4();
        }
        return ECSSystem<TransformSystem>::save(sgn, outputBuffer);
    }

    bool TransformSystem::load(SceneGraphNode& sgn, ByteBuffer& inputBuffer) {
        TransformComponent* tComp = sgn.GetComponent<TransformComponent>();
        if (tComp != nullptr) {
            vec3<F32> localPos;
            inputBuffer >> localPos;
            tComp->setPosition(localPos);

            vec3<F32> localScale;
            inputBuffer >> localScale;
            tComp->setScale(localScale);

            vec4<F32> localRotation;
            inputBuffer >> localRotation;
            tComp->setRotation(Quaternion<F32>(localRotation));
        }
        return ECSSystem<TransformSystem>::save(sgn, inputBuffer);
    }
}; //namespace Divide