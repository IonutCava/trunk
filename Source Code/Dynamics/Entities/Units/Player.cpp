#include "Headers/Player.h"

namespace Divide {

Player::Player(SceneGraphNode* const node) : Character(Character::CHARACTER_TYPE_PLAYER, node)
{
    _lockedControls = false;
}

Player::~Player()
{
}

};