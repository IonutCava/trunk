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

#include "AI/Headers/AIEntity.h"

namespace Divide {
    namespace AI {

namespace Navigation{
    class DivideDtCrowd;
    class NavigationMesh;
};

class Order {
public:
    Order(U32 id) : _id(id),
                    _locked(false)
    {
    }
    inline U32 getId() const { return _id; }
    inline  bool locked() const { return _locked; }
    virtual void lock()   { _locked = true;  }
    virtual void unlock() { _locked = false; }
protected:
    U32 _id;
    bool _locked;
};

class AITeam : public GUIDWrapper{
public:

public:
    typedef hashMapImpl<AIEntity::PresetAgentRadius, Navigation::DivideDtCrowd*, hashAlg::hash<I32> > AITeamCrowd;
    typedef hashMapImpl<AIEntity*, F32 > MemberVariable;
    typedef hashMapImpl<I64, AIEntity*> TeamMap;

    AITeam(U32 id);
    ~AITeam();

    inline Navigation::DivideDtCrowd* const getCrowd(AIEntity::PresetAgentRadius radius) const {
        ReadLock r_lock(_crowdMutex);
        AITeamCrowd::const_iterator it = _aiTeamCrowd.find(radius);
        if (it != _aiTeamCrowd.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool addTeamMember(AIEntity* entity);
    bool removeTeamMember(AIEntity* entity);
    bool addEnemyTeam(U32 enemyTeamID);
    bool removeEnemyTeam(U32 enemyTeamID);

    inline void setTeamID(U32 value) { _teamID = value; }
    inline U32  getTeamID() const { return  _teamID; }
    inline I32  getEnemyTeamID(U32 index) const {
        ReadLock r_lock(_enemyTeamLock);
        if (_enemyTeams.size() <= index) {
            return -1;
        }
        return _enemyTeams[index];
    }

    inline const TeamMap&  getTeamMembers()    const {  return _team;  }
    inline MemberVariable& getMemberVariable()       { return _memberVariable; }
   
    inline void clearOrders() {
        WriteLock w_lock(_orderMutex);
        _orders.clear();
    }
    inline void addOrder(Order* const order) {
        assert(order != nullptr);
        WriteLock w_lock(_orderMutex);
        if (findOrder(order) == _orders.end()){
            _orders.push_back(order);
        }
    }

    inline void removeOrder(Order* const order) {
        assert(order != nullptr);
        WriteLock w_lock(_orderMutex);
        vectorImpl<Order* >::iterator it = findOrder(order);
        if (it != _orders.end()){
            _orders.erase(it);
        }
    }

    const vectorImpl<Order* >& requestOrders() const { return _orders; }

protected:
    friend class AIManager;
    /// Update the crowding system
    void resetCrowd();
    void processInput(const U64 deltaTime);
    void processData(const U64 deltaTime);
    void update(const U64 deltaTime);
    void init();
    void addCrowd(AIEntity::PresetAgentRadius radius, Navigation::NavigationMesh* crowd);
    void removeCrowd(AIEntity::PresetAgentRadius radius);

protected:
    inline vectorImpl<Order* >::iterator findOrder(Order* const order) {
        assert(order != nullptr);
        U32 orderId = order->getId();
        return vectorAlg::find_if( _orders.begin(), _orders.end(), [&orderId]( Order* const order )->bool {
                                                                     return orderId == order->getId(); 
                                                                  });
    }
    inline vectorImpl<U32 >::iterator AITeam::findEnemyTeamEntry(U32 enemyTeamID) {
        ReadLock r_lock(_enemyTeamLock);
        return vectorAlg::find_if( _enemyTeams.begin(), _enemyTeams.end(), [&enemyTeamID]( U32 id )->bool {
                                                                            return id == enemyTeamID; 
                                                                           });
    }

protected:
    mutable SharedLock _orderMutex;

private:
    U32 _teamID;
    TeamMap  _team;
    /// Container with data per team member. For example a map of distances
    MemberVariable _memberVariable;
    AITeamCrowd        _aiTeamCrowd;
    mutable SharedLock _updateMutex;
    mutable SharedLock _crowdMutex;
    mutable SharedLock _enemyTeamLock;
    vectorImpl<U32 >   _enemyTeams;
    vectorImpl<Order* > _orders;
};

}; //namespace AI
}; //namespace Divide

#endif
