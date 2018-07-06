/*“Copyright 2009-2012 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PROJECTILE_H_
#define _PROJECTILE_H_
#include "core.h" 

/// Defines a projectile object (usually bullets or rockets)
class Projectile {
public:
	/// The physical characteristics of this projectile
	enum PROJECTILE_TYPE {
		/// Use a raytrace scan to the target's hitbox (raygun, non-tracked bullets)
		PROJECTILE_TYPE_RAYTRACE    = toBit(1),
		/// Use a slow-moving object as projectile (e.g. rockets, arrows, spells)
		PROJECTILE_TYPE_SLOW        = toBit(2),
		/// Add new projectile types above
		PROJECTILE_TYPE_PLACEHOLDER = toBit(10)
	};

	enum PROJECTILE_PROPERTY {
		/// Projectile is not affected by gravity (raygun, spells)
		PROJECTILE_PROPERTYE_NO_GRAVITY = toBit(1),
		/// Projectile is affected by gravity (rockets, boulders, sniperbullets)
		PROJECTILE_PROPERTY_GRAVITY     = toBit(2),
		/// Add new projectile properties above
		PROJECTILE_PROPERTY_PLACEHOLDER = toBit(10)
	};

	Projectile(PROJECTILE_TYPE type);
	~Projectile();

	/// Add a specific property to this projectile
	bool addProperties(U8 propertyMask);

private: 
	PROJECTILE_TYPE _type;
	U8 _properyMask; ///< weapon properties
};

#endif