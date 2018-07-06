#include "Headers/PhysicsSceneInterface.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {
PhysicsSceneInterface::PhysicsSceneInterface(Scene* parentScene) 
    : _parentScene(parentScene)
{
}

Scene* PhysicsSceneInterface::getParentScene() {
    return _parentScene;
}
};