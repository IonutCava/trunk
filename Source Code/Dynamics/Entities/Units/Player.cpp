#include "Headers/Player.h"

namespace Divide {

Player::Player(SceneGraphNode& node)
    : Character(Character::CharacterType::CHARACTER_TYPE_PLAYER, node) {
    _lockedControls = false;
}

Player::~Player() {}
};