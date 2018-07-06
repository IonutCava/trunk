#include "Headers/WarSceneActions.h"

#include "Core/Headers/Console.h"
#include "Scenes/WarScene/Headers/WarSceneAISceneImpl.h"

namespace Divide {
namespace AI {

    WarSceneAction::WarSceneAction(ActionType type, const std::string& name, F32 cost) : GOAPAction(name, cost)
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
    ApproachFlag::ApproachFlag(std::string name, F32 cost) : WarSceneAction(ACTION_APPROACH_FLAG, name, cost)
    {
    }

    CaptureFlag::CaptureFlag(std::string name, F32 cost) : WarSceneAction(ACTION_CAPTURE_FLAG, name, cost)
    {
    }

    ReturnFlag::ReturnFlag(std::string name, F32 cost) : WarSceneAction(ACTION_RETURN_FLAG, name, cost)
    {
    }

}; //namespace AI
}; //namespace Divide