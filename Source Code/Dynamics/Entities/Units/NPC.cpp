#include "Headers/NPC.h"
#include "AI/Headers/AIEntity.h"

namespace Divide {

NPC::NPC(SceneGraphNode* const node, AI::AIEntity* const aiEntity)
    : Character(Character::CHARACTER_TYPE_NPC, node), _aiUnit(aiEntity) {
    if (_aiUnit && !_aiUnit->getUnitRef()) {
        _aiUnit->addUnitRef(this);
    }
}

NPC::~NPC() {}

void NPC::update(const U64 deltaTime) { Character::update(deltaTime); }
};