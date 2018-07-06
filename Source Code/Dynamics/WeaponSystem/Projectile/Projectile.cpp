#include "Headers/Projectile.h"

namespace Divide {

Projectile::Projectile(ProjectileType type)
    : _type(type),
      _properyMask(0)
{
    /// no placeholders please
    assert(_type != ProjectileType::COUNT);
}

Projectile::~Projectile() {}

bool Projectile::addProperties(ProjectileProperty property) {
    assert(property != ProjectileProperty::COUNT);
    _properyMask |= to_U32(property);
    return true;
}
};