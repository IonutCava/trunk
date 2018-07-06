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

#ifndef _CHARACTER_H_
#define _CHARACTER_H_

#include "Unit.h"

/// Basic character class
class Character : public Unit {
public:
	/// Currently supported character types
	enum CharacterType{
		/// user controlled character
		CHARACTER_TYPE_PLAYER,
		/// non-user(player) character
		CHARACTER_TYPE_NPC,
		/// placeholder
		CHARACTER_TYPE_PLACEHOLDER
	};

	Character(CharacterType type, SceneGraphNode* const node);
	~Character();

	///Accesors

	/// Set unit type
	inline void setCharacterType(CharacterType type) {_type = type;}
	/// Get unit type
	inline CharacterType getCharacterType()		  {return _type;}

private:
	CharacterType _type;

};
#endif