#include "Headers/Projectile.h" 

Projectile::Projectile(PROJECTILE_TYPE type) : _type(type)
{
	/// no placeholders please
	assert(_type != PROJECTILE_TYPE_PLACEHOLDER);
}

Projectile::~Projectile()
{
}

bool Projectile::addProperties(U8 propertyMask){
	assert((propertyMask & ~(PROJECTILE_PROPERTY_PLACEHOLDER-1)) == 0);
	_properyMask |= static_cast<PROJECTILE_PROPERTY>(propertyMask);
	return true;
}