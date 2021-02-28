#include "stdafx.h"

#include "Headers/NavigationSystem.h"

namespace Divide {
    NavigationSystem::NavigationSystem(ECS::ECSEngine& parentEngine, PlatformContext& context)
        : PlatformContextComponent(context),
          ECSSystem(parentEngine)
    {
    }

    NavigationSystem::~NavigationSystem()
    {
    }

    void NavigationSystem::PreUpdate(const F32 dt) {
        Parent::PreUpdate(dt);
    }

    void NavigationSystem::Update(const F32 dt) {
        Parent::Update(dt);
    }

    void NavigationSystem::PostUpdate(const F32 dt) {
        Parent::PostUpdate(dt);
    }

    void NavigationSystem::OnFrameStart() {
        Parent::OnFrameStart();
    }

    void NavigationSystem::OnFrameEnd() {
        Parent::OnFrameEnd();
    }

    bool NavigationSystem::saveCache(const SceneGraphNode * sgn, ByteBuffer & outputBuffer) {
        return Parent::saveCache(sgn, outputBuffer);
    }

    bool NavigationSystem::loadCache(SceneGraphNode * sgn, ByteBuffer & inputBuffer) {
        return Parent::loadCache(sgn, inputBuffer);
    }
} //namespace Divide