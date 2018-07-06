#include "stdafx.h"

#include "Headers/SGNComponent.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {

SGNComponent::SGNComponent(ComponentType type, SceneGraphNode& parentSGN)
    : _type(type),
     _parentSGN(parentSGN),
     _elapsedTime(0ULL),
     _deltaTime(0ULL),
    _parentNodeActive(true)
{
}

SGNComponent::~SGNComponent()
{
}

};