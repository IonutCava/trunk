#include "Headers/PhysicsSceneInterface.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {

void PhysicsSceneInterface::addToScene(PhysicsAsset& actor, SceneGraphNode* const outNode) {
    if(outNode != nullptr) {
        outNode->getComponent<PhysicsComponent>()->physicsAsset(&actor);
    }
}

};