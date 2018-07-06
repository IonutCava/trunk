#include "Headers/WarSceneActions.h"

#include "Core/Headers/Console.h"
#include "Scenes/WarScene/Headers/WarSceneAISceneImpl.h"

namespace Divide {
namespace AI {

    WarSceneAction::WarSceneAction(ActionType type, const stringImpl& name, F32 cost) : GOAPAction(stringAlg::fromBase(name), cost)
    {
        _type = type;
        _parentScene = nullptr;
    }

    WarSceneAction::~WarSceneAction()
    {
    }

    bool WarSceneAction::preAction() const {
        return _parentScene->preAction(_type, this);
    }

    bool WarSceneAction::postAction() const {
        return _parentScene->postAction(_type, this);
    }

	ApproachFlag::ApproachFlag(const stringImpl& name, F32 cost) : WarSceneAction(ACTION_APPROACH_FLAG, name, cost)
    {
    }

	CaptureFlag::CaptureFlag(const stringImpl& name, F32 cost) : WarSceneAction(ACTION_CAPTURE_FLAG, name, cost)
    {
    }

	ReturnFlag::ReturnFlag(const stringImpl& name, F32 cost) : WarSceneAction(ACTION_RETURN_FLAG, name, cost)
    {
    }

	ProtectFlagCarrier::ProtectFlagCarrier(const stringImpl& name, F32 cost) : WarSceneAction(ACTION_PROTECT_FLAG_CARRIER, name, cost)
    {
    }

	RecoverFlag::RecoverFlag(const stringImpl& name, F32 cost) : WarSceneAction(ACTION_RECOVER_FLAG, name, cost)
    {
    }

	KillEnemy::KillEnemy(const stringImpl& name, F32 cost) : WarSceneAction(ACTION_KILL_ENEMY, name, cost)
	{
	}

	ReturnHome::ReturnHome(const stringImpl& name, F32 cost) : WarSceneAction(ACTION_RETURN_TO_BASE, name, cost)
	{
	}
	 
}; //namespace AI
}; //namespace Divide