#include "stdafx.h"

#include "ECS/Events/Headers/TransformEvents.h"

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


        vector<TransformComponent*> transforms;

        auto transform = _container->begin();
        auto transformEnd = _container->end();
        for (;transform != transformEnd; ++transform)
        {
            //transforms.push_back(transform.operator->());
            transform->PreUpdate(microSec);
        }

        //_preUpdateTask = CreateTask(context(), DELEGATE_CBK<void, Task&>());
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

        //vector<TransformComponent*> transforms;
        auto transform = _container->begin();
        auto transformEnd = _container->end();
        for (; transform != transformEnd; ++transform)
        {
            //transforms.push_back(transform.operator->());
            transform->Update(microSec);
        }

        //_preUpdateTask.wait();
        //_updateTask = CreateTask(context(), DELEGATE_CBK<void, Task&>());
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
        //vector<TransformComponent*> transforms;

        auto transform = _container->begin();
        auto transformEnd = _container->end();
        for (; transform != transformEnd; ++transform)
        {
            //transforms.push_back(transform.operator->());
            transform->PostUpdate(microSec);
        }

        //_updateTask.wait();
        //_postUpdateTask = CreateTask(context(), DELEGATE_CBK<void, Task&>());
        //for (TransformComponent* comp : transforms) {
        //    Task* child = _postUpdateTask.addChildTask(CreateTask(context(),
        //        [microSec, &comp](const Task& /*task*/) {
        //        comp->PostUpdate(microSec);
        //    }));

        //    child->startTask();
        //}

        //_postUpdateTask.startTask().wait();
    }

    void TransformSystem::FrameEnded() {

        auto transform = _container->begin();
        auto transformEnd = _container->end();
        for (; transform != transformEnd; ++transform)
        {
            transform->FrameEnded();
        }
    }

    void TransformSystem::OnUpdateLoop() {
        
        auto transform = _container->begin();
        auto transformEnd = _container->end();
        for (; transform != transformEnd; ++transform)
        {
            transform->OnUpdateLoop();
        }
    }

    bool TransformSystem::saveCache(const SceneGraphNode& sgn, ByteBuffer& outputBuffer) {
        TransformComponent* tComp = sgn.GetComponent<TransformComponent>();
        if (tComp != nullptr && !tComp->saveCache(outputBuffer)) {
            return false;
        }

        return Super::saveCache(sgn, outputBuffer);
    }

    bool TransformSystem::loadCache(SceneGraphNode& sgn, ByteBuffer& inputBuffer) {
        TransformComponent* tComp = sgn.GetComponent<TransformComponent>();
        if (tComp != nullptr && !tComp->loadCache(inputBuffer)) {
            return false;
        }

        return Super::loadCache(sgn, inputBuffer);
    }
}; //namespace Divide