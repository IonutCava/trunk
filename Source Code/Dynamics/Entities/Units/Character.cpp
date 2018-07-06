#include "stdafx.h"

#include "Headers/Character.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Core/Math/Headers/Transform.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

Character::Character(CharacterType type)
    : Unit(Unit::UnitType::UNIT_TYPE_CHARACTER), _type(type)
{
    _positionDirty = false;
    _velocityDirty = false;
    setRelativeLookingDirection(WORLD_Z_NEG_AXIS);
    _newVelocity.reset();
    _curVelocity.reset();
}

Character::~Character()
{
}

void Character::setParentNode(SceneGraphNode_ptr node) {
    Unit::setParentNode(node);
    PhysicsComponent* const transform = node->get<PhysicsComponent>();
    if (transform) {
        _newPosition.set(transform->getPosition());
        _oldPosition.set(_newPosition);
        _curPosition.set(_oldPosition);
        _positionDirty = true;
    }
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
        getBoundNode().lock()->get<PhysicsComponent>();
    
    vec3<F32> sourceDirection(getLookingDirection());
    sourceDirection.y = 0.0f;

    _oldPosition.set(nodePhysicsComponent->getPosition());
    _oldPosition.lerp(_curPosition, to_F32(GFXDevice::getFrameInterpolationFactor()));
    nodePhysicsComponent->setPosition(_oldPosition);
    nodePhysicsComponent->rotateSlerp(
        nodePhysicsComponent->getOrientation() *
            RotationFromVToU(sourceDirection, _curVelocity),
        to_F32(GFXDevice::getFrameInterpolationFactor()));
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

vec3<F32> Character::getPosition() const {
    return _curPosition;
}

vec3<F32> Character::getLookingDirection() {
    SceneGraphNode_ptr node(getBoundNode().lock());

    if (node) {
        return node->get<PhysicsComponent>()->getOrientation() *
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
        node->get<PhysicsComponent>()->getPosition().direction(
            targetPos));
    _velocityDirty = true;
}

void Character::playAnimation(I32 index) {
    SceneGraphNode_ptr node(getBoundNode().lock());
    if (node) {
        AnimationComponent* anim = node->get<AnimationComponent>();
        if (anim) {
            anim->playAnimation(index);
        } else {
            node->forEachChild([index](const SceneGraphNode& child) {
                AnimationComponent* anim = child.get<AnimationComponent>();
                if (anim) {
                    anim->playAnimation(index);
                }
            });
        }
    }
}

void Character::playNextAnimation() {
    SceneGraphNode_ptr node(getBoundNode().lock());
    if (node) {
        AnimationComponent* anim = node->get<AnimationComponent>();
        if (anim) {
            anim->playNextAnimation();
        } else {
            node->forEachChild([](const SceneGraphNode& child) {
                AnimationComponent* anim = child.get<AnimationComponent>();
                if (anim) {
                    anim->playNextAnimation();
                }
            });
        }
    }
}

void Character::playPreviousAnimation() {
    SceneGraphNode_ptr node(getBoundNode().lock());
    if (node) {
        AnimationComponent* anim = node->get<AnimationComponent>();
        if (anim) {
            anim->playPreviousAnimation();
        } else {
            node->forEachChild([](const SceneGraphNode& child) {
                AnimationComponent* anim = child.get<AnimationComponent>();
                if (anim) {
                    anim->playPreviousAnimation();
                }
            });
        }
    }
}

void Character::pauseAnimation(bool state) {
    SceneGraphNode_ptr node(getBoundNode().lock());
    if (node) {
        AnimationComponent* anim = node->get<AnimationComponent>();
        if (anim) {
            anim->playAnimations(state);
        } else {
            node->forEachChild([state](const SceneGraphNode& child) {
                AnimationComponent* anim = child.get<AnimationComponent>();
                if (anim) {
                    anim->playAnimations(state);
                }
            });
        }
    }
}
};