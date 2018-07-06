#include "Headers/NPC.h"
#include "AI/Headers/AIEntity.h"

NPC::NPC(SceneGraphNode* const node) : Character(Character::CHARACTER_TYPE_NPC, node), _aiUnit(NULL)
{
}

NPC::NPC(AIEntity* const aiEntity) : Character(Character::CHARACTER_TYPE_NPC, aiEntity->getBoundNode()), _aiUnit(aiEntity)
{
	aiEntity->addUnitRef(this);
}

NPC::~NPC()
{
}