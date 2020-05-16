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
     const stringImpl& cameraName = Util::StringFormat("Player_Cam_%d", getGUID());
     _camera = Camera::createCamera<FreeFlyCamera>(cameraName);
}

Player::~Player()
{
    Camera::destroyCamera(_camera);
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