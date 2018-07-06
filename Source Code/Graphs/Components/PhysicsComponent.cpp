#include "Headers/PhysicsComponent.h"

#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Dynamics/Physics/Headers/PXDevice.h"
#include "Dynamics/Physics/Headers/PhysicsAPIWrapper.h"

namespace Divide {

PhysicsComponent::PhysicsComponent(SceneGraphNode* const parentSGN) : SGNComponent(SGNComponent::SGN_COMP_PHYSICS, parentSGN),
                                                                       _physicsCollisionGroup(NODE_COLLIDE_IGNORE),
                                                                       _noDefaultTransform(false),
                                                                       _transformUpdated(true),
                                                                       _physicsAsset(nullptr),
                                                                       _transform(nullptr)
{

}

PhysicsComponent::~PhysicsComponent()
{
    SAFE_DELETE(_transform);
}

//Get the node's transform
Transform* PhysicsComponent::getTransform() {
    //A node does not necessarily have a transform. If this is the case, we can either create a default one or return nullptr.
    //When creating a node we can specify if we do not want a default transform
    if(!_noDefaultTransform && !_transform){
        _transform = New Transform();
        assert(_transform);
    }
    return _transform;
}

void PhysicsComponent::physicsAsset(PhysicsAsset* asset) {
    DIVIDE_ASSERT(_physicsAsset == nullptr, "PhysicsComponent error: Double set physics asset detected! remove the previous one first!");
    _physicsAsset = asset;
    _physicsAsset->setParent(this);
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
                    _parentSGN->usageContext() == SceneGraphNode::NODE_STATIC ? MASK_RIGID_STATIC : MASK_RIGID_DYNAMIC,
                    _physicsCollisionGroup == NODE_COLLIDE_IGNORE ? GROUP_NON_COLLIDABLE :
                    (_physicsCollisionGroup == NODE_COLLIDE_NO_PUSH ? GROUP_COLLIDABLE_NON_PUSHABLE : GROUP_COLLIDABLE_PUSHABLE));
    }

}

void PhysicsComponent::setTransformDirty() {
    if (_physicsAsset) {
        _physicsAsset->resetTransforms(true);
    }
    _transformUpdated = true;
}

void PhysicsComponent::setPosition(const vec3<F32>& position) {
    Transform* transform = getTransform();
    if (transform){
        transform->setPosition(position);
        setTransformDirty();
    }
}

void PhysicsComponent::setScale(const vec3<F32>& scale) {
    Transform* transform = getTransform();
    if (transform){
        transform->setScale(scale);
        setTransformDirty();
    }
}

void PhysicsComponent::setRotation(const vec3<F32>& axis, F32 degrees, bool inDegrees) {
    Transform* transform = getTransform();
    if (transform){
        transform->setRotation(axis, degrees, inDegrees);
        setTransformDirty();
    }
}

void PhysicsComponent::setRotation(const vec3<F32>& euler, bool inDegrees) {
    Transform* transform = getTransform();
    if (transform){
        transform->setRotation(euler, inDegrees);
        setTransformDirty();
    }
}

void PhysicsComponent::setRotation(const Quaternion<F32>& quat) {
    Transform* transform = getTransform();
    if (transform){
        transform->setRotation(quat);
        setTransformDirty();
    }
}

void PhysicsComponent::translate(const vec3<F32>& axisFactors) {
    Transform* transform = getTransform();
    if (transform){
        transform->translate(axisFactors);
        setTransformDirty();
    }
}

void PhysicsComponent::scale(const vec3<F32>& axisFactors) {
    Transform* transform = getTransform();
    if (transform){
        transform->scale(axisFactors);
        setTransformDirty();
    }
}

void PhysicsComponent::rotate(const vec3<F32>& axis, F32 degrees, bool inDegrees) {
    Transform* transform = getTransform();
    if (transform){
        transform->rotate(axis, degrees, inDegrees);
        setTransformDirty();
    }
}

void PhysicsComponent::rotate(const vec3<F32>& euler, bool inDegrees) {
    Transform* transform = getTransform();
    if (transform){
        transform->rotate(euler, inDegrees);
        setTransformDirty();
    }
}

void PhysicsComponent::rotate(const Quaternion<F32>& quat) {
    Transform* transform = getTransform();
    if (transform){
        transform->rotate(quat);
        setTransformDirty();
    }
}

void PhysicsComponent::rotateSlerp(const Quaternion<F32>& quat, const D32 deltaTime) {
    Transform* transform = getTransform();
    if (transform){
        transform->rotateSlerp(quat, deltaTime);
        setTransformDirty();
    }
}

void PhysicsComponent::setScale(const F32 scale) {
    Transform* transform = getTransform();
    if (transform){
        transform->setScale(scale);
        setTransformDirty();
    }
}

void PhysicsComponent::setScaleX(const F32 scale) {
    Transform* transform = getTransform();
    if (transform){
        transform->setScaleX(scale);
        setTransformDirty();
    }
}

void PhysicsComponent::setScaleY(const F32 scale) {
    Transform* transform = getTransform();
    if (transform){
        transform->setScaleY(scale);
        setTransformDirty();
    }
}

void PhysicsComponent::setScaleZ(const F32 scale) {
    Transform* transform = getTransform();
    if (transform){
        transform->setScaleZ(scale);
        setTransformDirty();
    }
}

void PhysicsComponent::scale(const F32 scale) {
    Transform* transform = getTransform();
    if (transform){
        transform->scale(scale);
        setTransformDirty();
    }
}

void PhysicsComponent::scaleX(const F32 scale) {
    Transform* transform = getTransform();
    if (transform){
        transform->scaleX(scale);
        setTransformDirty();
    }
}

void PhysicsComponent::scaleY(const F32 scale) {
    Transform* transform = getTransform();
    if (transform){
        transform->scaleY(scale);
        setTransformDirty();
    }
}

void PhysicsComponent::scaleZ(const F32 scale) {
    Transform* transform = getTransform();
    if (transform){
        transform->scaleZ(scale);
        setTransformDirty();
    }
}

void PhysicsComponent::rotateX(const F32 angle, bool inDegrees) {
    Transform* transform = getTransform();
    if (transform){
        transform->rotateX(angle, inDegrees);
        setTransformDirty();
    }
}
void PhysicsComponent::rotateY(const F32 angle, bool inDegrees) {
    Transform* transform = getTransform();
    if (transform){
        transform->rotateY(angle, inDegrees);
        setTransformDirty();
    }
}

void PhysicsComponent::rotateZ(const F32 angle, bool inDegrees) {
    Transform* transform = getTransform();
    if (transform){
        transform->rotateZ(angle, inDegrees);
        setTransformDirty();
    }
}

void PhysicsComponent::setRotationX(const F32 angle, bool inDegrees) {
    Transform* transform = getTransform();
    if (transform){
        transform->setRotationX(angle, inDegrees);
        setTransformDirty();
    }
}

void PhysicsComponent::setRotationXEuler(const F32 angle, bool inDegrees) {
    Transform* transform = getTransform();
    if (transform){
        transform->setRotationXEuler(angle, inDegrees);
        setTransformDirty();
    }
}

void PhysicsComponent::setRotationY(const F32 angle, bool inDegrees) {
    Transform* transform = getTransform();
    if (transform){
        transform->setRotationY(angle, inDegrees);
        setTransformDirty();
    }
}

void PhysicsComponent::setRotationZ(const F32 angle, bool inDegrees) {
    Transform* transform = getTransform();
    if (transform){
        transform->setRotationZ(angle, inDegrees);
        setTransformDirty();
    }
}

void PhysicsComponent::translateX(const F32 positionX) {
    Transform* transform = getTransform();
    if (transform){
        transform->translateX(positionX);
        setTransformDirty();
    }
}

void PhysicsComponent::translateY(const F32 positionY) {
    Transform* transform = getTransform();
    if (transform){
        transform->translateY(positionY);
        setTransformDirty();
    }
}

void PhysicsComponent::translateZ(const F32 positionZ) {
    Transform* transform = getTransform();
    if (transform){
        transform->translateZ(positionZ);
        setTransformDirty();
    }
}

void PhysicsComponent::setPositionX(const F32 positionX) {
    Transform* transform = getTransform();
    if (transform){
        transform->setPositionX(positionX);
        setTransformDirty();
    }
}

void PhysicsComponent::setPositionY(const F32 positionY) {
    Transform* transform = getTransform();
    if (transform){
        transform->setPositionY(positionY);
        setTransformDirty();
    }
}

void PhysicsComponent::setPositionZ(const F32 positionZ) {
    Transform* transform = getTransform();
    if (transform){
        transform->setPositionZ(positionZ);
        setTransformDirty();
    }
}

};