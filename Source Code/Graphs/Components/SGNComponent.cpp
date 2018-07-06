#include "Headers/SGNComponent.h"
#include "Graphs/Headers/SceneGraphNode.h"

SGNComponent::SGNComponent(ComponentType type, SceneGraphNode* const parentSGN) : _type(type),
                                                                                  _parentSGN(parentSGN),
                                                                                  _elapsedTime(0ULL),
                                                                                  _deltaTime(0ULL)
{
    _instanceID = parentSGN->getInstanceID();
}

SGNComponent::~SGNComponent()
{

}