#include "Headers/NPC.h"
#include "AI/Headers/AIEntity.h"

NPC::NPC(SceneGraphNode* const node, Navigation::DivideDtCrowd* detourCrowd) : Character(Character::CHARACTER_TYPE_NPC, node, detourCrowd), _aiUnit(NULL)
{
}

NPC::NPC(AIEntity* const aiEntity, Navigation::DivideDtCrowd* detourCrowd) : Character(Character::CHARACTER_TYPE_NPC, aiEntity->getBoundNode(), detourCrowd), _aiUnit(aiEntity)
{
	aiEntity->addUnitRef(this);
}

NPC::~NPC()
{
}