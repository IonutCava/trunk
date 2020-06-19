#include "stdafx.h"

#include "Headers/NavigationComponent.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {

NavigationComponent::NavigationComponent(SceneGraphNode* parentSGN, PlatformContext& context)
    : BaseComponentType<NavigationComponent, ComponentType::NAVIGATION>(parentSGN, context),
      _navigationContext(NavigationContext::NODE_IGNORE),
      _overrideNavMeshDetail(false)
{
}

NavigationComponent::~NavigationComponent() {}

void NavigationComponent::navigationContext(const NavigationContext& newContext) {
    _navigationContext = newContext;
    
    _parentSGN->forEachChild([&newContext](const SceneGraphNode* child, I32 /*childIdx*/) {
        NavigationComponent* navComp = child->get<NavigationComponent>();
        if (navComp != nullptr) {
            navComp->navigationContext(newContext);
        }
        return true;
    });
}

void NavigationComponent::navigationDetailOverride(const bool detailOverride) {
    _overrideNavMeshDetail = detailOverride;

    _parentSGN->forEachChild([&detailOverride](const SceneGraphNode* child, I32 /*childIdx*/) {
        NavigationComponent* navComp = child->get<NavigationComponent>();
        if (navComp != nullptr) {
            navComp->navigationDetailOverride(detailOverride);
        }
        return true;

    });
}
};