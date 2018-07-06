#include "Headers/ReflectionScene.h"

namespace Divide {
ReflectionScene::ReflectionScene(const stringImpl& name)
    : Scene(name)
{
}

bool ReflectionScene::load(const stringImpl& name) {
    return Scene::load(name);
}

bool ReflectionScene::loadResources(bool continueOnErrors) {
    return Scene::loadResources(continueOnErrors);
}

void ReflectionScene::processInput(const U64 deltaTime) {
    Scene::processInput(deltaTime);
}

void ReflectionScene::processTasks(const U64 deltaTime) {
    Scene::processTasks(deltaTime);
}

void ReflectionScene::processGUI(const U64 deltaTime) {
    Scene::processGUI(deltaTime);
}

void ReflectionScene::onSetActive() {
    Scene::onSetActive();
}

void ReflectionScene::onRemoveActive() {
    Scene::onRemoveActive();
}

}; //namespace Divice