#include "stdafx.h"

#include "Headers/GOAPInterface.h"
#include "Core/Headers/StringHelper.h"

namespace Divide {
namespace AI {

const char* GOAPFactName(GOAPFact fact) {
    return Util::to_string(fact).c_str();
}

GOAPGoal::GOAPGoal(const stringImpl& name, U32 ID)
    : goap::WorldState(), _relevancy(0.0f)
{
    _ID = ID;
    name_ = name.c_str();
}

GOAPGoal::~GOAPGoal()
{
}

bool GOAPGoal::plan(const GOAPWorldState& worldState,
                    const GOAPActionSet& actionSet)
{
    _currentPlan = _planner.plan(worldState, *this, actionSet);
    eastl::reverse(eastl::begin(_currentPlan), eastl::end(_currentPlan));
    
    return !_currentPlan.empty();
}

const GOAPPlan& GOAPGoal::getCurrentPlan() const {
    return _currentPlan;
}

};  // namespace AI
};  // namespace Divide
