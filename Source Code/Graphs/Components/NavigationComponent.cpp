#include "Headers/NavigationComponent.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {

NavigationComponent::NavigationComponent(SceneGraphNode& parentSGN)
    : SGNComponent(SGNComponent::SGN_COMP_NAVIGATION, parentSGN),
      _overrideNavMeshDetail(false),
      _navigationContext(NODE_IGNORE) {}

NavigationComponent::~NavigationComponent() {}

void NavigationComponent::navigationContext(
    const NavigationContext& newContext) {
    _navigationContext = newContext;
    for (SceneGraphNode::NodeChildren::value_type& it :
         _parentSGN.getChildren()) {
        it.second->getComponent<NavigationComponent>()->navigationContext(
            _navigationContext);
    }
}

void NavigationComponent::navigationDetailOverride(const bool detailOverride) {
    _overrideNavMeshDetail = detailOverride;
    for (SceneGraphNode::NodeChildren::value_type& it :
         _parentSGN.getChildren()) {
        it.second->getComponent<NavigationComponent>()
            ->navigationDetailOverride(detailOverride);
    }
}
};