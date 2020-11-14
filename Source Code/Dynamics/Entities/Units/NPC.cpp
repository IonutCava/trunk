#include "stdafx.h"

#include "Headers/NPC.h"
#include "AI/Headers/AIEntity.h"

namespace Divide {

NPC::NPC(AI::AIEntity* const aiEntity, FrameListenerManager& parent, U32 callOrder)
    : Character(CharacterType::CHARACTER_TYPE_NPC, parent, callOrder),
      _aiUnit(aiEntity)
{
    if (_aiUnit) {
        assert(!_aiUnit->getUnitRef());
        _aiUnit->addUnitRef(this);
    }
}

void NPC::update(const U64 deltaTimeUS) {
    Character::update(deltaTimeUS);
}

AI::AIEntity* NPC::getAIEntity() const {
    return _aiUnit;
}

}