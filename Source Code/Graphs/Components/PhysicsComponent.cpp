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

void PhysicsComponent::cookCollisionMesh(const std::string& sceneName){
    STUBBED("ToDo: add terrain height field and water cooking support! -Ionut")
    FOR_EACH(SceneGraphNode::NodeChildren::value_type& it, _parentSGN->getChildren()){
        it.second->getComponent<PhysicsComponent>()->cookCollisionMesh(sceneName);
    }

    if (_parentSGN->getNode()->getType() == TYPE_OBJECT3D) {
        U32 obj3DExclussionMask = Object3D::TEXT_3D | Object3D::MESH | Object3D::FLYWEIGHT;
        if (!bitCompare(obj3DExclussionMask, dynamic_cast<Object3D*>(_parentSGN->getNode())->getObjectType()))
            PHYSICS_DEVICE.createActor(_parentSGN, sceneName,
                    _parentSGN->getUsageContext() == SceneGraphNode::NODE_STATIC ? MASK_RIGID_STATIC : MASK_RIGID_DYNAMIC,
                    _physicsCollisionGroup == NODE_COLLIDE_IGNORE ? GROUP_NON_COLLIDABLE :
                    (_physicsCollisionGroup == NODE_COLLIDE_NO_PUSH ? GROUP_COLLIDABLE_NON_PUSHABLE : GROUP_COLLIDABLE_PUSHABLE));
    }

}