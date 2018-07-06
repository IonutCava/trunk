#include "Headers/Player.h"

Player::Player(SceneGraphNode* const node, Navigation::DivideDtCrowd* detourCrowd) : Character(Character::CHARACTER_TYPE_PLAYER,node, detourCrowd)
{
	_lockedControls = false;
}

Player::~Player()
{
}