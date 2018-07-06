#include "Headers/NPC.h"
#include "AI/Headers/AIEntity.h"

NPC::NPC(SceneGraphNode* const node, AIEntity* const aiEntity) : Character(Character::CHARACTER_TYPE_NPC, node), _aiUnit(aiEntity)
{
    if(aiEntity)
        aiEntity->addUnitRef(this);
}

NPC::~NPC()
{
}