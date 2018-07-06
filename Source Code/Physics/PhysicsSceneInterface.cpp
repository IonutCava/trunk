#include "stdafx.h"

#include "Headers/PhysicsSceneInterface.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {
PhysicsSceneInterface::PhysicsSceneInterface(Scene& parentScene) 
    : SceneComponent(parentScene),
      GUIDWrapper()
{
}

};