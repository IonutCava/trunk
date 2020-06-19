#include "stdafx.h"

#include "Headers/BoundsSystem.h"

namespace Divide {
    BoundsSystem::BoundsSystem(ECS::ECSEngine& parentEngine, PlatformContext& context)
        : PlatformContextComponent(context),
          ECSSystem(parentEngine)
    {
    }

    BoundsSystem::~BoundsSystem()
    {
    }

}; //namespace Divide