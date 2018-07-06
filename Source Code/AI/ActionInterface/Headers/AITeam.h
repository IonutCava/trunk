/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _AI_TEAM_H_
#define _AI_TEAM_H_

#include "core.h"

namespace Navigation{
    class DivideDtCrowd;
};
class AIEntity;
class AITeam {
   const static int maxAgentRadiusCount = 3;
public:
    typedef Unordered_map<I64, AIEntity*> teamMap;
    typedef Unordered_map<AIEntity*, F32 > memberVariable;

    AITeam(U32 id);
    ~AITeam();

    void update(const U64 deltaTime);
    bool addTeamMember(AIEntity* entity);
    bool removeTeamMember(AIEntity* entity);
    bool addEnemyTeam(AITeam* enemyTeam);
    void resetNavMeshes();

    inline void setTeamID(U32 value) { _teamID = value; }

    inline U32      getTeamID() const {return  _teamID;}
    inline teamMap& getTeam()         {return  _team;}

    inline AITeam& getEnemyTeam()    {return *_enemyTeam;}

    inline const Navigation::DivideDtCrowd& getCrowd(U16 radiusIndex = 0)    const {return *_teamCrowd[radiusIndex];}
    inline       Navigation::DivideDtCrowd* getCrowdPtr(U16 radiusIndex = 0) const {return _teamCrowd[radiusIndex];}
    inline memberVariable&  getMemberVariable()    {return _memberVariable;}
   
private:
    U32 _teamID;
    teamMap  _team;
    AITeam* _enemyTeam;
    Navigation::DivideDtCrowd* _teamCrowd[maxAgentRadiusCount];
    /// Container with data per team member. For example a map of distances
    memberVariable _memberVariable;
    mutable SharedLock _updateMutex;
};

#endif