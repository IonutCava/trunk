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
