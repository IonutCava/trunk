#include "Headers/Player.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Rendering/Camera/Headers/Camera.h"

namespace Divide {

Player::Player(U8 index)
    : Character(Character::CharacterType::CHARACTER_TYPE_PLAYER),
      _index(index)
{
     _lockedControls = false;

     _playerCam = Camera::createCamera(Util::StringFormat("Player_%d_Cam", getGUID()), Camera::CameraType::FREE_FLY);
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