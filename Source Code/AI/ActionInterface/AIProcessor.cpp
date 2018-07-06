#include "stdafx.h"

#include "Headers/AIProcessor.h"

#include "AI/Headers/AIManager.h"
#include "Platform/Headers/PlatformDefines.h"

namespace Divide {
namespace AI {

AIProcessor::AIProcessor(AIManager& parentManager)
    : _parentManager(parentManager),
      _entity(nullptr),
      _activeGoal(nullptr),
      _currentStep(-1)
{
    _init = false;
}

AIProcessor::~AIProcessor()
{
    _actionSet.clear();
    _goals.clear();
}

void AIProcessor::addEntityRef(AIEntity* entity) {
    if (entity) {
        _entity = entity;
    }
}

void AIProcessor::registerAction(const GOAPAction& action) {
    _actionSet.push_back(&action);
}

void AIProcessor::registerActionSet(const GOAPActionSet& actionSet) {
    _actionSet.insert(std::end(_actionSet), std::begin(actionSet),
                      std::end(actionSet));
}

void AIProcessor::registerGoal(const GOAPGoal& goal) {
    assert(!goal.vars_.empty());
    _goals.push_back(goal);
}

void AIProcessor::registerGoalList(const GOAPGoalList& goalList) {
    for (const GOAPGoal& goal : goalList) {
        registerGoal(goal);
    }
}

const std::string& AIProcessor::printActionStats(const GOAPAction& planStep) const {
    static const std::string placeholder("");
    return placeholder;
}

};  // namespace AI
};  // namespace Divide