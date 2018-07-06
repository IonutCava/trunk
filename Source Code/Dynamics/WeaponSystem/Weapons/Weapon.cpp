#include "Headers/Weapon.h"

Weapon::Weapon(WEAPON_TYPE type) : _type(type)
{
	/// no placeholders please
	assert(_type != WEAPON_TYPE_PLACEHOLDER);
}

Weapon::~Weapon()
{
}

bool Weapon::addProperties(U8 propertyMask){
	assert((propertyMask & ~(WEAPON_PROPERTY_PLACEHOLDER-1)) == 0);
	_properyMask |= static_cast<WEAPON_PROPERTY>(propertyMask);
}