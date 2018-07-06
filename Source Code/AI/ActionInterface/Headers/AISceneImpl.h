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

#ifndef _AI_SCENE_IMPLEMENTATION_H_
#define _AI_SCENE_IMPLEMENTATION_H_

#include "GOAPInterface.h"
#include "AI/Headers/AIEntity.h"
#include "Core/Headers/cdigginsAny.h"
#include <boost/noncopyable.hpp>
#include <boost/atomic.hpp>

class Texture;
namespace AI {

enum AIMsg;
/// Provides a scene-level AI implementation
class AISceneImpl : private boost::noncopyable {
public:
    AISceneImpl() : _entity(nullptr),
                    _activeGoal(nullptr),
                    _currentStep(-1)
    {
    }

    virtual ~AISceneImpl() 
    {
        _goals.clear();
    }

    virtual void addEntityRef(AIEntity* entity) {
        if (entity) {
            _entity = entity;
        }
    }

    inline GOAPWorldState& worldState() { return _worldState; }

    /// Register a specific action. This only holds a reference to the action itself and does not create a local copy!
    virtual void registerAction(GOAPAction* const action) {
        _actionSet.push_back(action);
    }
    /// Register a specific action. This only holds a reference to the action itself and does not create a local copy!
    virtual void registerGoal(const GOAPGoal& goal) {
        _goals.push_back(goal);
    }

    virtual GOAPGoal* findGoal(const std::string& goalName) {
        vectorImpl<GOAPGoal >::iterator it;
        it = std::find_if(_goals.begin(), _goals.end(), [&goalName](const GOAPGoal& goal) { 
                                                            return goal.getName().compare(goalName) == 0; 
                                                        });
        if (it != _goals.end()) {
            return &(*it);
        }
    
        return nullptr;
    }

    inline const vectorImpl<GOAPGoal >& goalListConst() const { return _goals; }

protected:
    friend class AIEntity;
    inline vectorImpl<GOAPGoal >& goalList() { return _goals; }
    inline GOAPActionSet&  actionSetPtr() { return _actionSet; }

    inline void resetActiveGoals() {
        _activeGoals.clear();
        for (GOAPGoal& goal : goalList()) {
            goal.relevancy(0.0f);
            activateGoal(goal.getName());
        }
    }
    /// Although we want the goal to be activated, it might not be the most relevant in the current scene state
    inline bool activateGoal(const std::string& name) {
        GOAPGoal* goal = findGoal(name);
        if (goal != nullptr) {
            _activeGoals.push_back(goal);
        }
        return (goal != nullptr);
    }

    /// Get the most relevant goal and set it as active
    GOAPGoal* findRelevantGoal() {

        if (_activeGoals.empty()) {
            return nullptr;
        }
        std::sort(_activeGoals.begin(), _activeGoals.end(), [](GOAPGoal const* a, GOAPGoal const* b) {
                                                                return a->relevancy() < b->relevancy();
                                                            });
        _activeGoal = _activeGoals.back();
        _currentStep = -1;
        return _activeGoal;
    }

    bool popActiveGoal(GOAPGoal* goal) {
        if (goal == nullptr) {
            return false;
        }
        if (_activeGoals.empty()) {
            return false;
        }
        vectorImpl<GOAPGoal* >::const_iterator it;
        it = std::find_if(_activeGoals.begin(), _activeGoals.end(), [goal](GOAPGoal const* actGoal) { 
                                                            return actGoal->getName().compare(goal->getName()) == 0; 
                                                        });
        if (it == _activeGoals.end()) {
            return false;
        }

        _activeGoals.erase(it);
        return true;
    }

    inline bool replanGoal() {
        if (_activeGoal != nullptr) {
            if (_activeGoal->plan(_worldState, _actionSet) ) {
                popActiveGoal(_activeGoal);
                advanceGoal();
                return true;
            }  else {
                invalidateCurrentPlan();
            }
        }
        return false;
    }

    inline bool advanceGoal() {
        if (_activeGoal == nullptr) {
            return false;
        }
        const GOAPPlan& plan = _activeGoal->getCurrentPlan();
        if (!plan.empty()) {
            _currentStep++;
            if (!performAction(getActiveAction())) {
                invalidateCurrentPlan();
                return false;
            }
            return true;
        }
        return false;
    }

    inline void invalidateCurrentPlan() { 
        _activeGoal = nullptr;   
        _currentStep = -1;
    }

    inline const GOAPAction* getActiveAction() const { 
        if (_activeGoal == nullptr) {
            return nullptr;
        }
        const GOAPPlan& plan = _activeGoal->getCurrentPlan();
        if (_currentStep >= plan.size()) {
            return nullptr;
        }
        return plan[_currentStep]; 
    }

    inline GOAPGoal* const getActiveGoal() { return _activeGoal; }

    virtual bool performActionStep(GOAPAction::operationsIterator step) = 0;
    virtual bool performAction(const GOAPAction* planStep) = 0;
    virtual void processData(const U64 deltaTime) = 0;
    virtual void processInput(const U64 deltaTime) = 0;
    virtual void update(const U64 deltaTime, NPC* unitRef = nullptr) = 0;
    virtual void processMessage(AIEntity* sender, AIMsg msg, const cdiggins::any& msg_content) = 0;
    virtual void init() = 0;

protected:
    AIEntity*  _entity;
 
private:
    I32 _currentStep;
    GOAPGoal* _activeGoal;
    GOAPActionSet   _actionSet;
    GOAPWorldState  _worldState;

    vectorImpl<GOAPGoal >  _goals;
    vectorImpl<GOAPGoal* > _activeGoals;
};

};

#endif