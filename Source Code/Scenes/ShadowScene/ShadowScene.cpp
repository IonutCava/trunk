#include "stdafx.h"

#include "Headers/ShadowScene.h"

namespace Divide {
ShadowScene::ShadowScene(PlatformContext& context, ResourceCache& cache, SceneManager& parent, const stringImpl& name)
    : Scene(context, cache, parent, name)
{
}

bool ShadowScene::load(const stringImpl& name) {
    return Scene::load(name);
}

bool ShadowScene::loadResources(bool continueOnErrors) {
    return Scene::loadResources(continueOnErrors);
}

void ShadowScene::processInput(PlayerIndex idx, const U64 deltaTimeUS) {
    Scene::processInput(idx, deltaTimeUS);
}

void ShadowScene::processTasks(const U64 deltaTimeUS) {
    Scene::processTasks(deltaTimeUS);
}

void ShadowScene::processGUI(const U64 deltaTimeUS) {
    Scene::processGUI(deltaTimeUS);
}

void ShadowScene::onSetActive() {
    Scene::onSetActive();
}

void ShadowScene::onRemoveActive() {
    Scene::onRemoveActive();
}

}; //namespace Divice