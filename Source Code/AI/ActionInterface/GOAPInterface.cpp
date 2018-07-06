#include "Headers/GOAPInterface.h"

#include "Core\Math\Headers\MathHelper.h"

namespace AI {

const char* GOAPFactName(GOAPFact fact) {
    return Util::toString(fact).c_str();
}

GOAPGoal::GOAPGoal(const std::string& name) : goap::WorldState(),
                                              _relevancy(0.0f)
{
    name_ = name;
}

GOAPGoal::~GOAPGoal()
{
}

bool GOAPGoal::plan(const GOAPWorldState& worldState, const GOAPActionSet& actionSet) {
    _currentPlan.resize(0);
    if (worldState.meetsGoal(*this)) {
        return true;
    }
    if (_planner.plan(worldState, *this, actionSet, _currentPlan)) {
        std::reverse(_currentPlan.begin(), _currentPlan.end());
    }

    return !_currentPlan.empty();
}

const GOAPPlan& GOAPGoal::getCurrentPlan() const {
    return _currentPlan;
}

}; //namespace AI