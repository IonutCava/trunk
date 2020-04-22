#include "stdafx.h"

#include "Headers/Player.h"

#include "Core/Headers/StringHelper.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"

namespace Divide {

Player::Player(U8 index, FrameListenerManager& parent, U32 callOrder)
    : Character(Character::CharacterType::CHARACTER_TYPE_PLAYER, parent, callOrder),
      _index(index)
{
     _lockedControls = false;

     _playerCam = Camera::createCamera<FreeFlyCamera>(Util::StringFormat("Player_Cam_%d", getGUID()));
}

Player::~Player()
{
    Camera::destroyCamera(_playerCam);
}

FreeFlyCamera& Player::getCamera() {
    return *_playerCam;
}

const FreeFlyCamera& Player::getCamera() const {
    return *_playerCam;
}

void Player::setParentNode(SceneGraphNode* node) {
    Character::setParentNode(node);
    if (node != nullptr) {
        BoundingBox bb;
        bb.setMin(-0.5f);
        bb.setMax(0.5f);
        Attorney::SceneNodePlayer::setBounds(node->getNode(), bb);
    }
}

};