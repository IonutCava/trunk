#include "Headers/DefaultScene.h"

namespace Divide {
DefaultScene::DefaultScene() 
    : Scene("DefaultScene")
{
}

bool DefaultScene::load(const stringImpl& name, GUI* const gui) {
    return true;
}

bool DefaultScene::loadResources(bool continueOnErrors) {
    return true;
}

void DefaultScene::processInput(const U64 deltaTime) {
}

void DefaultScene::processTasks(const U64 deltaTime) {
}

};
