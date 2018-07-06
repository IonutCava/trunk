#include "Headers/Character.h"

Character::Character(CHARACTER_TYPE type, SceneGraphNode* const node) : Unit(Unit::UNIT_TYPE_CHARACTER, node),
																	    _type(type)
{
}

Character::~Character(){
}