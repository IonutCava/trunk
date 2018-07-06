#include "Headers/Weapon.h"

namespace Divide {

Weapon::Weapon(WeaponType type)
    : _type(type),
      _properyMask(0)
{
    /// no placeholders please
    assert(_type != WeaponType::COUNT);
}

Weapon::~Weapon() {}

bool Weapon::addProperty(WeaponProperty property) {
    assert(property != WeaponProperty::COUNT);
    _properyMask |= to_U32(property);
    return true;
}
};