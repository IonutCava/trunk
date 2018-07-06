#include "stdafx.h"

#include "Headers/ReflectionScene.h"

namespace Divide {
ReflectionScene::ReflectionScene(PlatformContext& context, ResourceCache& cache, SceneManager& parent, const stringImpl& name)
    : Scene(context, cache, parent, name)
{
}

bool ReflectionScene::load(const stringImpl& name) {
    return Scene::load(name);
}

bool ReflectionScene::loadResources(bool continueOnErrors) {
    return Scene::loadResources(continueOnErrors);
}

void ReflectionScene::processInput(PlayerIndex idx, const U64 deltaTimeUS) {
    Scene::processInput(idx, deltaTimeUS);
}

void ReflectionScene::processTasks(const U64 deltaTimeUS) {
    Scene::processTasks(deltaTimeUS);
}

void ReflectionScene::processGUI(const U64 deltaTimeUS) {
    Scene::processGUI(deltaTimeUS);
}

void ReflectionScene::onSetActive() {
    Scene::onSetActive();
}

void ReflectionScene::onRemoveActive() {
    Scene::onRemoveActive();
}

}; //namespace Divice