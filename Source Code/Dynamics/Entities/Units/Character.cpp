#include "Headers/Character.h"

Character::Character(CharacterType type, SceneGraphNode* const node) : Unit(Unit::UNIT_TYPE_CHARACTER, node),
																	    _type(type)
{
}

Character::~Character(){
}