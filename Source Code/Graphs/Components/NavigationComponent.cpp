#include "Headers/NavigationComponent.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {

NavigationComponent::NavigationComponent(SceneGraphNode& parentSGN)
    : SGNComponent(SGNComponent::ComponentType::NAVIGATION, parentSGN),
      _overrideNavMeshDetail(false),
      _navigationContext(NavigationContext::NODE_IGNORE)
{
}

NavigationComponent::~NavigationComponent() {}

void NavigationComponent::navigationContext(
    const NavigationContext& newContext) {
    _navigationContext = newContext;
    
    for (SceneGraphNode_ptr child : _parentSGN.getChildren()) {
        child->getComponent<NavigationComponent>()->navigationContext(_navigationContext);
    }
}

void NavigationComponent::navigationDetailOverride(const bool detailOverride) {
    _overrideNavMeshDetail = detailOverride;
    
    for (SceneGraphNode_ptr child : _parentSGN.getChildren()) {
        child->getComponent<NavigationComponent>()->navigationDetailOverride(detailOverride);
    }
}
};