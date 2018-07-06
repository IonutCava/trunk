#include "Headers/WarSceneActions.h"

#include "Core/Headers/Console.h"
#include "Scenes/WarScene/Headers/WarSceneAIProcessor.h"

namespace Divide {
namespace AI {

WarSceneAction::WarSceneAction(ActionType type,
                               const stringImpl& name,
                               F32 cost)
    : GOAPAction(name.c_str(), to_int(cost)),
    _type(type)
{
}

WarSceneAction::~WarSceneAction()
{
}

bool WarSceneAction::preAction(WarSceneAIProcessor& parentProcessor) const {
    return Attorney::WarAISceneWarAction::preAction(parentProcessor, _type, this);
}

bool WarSceneAction::postAction(WarSceneAIProcessor& parentProcessor) const {
    return Attorney::WarAISceneWarAction::postAction(parentProcessor, _type, this);
}

};  // namespace AI
};  // namespace Divide