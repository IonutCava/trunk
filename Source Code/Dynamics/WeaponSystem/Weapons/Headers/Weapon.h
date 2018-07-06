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

#ifndef _WEAPON_H_
#define _WEAPON_H_
#include "core.h" 

///Base class for defining a weapon
class Weapon {
public:
	/// Weapon type mask
	enum WEAPON_TYPE {
		///Melee weapon (sword,axe,kinfe,lightsaber)
		WEAPON_TYPE_MELEE       = toBit(1),
		///ranged weapons (guns, bows etc)
		WEAPON_TYPE_RANGED      = toBit(2),
		/// Place all weapon types above this
		WEAPON_TYPE_PLACEHOLDER = toBit(10)
	};

	enum WEAPON_PROPERTY {
		/// this weapon does use ammo (or charges for melee)
		WEAPON_PROPERTY_WITH_AMMO    = toBit(1), 
		/// this weapon does not us ammo
		WEAPON_PROPERTY_WITHOUT_AMMO = toBit(2),
		/// Place all weapon types above this
		WEAPON_PROPERTY_PLACEHOLDER  = toBit(10)
	};

	Weapon(WEAPON_TYPE type);
	~Weapon();

	/// Add a specific property to this weapon
	bool addProperties(U8 propertyMask);

private:
	WEAPON_TYPE _type;
	U8 _properyMask; ///< weapon properties
};
#endif