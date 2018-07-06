/*“Copyright 2009-2011 DIVIDE-Studio”*/
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

#ifndef _AI_COORDINATION_H_
#define _AI_COORDINATION_H_

#include "resource.h"

class AIEntity;
class AICoordination {
public:
	typedef unordered_map<U32, AIEntity*> teamMap;
	AICoordination() {}

	bool addTeamMember(AIEntity* entity);
	bool removeTeamMember(AIEntity* entity);
	bool addEnemyTeam(teamMap& enemyTeam);
	inline teamMap& getTeam() {return _team;}
	inline U32 getTeamID() {return _teamID;}
	void setTeamID(U32 value);
	inline teamMap& getEnemyTeam() {return _enemyTeam;}

private:
	U32 _teamID;
	teamMap _team;
	teamMap _enemyTeam;
	boost::mutex _updateMutex;
};

#endif
