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
    
    U32 childCount = _parentSGN.getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        _parentSGN.getChild(i, childCount).getComponent<NavigationComponent>()->navigationContext(_navigationContext);
    }
}

void NavigationComponent::navigationDetailOverride(const bool detailOverride) {
    _overrideNavMeshDetail = detailOverride;
    
    U32 childCount = _parentSGN.getChildCount();
    for (U32 i = 0; i < childCount; ++i) {
        _parentSGN.getChild(i, childCount).getComponent<NavigationComponent>()->navigationDetailOverride(detailOverride);
    }
}
};