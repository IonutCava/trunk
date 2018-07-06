#include "Headers/Character.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Core/Math/Headers/Transform.h"

Character::Character(CharacterType type, SceneGraphNode* const node) : Unit(Unit::UNIT_TYPE_CHARACTER, node),
                                                                       _type(type),
                                                                       _positionDirty(false)                                                                       
{
    _lookingDirection.set(1,0,0);
    if(node && node->getTransform()){
        _newPosition = node->getTransform()->getPosition();
        _positionDirty = true;
    }
}

Character::~Character()
{
}

void Character::update(const U64 deltaTime) {
    if(_positionDirty){
        if(getBoundNode())
            getBoundNode()->getTransform()->setPosition(_newPosition);
        _positionDirty = false;
    }
}

void Character::setPosition(const vec3<F32> newPosition){
    _newPosition = newPosition;
    _positionDirty = true;
}

vec3<F32> Character::getPosition() const {
    return getBoundNode() ? getBoundNode()->getTransform()->getPosition() : vec3<F32>();
}

vec3<F32> Character::getLookingDirection() const {
    return getBoundNode() ? getBoundNode()->getTransform()->getOrientation() * getRelativeLookingDirection() : vec3<F32>(0.0f, 0.0f, -1.0f);
}

