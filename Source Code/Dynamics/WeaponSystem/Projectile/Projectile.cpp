#include "Headers/Projectile.h" 

Projectile::Projectile(ProjectileType type) : _type(type)
{
	/// no placeholders please
	assert(_type != PROJECTILE_TYPE_PLACEHOLDER);
}

Projectile::~Projectile()
{
}

bool Projectile::addProperties(U8 propertyMask){
	assert((propertyMask & ~(PROJECTILE_PROPERTY_PLACEHOLDER-1)) == 0);
	_properyMask |= static_cast<ProjectileProperty>(propertyMask);
	return true;
}