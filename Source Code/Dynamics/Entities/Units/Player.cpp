#include "Headers/Player.h"

Player::Player(SceneGraphNode* const node) : Character(Character::CHARACTER_TYPE_PLAYER,node) 
{
	_lockedControls = false;
}

Player::~Player()
{
}