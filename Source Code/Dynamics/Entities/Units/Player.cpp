#include "Headers/Player.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Rendering/Camera/Headers/Camera.h"

namespace Divide {

Player::Player(SceneGraphNode_ptr node)
    : Character(Character::CharacterType::CHARACTER_TYPE_PLAYER, node)
{
     _lockedControls = false;

     _playerCam = Camera::createCamera(Util::StringFormat("%s_Cam", node->getName().c_str()),
                                       Camera::CameraType::FREE_FLY);
}

Player::~Player()
{
    Camera::destroyCamera(_playerCam);
}

Camera& Player::getCamera() {
    return *_playerCam;
}

const Camera& Player::getCamera() const {
    return *_playerCam;
}

};