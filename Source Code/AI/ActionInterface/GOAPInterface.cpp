#include "Headers/GOAPInterface.h"

#include "Core\Math\Headers\MathHelper.h"
#include "AI/ActionInterface/Headers/AISceneImpl.h"

namespace Divide {
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
        return false;
    }
    if (_planner.plan(worldState, *this, actionSet, _currentPlan)) {
        std::reverse(std::begin(_currentPlan), std::end(_currentPlan));
    }
    return !_currentPlan.empty();
}

const GOAPPlan& GOAPGoal::getCurrentPlan() const {
    return _currentPlan;
}

}; //namespace AI
}; //namespace Divide