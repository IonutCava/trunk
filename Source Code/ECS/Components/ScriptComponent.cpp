#include "stdafx.h"

#include "Headers/ScriptComponent.h"

namespace Divide {

ScriptComponent::ScriptComponent(SceneGraphNode* parentSGN, PlatformContext& context)
    : BaseComponentType<ScriptComponent, ComponentType::SCRIPT>(parentSGN, context)
{
}

void ScriptComponent::Update(const U64 deltaTimeUS) {
    BaseComponentType<ScriptComponent, ComponentType::SCRIPT>::Update(deltaTimeUS);
}


} //namespace Divide
