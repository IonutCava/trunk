#include "Headers/Character.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Core/Math/Headers/Transform.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

Character::Character(CharacterType type, SceneGraphNode_ptr node)
    : Unit(Unit::UnitType::UNIT_TYPE_CHARACTER, node), _type(type) {
    _positionDirty = false;
    _velocityDirty = false;
    setRelativeLookingDirection(WORLD_Z_NEG_AXIS);
    _newVelocity.reset();
    _curVelocity.reset();
    PhysicsComponent* const transform = node->getComponent<PhysicsComponent>();
    if (transform) {
        _newPosition.set(transform->getPosition());
        _oldPosition.set(_newPosition);
        _curPosition.set(_oldPosition);
        _positionDirty = true;
    }
}

Character::~Character()
{
}

void Character::update(const U64 deltaTime) {
    assert(_node.lock() != nullptr);

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

    PhysicsComponent* const nodePhysicsComponent =
        getBoundNode().lock()->getComponent<PhysicsComponent>();

    vec3<F32> sourceDirection(getLookingDirection());
    sourceDirection.y = 0.0f;

    _oldPosition.set(nodePhysicsComponent->getPosition());
    _oldPosition.lerp(_curPosition, to_float(GFX_DEVICE.getInterpolation()));
    nodePhysicsComponent->setPosition(_oldPosition);
    nodePhysicsComponent->rotateSlerp(
        nodePhysicsComponent->getOrientation() *
            RotationFromVToU(sourceDirection, _curVelocity),
        to_float(GFX_DEVICE.getInterpolation()));
}

/// Just before we render the frame
bool Character::frameRenderingQueued(const FrameEvent& evt) {
    if (!getBoundNode().lock()) {
        return false;
    }

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
    SceneGraphNode_ptr node(getBoundNode().lock());

    if (node) {
        return node->getComponent<PhysicsComponent>()->getOrientation() *
               getRelativeLookingDirection();
    }

    return getRelativeLookingDirection();
}

void Character::lookAt(const vec3<F32>& targetPos) {
    SceneGraphNode_ptr node(getBoundNode().lock());

    if (!node) {
        return;
    }

    _newVelocity.set(
        node->getComponent<PhysicsComponent>()->getPosition().direction(
            targetPos));
    _velocityDirty = true;
}


void Character::playAnimation(I32 index) {
    SceneGraphNode_ptr node(getBoundNode().lock());
    if (node) {
        AnimationComponent* anim = node->getComponent<AnimationComponent>();
        if (anim) {
            anim->playAnimation(index);
        } else {
            for (SceneGraphNode_ptr child : node->getChildren()) {
                anim = child->getComponent<AnimationComponent>();
                if (anim) {
                    anim->playAnimation(index);
                }
            }
        }
    }
}

void Character::playNextAnimation() {
    SceneGraphNode_ptr node(getBoundNode().lock());
    if (node) {
        AnimationComponent* anim = node->getComponent<AnimationComponent>();
        if (anim) {
            anim->playNextAnimation();
        } else {
            for (SceneGraphNode_ptr child :  node->getChildren()) {
                anim = child->getComponent<AnimationComponent>();
                if (anim) {
                    anim->playNextAnimation();
                }
            }
        }
    }
}

void Character::playPreviousAnimation() {
    SceneGraphNode_ptr node(getBoundNode().lock());
    if (node) {
        AnimationComponent* anim = node->getComponent<AnimationComponent>();
        if (anim) {
            anim->playPreviousAnimation();
        } else {
            for (SceneGraphNode_ptr child : node->getChildren()) {
                anim = child->getComponent<AnimationComponent>();
                if (anim) {
                    anim->playPreviousAnimation();
                }
            }
        }
    }
}

void Character::pauseAnimation(bool state) {
    SceneGraphNode_ptr node(getBoundNode().lock());
    if (node) {
        AnimationComponent* anim = node->getComponent<AnimationComponent>();
        if (anim) {
            anim->playAnimations(state);
        } else {
            for (SceneGraphNode_ptr child : node->getChildren()) {
                anim = child->getComponent<AnimationComponent>();
                if (anim) {
                    anim->playAnimations(state);
                }
            }
        }
    }
}
};