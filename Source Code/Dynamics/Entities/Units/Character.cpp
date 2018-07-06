#include "Headers/Character.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Core/Math/Headers/Transform.h"

Character::Character(CharacterType type, SceneGraphNode* const node) : Unit(Unit::UNIT_TYPE_CHARACTER, node),
                                                                       _type(type),
                                                                       _positionDirty(false),
                                                                       _velocityDirty(false)
{
    setRelativeLookingDirection(-WORLD_Z_AXIS);
    _newVelocity.set(0,0,0);
    if(node && node->getTransform()){
        _newPosition = node->getTransform()->getPosition();
        _positionDirty = true;
    }
}

Character::~Character()
{
}

void Character::update(const U64 deltaTime) {
    if(getBoundNode()){
        Transform* t = getBoundNode()->getTransform();
        assert(t != NULL);

        if(_positionDirty)
            t->setPosition(_newPosition);

        if(_velocityDirty){
            if(_newVelocity.length() > 0){
                vec3<F32> sourceDirection = getLookingDirection();
                const Quaternion<F32>& orientation = t->getOrientation();
                sourceDirection.y = 0.0f;
                _newVelocity.y = 0.0f;
                _newVelocity.z *= -1.0f;
                _newVelocity.normalize();
                t->rotateSlerp(orientation * rotationFromVToU(sourceDirection, _newVelocity), getUsToSec(deltaTime) * 2.0f);
            }
        }
    }
    
    _positionDirty = false;
    _velocityDirty = false;
}

void Character::setPosition(const vec3<F32>& newPosition){
    _newPosition = newPosition;
    _positionDirty = true;
}

void Character::setVelocity(const vec3<F32>& newVelocity){
    _newVelocity = newVelocity;
    _velocityDirty = true;
}

vec3<F32> Character::getPosition() const {
    return getBoundNode() ? getBoundNode()->getTransform()->getPosition() : vec3<F32>();
}

vec3<F32> Character::getLookingDirection() {
    if(getBoundNode())
        return getBoundNode()->getTransform()->getOrientation() * getRelativeLookingDirection();
    
    return getRelativeLookingDirection();
}

