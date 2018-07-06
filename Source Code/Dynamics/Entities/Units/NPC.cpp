#include "Headers/NPC.h"
#include "AI/Headers/AIEntity.h"

namespace Divide {

NPC::NPC(SceneGraphNode& node, AI::AIEntity* const aiEntity)
    : Character(Character::CharacterType::CHARACTER_TYPE_NPC, node), _aiUnit(aiEntity) {
    if (_aiUnit) {
        assert(!_aiUnit->getUnitRef());
        _aiUnit->addUnitRef(this);
    }
}

NPC::~NPC() {}

void NPC::update(const U64 deltaTime) { Character::update(deltaTime); }
};