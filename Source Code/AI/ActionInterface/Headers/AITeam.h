/*
   Copyright (c) 2018 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _AI_TEAM_H_
#define _AI_TEAM_H_

#include "AI/Headers/AIEntity.h"

namespace Divide {

class TaskPool;

namespace AI {

class AIManager;

namespace Navigation {
    class DivideDtCrowd;
    class NavigationMesh;
};

class Order {
   public:
    Order(U32 id) : _id(id), _locked(false) {}
    inline U32 getID() const { return _id; }
    inline bool locked() const { return _locked; }
    virtual void lock() { _locked = true; }
    virtual void unlock() { _locked = false; }

   protected:
    U32 _id;
    bool _locked;
};

class AITeam : public GUIDWrapper {
   public:
    typedef Navigation::DivideDtCrowd* CrowdPtr;
    typedef hashMap<AIEntity::PresetAgentRadius, CrowdPtr> AITeamCrowd;
    typedef hashMap<AIEntity*, F32> MemberVariable;
    typedef hashMap<I64, AIEntity*> TeamMap;
    typedef std::shared_ptr<Order> OrderPtr;
    typedef vector<OrderPtr> OrderList;
   public:
    AITeam(U32 id, AIManager& parentManager);
    ~AITeam();

    inline CrowdPtr const getCrowd(AIEntity::PresetAgentRadius radius) const {
        ReadLock r_lock(_crowdMutex);
        AITeamCrowd::const_iterator it = _aiTeamCrowd.find(radius);
        if (it != std::end(_aiTeamCrowd)) {
            return it->second;
        }
        return nullptr;
    }

    bool addTeamMember(AIEntity* entity);
    bool removeTeamMember(AIEntity* entity);
    bool addEnemyTeam(U32 enemyTeamID);
    bool removeEnemyTeam(U32 enemyTeamID);

    inline void setTeamID(U32 value) { _teamID = value; }
    inline U32  getTeamID() const { return _teamID; }
    inline I32  getEnemyTeamID(U32 index) const {
        if (_enemyTeams.size() <= index) {
            return -1;
        }
        return _enemyTeams[index];
    }

    inline const TeamMap& getTeamMembers() const { return _team; }
    inline MemberVariable& getMemberVariable() { return _memberVariable; }

    inline void clearOrders() {
        WriteLock w_lock(_orderMutex);
        _orders.clear();
    }

    inline void addOrder(const OrderPtr& order) {
        WriteLock w_lock(_orderMutex);
        if (findOrder(order->getID()) == std::end(_orders)) {
            _orders.push_back(order);
        }
    }

    inline void removeOrder(const Order& order) {
        WriteLock w_lock(_orderMutex);
        OrderList::iterator it = findOrder(order);
        if (it != std::end(_orders)) {
            _orders.erase(it);
        }
    }

    const OrderList& requestOrders() const { return _orders; }

   protected:
    friend class AIManager;
    /// Update the crowding system
    void resetCrowd();
    bool processInput(TaskPool& parentPool, const U64 deltaTimeUS);
    bool processData(TaskPool& parentPool, const U64 deltaTimeUS);
    bool update(TaskPool& parentPool, const U64 deltaTimeUS);
    void addCrowd(AIEntity::PresetAgentRadius radius,
                  Navigation::NavigationMesh* crowd);
    void removeCrowd(AIEntity::PresetAgentRadius radius);

   protected:
    vector<AIEntity*> getEntityList() const;

    inline OrderList::iterator findOrder(const Order& order) {
        return findOrder(order.getID());
    }

    inline OrderList::iterator findOrder(U32 orderID) {
        return vectorAlg::find_if(
            std::begin(_orders), 
            std::end(_orders),
            [&orderID](const OrderPtr& order) -> bool {
                return orderID == order->getID();
            });
    }

    inline vector<U32>::iterator findEnemyTeamEntry(U32 enemyTeamID) {
        return vectorAlg::find_if(
            std::begin(_enemyTeams), std::end(_enemyTeams),
            [&enemyTeamID](U32 id) -> bool { return id == enemyTeamID; });
    }

   protected:
    mutable SharedLock _orderMutex;

   private:
    U32 _teamID;
    TeamMap _team;
    AI::AIManager& _parentManager;
    /// Container with data per team member. For example a map of distances
    MemberVariable _memberVariable;
    AITeamCrowd _aiTeamCrowd;
    mutable SharedLock _updateMutex;
    mutable SharedLock _crowdMutex;
    vector<U32> _enemyTeams;
    OrderList _orders;
};

};  // namespace AI
};  // namespace Divide

#endif
