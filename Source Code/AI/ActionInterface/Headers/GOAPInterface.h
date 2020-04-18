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
#ifndef _AESOP_ACTION_INTERFACE_H_
#define _AESOP_ACTION_INTERFACE_H_

#include "Platform/Headers/PlatformDefines.h"

#include <CPPGoap/Planner.h>
#include <CPPGoap/Action.h>
#include <CPPGoap/WorldState.h>

namespace Divide {
namespace AI {

using GOAPFact = I32;
using GOAPValue = bool;
using GOAPAction = goap::Action;
using GOAPWorldState = goap::WorldState;
using GOAPActionSet = vectorEASTL<const GOAPAction*>;
using GOAPPlan = vectorEASTL<const GOAPAction*>;

inline const char* GOAPValueName(GOAPValue val) {
    return val ? "true" : "false";
}

const char* GOAPFactName(GOAPFact fact);

class GOAPGoal : public goap::WorldState {
   public:
    GOAPGoal(const stringImpl& name, U32 ID);
    virtual ~GOAPGoal();

    inline F32 relevancy() const { return _relevancy; }
    inline void relevancy(F32 relevancy) { _relevancy = relevancy; }

    const stringImpl& name() const { return name_; }
    U32 getID() const { return _ID; }
    virtual bool plan(const GOAPWorldState& worldState,
                      const GOAPActionSet& actionSet);

    const GOAPPlan& getCurrentPlan() const;

    stringImpl getOpenList() const {
        stringImpl ret;
        _planner.printOpenList(ret);
        return ret;
    }

    stringImpl getClosedList() const {
        stringImpl ret;
        _planner.printClosedList(ret);
        return ret;
    }
   protected:
    U32 _ID;
    F32 _relevancy;
    goap::Planner _planner;
    GOAPPlan _currentPlan;
};

using GOAPGoalList = vectorEASTL<GOAPGoal>;

}; // namespace AI
};  // namespace Divide

#endif
