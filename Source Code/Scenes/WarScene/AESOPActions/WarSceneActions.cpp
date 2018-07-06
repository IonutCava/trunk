#include "Headers/WarSceneActions.h"

#include "Core/Headers/Console.h"
#include "Scenes/WarScene/Headers/WarSceneAISceneImpl.h"

namespace Divide {
namespace AI {

WarSceneAction::WarSceneAction(ActionType type, const stringImpl& name,
                               F32 cost)
    : GOAPAction(stringAlg::fromBase(name), cost)
{
    _type = type;
}

WarSceneAction::~WarSceneAction()
{
}

bool WarSceneAction::preAction(WarSceneAISceneImpl& parentScene) const {
    return Attorney::WarAISceneWarAction::preAction(parentScene, _type, this);
}

bool WarSceneAction::postAction(WarSceneAISceneImpl& parentScene) const {
    return Attorney::WarAISceneWarAction::postAction(parentScene, _type, this);
}

Idle::Idle(const stringImpl& name, F32 cost)
    : WarSceneAction(ActionType::ACTION_IDLE, name, cost)
{
}

ApproachFlag::ApproachFlag(const stringImpl& name, F32 cost)
    : WarSceneAction(ActionType::ACTION_APPROACH_FLAG, name, cost)
{
}

CaptureFlag::CaptureFlag(const stringImpl& name, F32 cost)
    : WarSceneAction(ActionType::ACTION_CAPTURE_FLAG, name, cost)
{
}

ReturnFlagHome::ReturnFlagHome(const stringImpl& name, F32 cost)
    : WarSceneAction(ActionType::ACTION_RETURN_FLAG_TO_BASE, name, cost)
{
}

ScoreFlag::ScoreFlag(const stringImpl& name, F32 cost)
    : WarSceneAction(ActionType::ACTION_SCORE_FLAG, name, cost)
{
}

AttackEnemy::AttackEnemy(const stringImpl& name, F32 cost)
    : WarSceneAction(ActionType::ACTION_ATTACK_ENEMY, name, cost)
{
}

};  // namespace AI
};  // namespace Divide