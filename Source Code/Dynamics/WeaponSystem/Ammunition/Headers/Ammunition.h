/*“Copyright 2009-2013 DIVIDE-Studio”*/
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

#ifndef _AMMUNITION_H_
#define _AMMUNITION_H_

#include "core.h"
///Base class for ammunition
class Ammunition {
public:
	/// Type of ammo defines it's properties
	enum AmmunitionType{
		/// Uses a counter to keep track of quantity
		AMMUNITION_TYPE_DEPLETABLE = toBit(1),
		/// Does not keep track of quantity
		AMMUNITION_TYPE_INFINITE   = toBit(2),
		/// Place all ammo types above this
		AMMUNITION_TYPE_PLACEHOLDER = toBit(10),
	};

	Ammunition(AmmunitionType type);
	~Ammunition();

private:
	AmmunitionType _type;
};

#endif