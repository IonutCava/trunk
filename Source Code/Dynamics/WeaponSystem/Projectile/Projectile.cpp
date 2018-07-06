#include "Headers/Projectile.h"

namespace Divide {

Projectile::Projectile(ProjectileType type) : _type(type) {
    /// no placeholders please
    assert(_type != ProjectileType::COUNT);
}

Projectile::~Projectile() {}

bool Projectile::addProperties(ProjectileProperty property) {
    assert(property != ProjectileProperty::COUNT);
    _properyMask |= to_uint(property);
    return true;
}
};