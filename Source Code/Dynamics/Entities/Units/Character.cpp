#include "Headers/Character.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Core/Math/Headers/Transform.h"

Character::Character(CharacterType type, SceneGraphNode* const node) : Unit(Unit::UNIT_TYPE_CHARACTER, node),
                                                                       _type(type),
                                                                       _positionDirty(false),
                                                                       _velocityDirty(false)
{
    setRelativeLookingDirection(WORLD_Z_NEG_AXIS);
    _newVelocity.reset();
    _curVelocity.reset();
    if(node && node->getTransform()){
        _newPosition.set(node->getTransform()->getPosition());
        _oldPosition.set(_newPosition);
        _curPosition.set(_oldPosition);
        _positionDirty = true;
    }
}

Character::~Character()
{
}

void Character::update(const U64 deltaTime) {
    if(_positionDirty){
        _curPosition.lerp(_newPosition, getUsToSec(deltaTime));
        _positionDirty = false;
    }

    if(_velocityDirty && _newVelocity.length() > 0.0f){
        _newVelocity.y = 0.0f;
        _newVelocity.z *= -1.0f;
        _newVelocity.normalize();
        _curVelocity.lerp(_newVelocity, getUsToSec(deltaTime));
        _velocityDirty = false;
    }
}

/// Just before we render the frame
bool Character::frameRenderingQueued(const FrameEvent& evt) {
    if(!getBoundNode())
        return false;

    Transform* t = getBoundNode()->getTransform();
    assert(t != nullptr);

    vec3<F32> sourceDirection(getLookingDirection());
    sourceDirection.y = 0.0f;

    _oldPosition.set(t->getPosition());
    _oldPosition.lerp(_curPosition, evt._interpolationFactor);  
    t->setPosition(_oldPosition);
    t->rotateSlerp(t->getOrientation() * rotationFromVToU(sourceDirection, _curVelocity), evt._interpolationFactor);
    return true;
}

void Character::setPosition(const vec3<F32>& newPosition){
    _newPosition.set(newPosition);
    _positionDirty = true;
}

void Character::setVelocity(const vec3<F32>& newVelocity){
    _newVelocity.set(newVelocity);
    _velocityDirty = true;
}

vec3<F32> Character::getPosition() const {
    return _curPosition;
}

vec3<F32> Character::getLookingDirection() {
    if(getBoundNode())
        return getBoundNode()->getTransform()->getOrientation() * getRelativeLookingDirection();
    
    return getRelativeLookingDirection();
}

void Character::lookAt(const vec3<F32>& targetPos) {
    if(!getBoundNode()) 
        return;
    _newVelocity.set(getBoundNode()->getTransform()->getPosition().direction(targetPos));
    _velocityDirty = true;
}

