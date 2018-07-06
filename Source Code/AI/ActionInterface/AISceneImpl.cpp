#include "Headers/AISceneImpl.h"

#include "Platform/Platform/Headers/PlatformDefines.h"

namespace Divide {
namespace AI {

AISceneImpl::AISceneImpl()
    : _entity(nullptr), _activeGoal(nullptr), _currentStep(-1)
{
    _init = false;
}

AISceneImpl::~AISceneImpl()
{
    _actionSet.clear();
    _goals.clear();
}

void AISceneImpl::addEntityRef(AIEntity* entity) {
    if (entity) {
        _entity = entity;
    }
}

void AISceneImpl::registerAction(const GOAPAction& action) {
    _actionSet.push_back(&action);
}

void AISceneImpl::registerActionSet(const GOAPActionSet& actionSet) {
    _actionSet.insert(std::end(_actionSet), std::begin(actionSet),
                      std::end(actionSet));
}

void AISceneImpl::registerGoal(const GOAPGoal& goal) {
    assert(!goal.vars_.empty());
    _goals.push_back(goal);
}

void AISceneImpl::registerGoalList(const GOAPGoalList& goalList) {
    for (const GOAPGoal& goal : goalList) {
        registerGoal(goal);
    }
}

};  // namespace AI
};  // namespace Divide