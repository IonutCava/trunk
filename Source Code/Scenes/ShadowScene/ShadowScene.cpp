#include "Headers/ShadowScene.h"

namespace Divide {
ShadowScene::ShadowScene(const stringImpl& name)
    : Scene(name)
{
}

bool ShadowScene::load(const stringImpl& name) {
    return Scene::load(name);
}

bool ShadowScene::loadResources(bool continueOnErrors) {
    return Scene::loadResources(continueOnErrors);
}

void ShadowScene::processInput(const U64 deltaTime) {
    Scene::processInput(deltaTime);
}

void ShadowScene::processTasks(const U64 deltaTime) {
    Scene::processTasks(deltaTime);
}

void ShadowScene::processGUI(const U64 deltaTime) {
    Scene::processGUI(deltaTime);
}

void ShadowScene::onSetActive() {
    Scene::onSetActive();
}

void ShadowScene::onRemoveActive() {
    Scene::onRemoveActive();
}

}; //namespace Divice