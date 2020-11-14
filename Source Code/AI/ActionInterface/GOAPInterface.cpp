#include "stdafx.h"

#include "Headers/GOAPInterface.h"

namespace Divide {
namespace AI {

GOAPGoal::GOAPGoal(const stringImpl& name, const U32 ID)
    : WorldState(), _relevancy(0.0f)
{
    _ID = ID;
    name_ = name;
}

bool GOAPGoal::plan(const GOAPWorldState& worldState,
                    const GOAPActionSet& actionSet)
{
    _currentPlan = _planner.plan(worldState, *this, actionSet);
    eastl::reverse(begin(_currentPlan), end(_currentPlan));
    
    return !_currentPlan.empty();
}

const GOAPPlan& GOAPGoal::getCurrentPlan() const {
    return _currentPlan;
}

}  // namespace AI
}  // namespace Divide
