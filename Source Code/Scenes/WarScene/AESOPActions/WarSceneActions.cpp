#include "Headers/WarSceneActions.h"

#include "Core/Headers/Console.h"
#include "Scenes/WarScene/Headers/WarSceneAISceneImpl.h"

namespace Divide {
namespace AI {

WarSceneAction::WarSceneAction(ActionType type, const stringImpl& name,
                               F32 cost)
    : GOAPAction(name, to_int(cost)),
    _type(type)
{
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

};  // namespace AI
};  // namespace Divide