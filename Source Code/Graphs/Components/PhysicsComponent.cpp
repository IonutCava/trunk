#include "Headers/PhysicsComponent.h"

#include "Graphs/Headers/SceneGraphNode.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Dynamics/Physics/Headers/PXDevice.h"

PhysicsComponent::PhysicsComponent(SceneGraphNode* const parentSGN) : SGNComponent(SGNComponent::SGN_COMP_PHYSICS, parentSGN),
                                                                       _physicsCollisionGroup(NODE_COLLIDE_IGNORE)
{

}

PhysicsComponent::~PhysicsComponent()
{

}

#pragma message("ToDo: add terrain heightfield and water cooking support! -Ionut")
void PhysicsComponent::cookCollisionMesh(const std::string& sceneName){
    SceneNodeType nodeType = _parentSGN->getSceneNode()->getType();
    if (nodeType != TYPE_SKY && nodeType != TYPE_WATER && nodeType != TYPE_TERRAIN)
        FOR_EACH(SceneGraphNode::NodeChildren::value_type& it, _parentSGN->getChildren()){
            it.second->getComponent<PhysicsComponent>()->cookCollisionMesh(sceneName);
        }

    if (!bitCompare(/*TYPE_WATER | TYPE_TERRAIN | */TYPE_OBJECT3D, nodeType))
        return;

    if (nodeType == TYPE_OBJECT3D) {
        if (bitCompare(Object3D::TEXT_3D | Object3D::MESH | Object3D::FLYWEIGHT, dynamic_cast<Object3D*>(_parentSGN->getSceneNode())->getType()))
            return;
    }

    PHYSICS_DEVICE.createActor(_parentSGN, sceneName,
        _parentSGN->getUsageContext() == SceneGraphNode::NODE_STATIC ? MASK_RIGID_STATIC : MASK_RIGID_DYNAMIC,
        _physicsCollisionGroup == NODE_COLLIDE_IGNORE ? GROUP_NON_COLLIDABLE :
        (_physicsCollisionGroup == NODE_COLLIDE_NO_PUSH ? GROUP_COLLIDABLE_NON_PUSHABLE : GROUP_COLLIDABLE_PUSHABLE));
}