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

#pragma once
#ifndef _AI_PROCESSOR_H_
#define _AI_PROCESSOR_H_

#include "GOAPInterface.h"
#include "AI/Headers/AIEntity.h"

namespace Divide {
class Texture;
namespace AI {

class AIManager;
enum class AIMsg : U8;
/// Provides a scene-level AI implementation
class NOINITVTABLE AIProcessor : private NonCopyable {
   public:
    AIProcessor(AIManager& parentManager);
    virtual ~AIProcessor();
    virtual void addEntityRef(AIEntity* entity);

    inline void worldState(const GOAPWorldState& state) { _worldState = state; }
    inline GOAPWorldState& worldState() { return _worldState; }
    inline const GOAPWorldState& worldState() const { return _worldState; }

    /// Register a specific action.
    /// This only holds a reference to the action itself and does not create a
    /// local copy!
    virtual void registerAction(const GOAPAction& action);
    virtual void registerActionSet(const GOAPActionSet& actionSet);
    /// Register a specific action.
    /// This only holds a reference to the action itself and does not create a
    /// local copy!
    void registerGoal(const GOAPGoal& goal);
    void registerGoalList(const GOAPGoalList& goalList);

    virtual GOAPGoal* findGoal(const stringImpl& goalName) {
        GOAPGoalList::iterator it;
        it = eastl::find_if(eastl::begin(_goals), eastl::end(_goals),
                          [&goalName](const GOAPGoal& goal) -> bool {
                              return goal.name().compare(goalName.c_str()) ==
                                     0;
                          });

        if (it != eastl::end(_goals)) {
            return &(*it);
        }

        return nullptr;
    }

    inline const GOAPGoalList& goalList() const { return _goals; }

   protected:
    friend class AIEntity;
    inline GOAPGoalList& goalList() { return _goals; }

    inline void resetActiveGoals() {
        _activeGoals.clear();
        for (GOAPGoal& goal : goalList()) {
            goal.relevancy(0.0f);
            activateGoal(goal.name());
        }
    }
    /// Although we want the goal to be activated,
    /// it might not be the most relevant in the current scene state
    inline bool activateGoal(const stringImpl& name) {
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

        eastl::sort(eastl::begin(_activeGoals), eastl::end(_activeGoals),
                  [](GOAPGoal const* a, GOAPGoal const* b) {
                      return a->relevancy() < b->relevancy();
                  });

        _activeGoal = _activeGoals.back();
        _currentStep = -1;
        assert(_activeGoal != nullptr);

        return _activeGoal;
    }

    bool popActiveGoal(GOAPGoal* goal) {
        if (goal == nullptr) {
            return false;
        }
        if (_activeGoals.empty()) {
            return false;
        }

        auto it = eastl::find_if(
            eastl::begin(_activeGoals), eastl::end(_activeGoals),
            [goal](GOAPGoal const* actGoal) {
                return actGoal->name().compare(goal->name()) == 0;
            });

        if (it == eastl::end(_activeGoals)) {
            return false;
        }

        _activeGoals.erase(it);
        return true;
    }

    inline bool replanGoal() {
        if (_activeGoal != nullptr) {
            if (_activeGoal->plan(_worldState, _actionSet)) {
                popActiveGoal(_activeGoal);
                advanceGoal();
                return true;
            } else {
                _planLog = "Plan Log: \n";
                _planLog.append("\t OpenList: \n");
                _planLog.append(_activeGoal->getOpenList().c_str());
                _planLog.append("\t ClosedList: \n");
                _planLog.append(_activeGoal->getClosedList().c_str());
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
            const GOAPAction* crtAction = getActiveAction();
            if (!crtAction || !performAction(*crtAction)) {
                invalidateCurrentPlan();
                return false;
            }
            return true;
        }
        return false;
    }

    inline stringImpl printPlan() {
        if (_activeGoal == nullptr) {
            return "no active goal!";
        }
        stringImpl returnString("");
        const GOAPPlan& plan = _activeGoal->getCurrentPlan();
        for (const GOAPAction* action : plan) {
            returnString.append(" - " + printActionStats(*action) + "\n");
        }

        return returnString.c_str();
    }

    virtual void invalidateCurrentPlan() {
        _activeGoal = nullptr;
        _currentStep = -1;
    }

    inline const GOAPAction* getActiveAction() const {
        assert(_activeGoal != nullptr);
        const GOAPPlan& plan = _activeGoal->getCurrentPlan();
        if (to_U32(_currentStep) >= plan.size()) {
            return nullptr;
        }
        return plan[_currentStep];
    }

    inline GOAPGoal* const getActiveGoal() const { return _activeGoal; }

    inline const stringImpl& getPlanLog() const { return _planLog; }

    virtual const stringImpl& printActionStats(const GOAPAction& planStep) const;
    virtual bool performActionStep(GOAPAction::operationsIterator step) = 0;
    virtual bool performAction(const GOAPAction& planStep) = 0;
    virtual bool processData(U64 deltaTimeUS) = 0;
    virtual bool processInput(U64 deltaTimeUS) = 0;
    virtual bool update(U64 deltaTimeUS, NPC* unitRef = nullptr) = 0;
    virtual void processMessage(AIEntity& sender, AIMsg msg, const std::any& msg_content) = 0;
    void init() {
        if (_init) {
            return;
        }
        initInternal();
        _init = true;
    }

    virtual void initInternal() = 0;
    
    virtual stringImpl toString(bool state = false) const = 0;

   protected:
    AIEntity* _entity;
    AIManager& _parentManager;

   private:
    I32 _currentStep;
    GOAPGoal* _activeGoal;
    GOAPActionSet _actionSet;
    GOAPWorldState _worldState;

    GOAPGoalList _goals;
    vectorEASTL<GOAPGoal*> _activeGoals;
    std::atomic_bool _init;

    stringImpl _planLog;
};

};  // namespace AI
};  // namespace Divide

#endif //_AI_PROCESSOR_H_