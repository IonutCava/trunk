#include "stdafx.h"

#include "ECS/Events/Headers/TransformEvents.h"
#include "ECS/Components/Headers/TransformComponent.h"

#include "Headers/TransformSystem.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Core/Headers/EngineTaskPool.h"

namespace Divide {

    TransformSystem::TransformSystem(ECS::ECSEngine& parentEngine, PlatformContext& context)
        : ECSSystem(parentEngine),
          PlatformContextComponent(context)
    {

    }

    TransformSystem::~TransformSystem()
    {

    }

    void TransformSystem::PreUpdate(F32 dt) {
        U64 microSec = Time::MillisecondsToMicroseconds(dt);


        vectorImpl<TransformComponent*> transforms;
        auto transform = _engine.GetComponentManager()->begin<TransformComponent>();
        auto transformEnd = _engine.GetComponentManager()->end<TransformComponent>();
        for (;transform != transformEnd; ++transform)
        {
            //transforms.push_back(transform.operator->());
            transform->PreUpdate(microSec);
        }

        //_preUpdateTask = CreateTask(context(), DELEGATE_CBK<void, const Task&>());
        //for (TransformComponent* comp : transforms) {
        //    Task* child = _preUpdateTask.addChildTask(CreateTask(context(),
        //        [microSec, &comp](const Task& /*task*/) {
        //        comp->PreUpdate(microSec);
        //    }));

        //    child->startTask();
        //}
        //_preUpdateTask.startTask();
    }

    void TransformSystem::Update(F32 dt) {
        U64 microSec = Time::MillisecondsToMicroseconds(dt);

        vectorImpl<TransformComponent*> transforms;
        auto transform = _engine.GetComponentManager()->begin<TransformComponent>();
        auto transformEnd = _engine.GetComponentManager()->end<TransformComponent>();
        for (; transform != transformEnd; ++transform)
        {
            //transforms.push_back(transform.operator->());
            transform->Update(microSec);
        }

        //_preUpdateTask.wait();
        //_updateTask = CreateTask(context(), DELEGATE_CBK<void, const Task&>());
        //for (TransformComponent* comp : transforms) {
        //    Task* child = _updateTask.addChildTask(CreateTask(context(),
        //        [microSec, &comp](const Task& /*task*/) {
        //        comp->Update(microSec);
        //    }));

        //    child->startTask();

        //}
        //_updateTask.startTask();
    }

    void TransformSystem::PostUpdate(F32 dt) {
        U64 microSec = Time::MillisecondsToMicroseconds(dt);
        vectorImpl<TransformComponent*> transforms;
        auto transform = _engine.GetComponentManager()->begin<TransformComponent>();
        auto transformEnd = _engine.GetComponentManager()->end<TransformComponent>();
        for (; transform != transformEnd; ++transform)
        {
            //transforms.push_back(transform.operator->());
            transform->PostUpdate(microSec);
        }

        //_updateTask.wait();
        //_postUpdateTask = CreateTask(context(), DELEGATE_CBK<void, const Task&>());
        //for (TransformComponent* comp : transforms) {
        //    Task* child = _postUpdateTask.addChildTask(CreateTask(context(),
        //        [microSec, &comp](const Task& /*task*/) {
        //        comp->PostUpdate(microSec);
        //    }));

        //    child->startTask();
        //}

        //_postUpdateTask.startTask().wait();
    }

    bool TransformSystem::save(const SceneGraphNode& sgn, ByteBuffer& outputBuffer) {
        TransformComponent* tComp = sgn.GetComponent<TransformComponent>();
        if (tComp != nullptr && !tComp->save(outputBuffer)) {
            return false;
        }
         
        return ECSSystem<TransformSystem>::save(sgn, outputBuffer);
    }

    bool TransformSystem::load(SceneGraphNode& sgn, ByteBuffer& inputBuffer) {
        TransformComponent* tComp = sgn.GetComponent<TransformComponent>();
        if (tComp != nullptr && !tComp->load(inputBuffer)) {
            return false;
        }
           
        return ECSSystem<TransformSystem>::save(sgn, inputBuffer);
    }
}; //namespace Divide