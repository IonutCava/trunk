#include "Headers/Weapon.h"

namespace Divide {

Weapon::Weapon(WeaponType type) : _type(type)
{
    /// no placeholders please
    assert(_type != WEAPON_TYPE_PLACEHOLDER);
}

Weapon::~Weapon()
{
}

bool Weapon::addProperties(U8 propertyMask){
    assert((propertyMask & ~(WEAPON_PROPERTY_PLACEHOLDER-1)) == 0);
    _properyMask |= static_cast<WeaponType>(propertyMask);
    return true;
}

};