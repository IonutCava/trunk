#include "Headers/NavigationComponent.h"

#include "Graphs/Headers/SceneGraphNode.h"

NavigationComponent::NavigationComponent(SceneGraphNode* const parentSGN) : SGNComponent(SGNComponent::SGN_COMP_NAVIGATION, parentSGN),
                                                                            _overrideNavMeshDetail(false),
                                                                            _navigationContext(NODE_IGNORE)
{

}

NavigationComponent::~NavigationComponent()
{

}

void NavigationComponent::setNavigationContext(const NavigationContext& newContext) {
    _navigationContext = newContext;
    FOR_EACH(SceneGraphNode::NodeChildren::value_type& it, _parentSGN->getChildren()){
        it.second->getComponent<NavigationComponent>()->setNavigationContext(_navigationContext);
    }
}

void  NavigationComponent::setNavigationDetailOverride(const bool detailOverride){
    _overrideNavMeshDetail = detailOverride;
    FOR_EACH(SceneGraphNode::NodeChildren::value_type& it, _parentSGN->getChildren()){
        it.second->getComponent<NavigationComponent>()->setNavigationDetailOverride(detailOverride);
    }
}