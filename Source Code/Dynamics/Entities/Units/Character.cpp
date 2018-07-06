#include "Headers/Character.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Core/Math/Headers/Transform.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

Character::Character(CharacterType type, SceneGraphNode& node)
    : Unit(Unit::UnitType::UNIT_TYPE_CHARACTER, node), _type(type) {
    _positionDirty = false;
    _velocityDirty = false;
    setRelativeLookingDirection(WORLD_Z_NEG_AXIS);
    _newVelocity.reset();
    _curVelocity.reset();
    PhysicsComponent* const transform = node.getComponent<PhysicsComponent>();
    if (transform) {
        _newPosition.set(transform->getPosition());
        _oldPosition.set(_newPosition);
        _curPosition.set(_oldPosition);
        _positionDirty = true;
    }
}

Character::~Character() {}

void Character::update(const U64 deltaTime) {
    assert(_node != nullptr);

    if (_positionDirty) {
        _curPosition.lerp(_newPosition,
                          Time::MicrosecondsToSeconds<F32>(deltaTime));
        _positionDirty = false;
    }

    if (_velocityDirty && _newVelocity.length() > 0.0f) {
        _newVelocity.y = 0.0f;
        _newVelocity.z *= -1.0f;
        _newVelocity.normalize();
        _curVelocity.lerp(_newVelocity,
                          Time::MicrosecondsToSeconds<F32>(deltaTime));
        _velocityDirty = false;
    }
}

/// Just before we render the frame
bool Character::frameRenderingQueued(const FrameEvent& evt) {
    if (!getBoundNode()) {
        return false;
    }
    PhysicsComponent* const nodePhysicsComponent =
        getBoundNode()->getComponent<PhysicsComponent>();

    vec3<F32> sourceDirection(getLookingDirection());
    sourceDirection.y = 0.0f;

    _oldPosition.set(nodePhysicsComponent->getPosition());
    _oldPosition.lerp(_curPosition, GFX_DEVICE.getInterpolation());
    nodePhysicsComponent->setPosition(_oldPosition);
    nodePhysicsComponent->rotateSlerp(
        nodePhysicsComponent->getOrientation() *
            rotationFromVToU(sourceDirection, _curVelocity),
        GFX_DEVICE.getInterpolation());
    return true;
}

void Character::setPosition(const vec3<F32>& newPosition) {
    _newPosition.set(newPosition);
    _positionDirty = true;
}

void Character::setVelocity(const vec3<F32>& newVelocity) {
    _newVelocity.set(newVelocity);
    _velocityDirty = true;
}

vec3<F32> Character::getPosition() const { return _curPosition; }

vec3<F32> Character::getLookingDirection() {
    if (getBoundNode())
        return getBoundNode()
                   ->getComponent<PhysicsComponent>()
                   ->getOrientation() *
               getRelativeLookingDirection();

    return getRelativeLookingDirection();
}

void Character::lookAt(const vec3<F32>& targetPos) {
    if (!getBoundNode()) return;
    _newVelocity.set(getBoundNode()
                         ->getComponent<PhysicsComponent>()
                         ->getPosition()
                         .direction(targetPos));
    _velocityDirty = true;
}
};