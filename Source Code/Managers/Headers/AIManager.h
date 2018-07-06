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

#ifndef _AI_MANAGER_H_
#define _AI_MANAGER_G_

#include "resource.h"
#include "AI/Headers/AIEntity.h"

DEFINE_SINGLETON(AIManager)
	typedef unordered_map<U32, AIEntity*> AIEntityMap;

public:
	U8 tick();
	bool addEntity(AIEntity* entity);
	void Destroy();

private:
	void processInput();  ///< sensors
	void processData();   ///< think
	void updateEntities();///< react

private:
	///ToDo: Maybe create the "Unit" class and agregate it with AIEntity? -Ionut
	AIEntityMap _aiEntities;
	boost::mutex _updateMutex;

END_SINGLETON

#endif