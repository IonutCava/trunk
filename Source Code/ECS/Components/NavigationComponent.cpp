#include "stdafx.h"

#include "Headers/NavigationComponent.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {

NavigationComponent::NavigationComponent(SceneGraphNode& parentSGN)
    : SGNComponent(parentSGN, getComponentTypeName(ComponentType::NAVIGATION)),
      _overrideNavMeshDetail(false),
      _navigationContext(NavigationContext::NODE_IGNORE)
{
}

NavigationComponent::~NavigationComponent() {}

void NavigationComponent::navigationContext(const NavigationContext& newContext) {
    _navigationContext = newContext;
    
    _parentSGN.forEachChild([&newContext](const SceneGraphNode& child) {
        child.get<NavigationComponent>()->navigationContext(newContext);
    });
}

void NavigationComponent::navigationDetailOverride(const bool detailOverride) {
    _overrideNavMeshDetail = detailOverride;

    _parentSGN.forEachChild([&detailOverride](const SceneGraphNode& child) {
        child.get<NavigationComponent>()->navigationDetailOverride(detailOverride);
    });
}
};