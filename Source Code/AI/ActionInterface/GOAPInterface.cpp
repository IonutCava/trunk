#include "Headers/GOAPInterface.h"

#include "Core\Math\Headers\MathHelper.h"
#include "AI/ActionInterface/Headers/AISceneImpl.h"

namespace Divide {
namespace AI {

const char* GOAPFactName(GOAPFact fact) {
    return std::to_string(fact).c_str();
}

GOAPGoal::GOAPGoal(const std::string& name, U32 ID)
    : goap::WorldState(), _relevancy(0.0f)
{
    _ID = ID;
    name_ = name;
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