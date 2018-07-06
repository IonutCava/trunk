#include "Headers/GOAPInterface.h"

namespace Divide {
namespace AI {

const char* GOAPFactName(GOAPFact fact) {
    return to_stringImpl(fact).c_str();
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
    std::reverse(std::begin(_currentPlan), std::end(_currentPlan));
    
    return !_currentPlan.empty();
}

const GOAPPlan& GOAPGoal::getCurrentPlan() const {
    return _currentPlan;
}

};  // namespace AI
};  // namespace Divide
