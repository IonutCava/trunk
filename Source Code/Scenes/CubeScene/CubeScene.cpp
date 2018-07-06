#include "stdafx.h"

#include "Headers/CubeScene.h"

#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"
#include "Rendering/Headers/DeferredShadingRenderer.h"

#include "Core/Time/Headers/ApplicationTimer.h"
#include "Core/Math/Headers/Transform.h"
#include "Core/Headers/PlatformContext.h"

namespace Divide {

void CubeScene::processTasks(const U64 deltaTimeUS) {
    D64 updateLights = Time::SecondsToMilliseconds(0.05);

    if (_taskTimers[0] >= updateLights) {
        for (U8 row = 0; row < 3; row++)
            for (U8 col = 0; col < _lightNodes.size() / 3.0f; col++) {
                F32 x = col * 150.0f - 5.0f +
                        cos(to_F32(Time::ElapsedMilliseconds()) * (col - row + 2) *
                            0.008f) *
                            200.0f;
                F32 y = cos(to_F32(Time::ElapsedSeconds()) * (col - row + 2) * 0.01f) *
                            200.0f +
                        20;
                ;
                F32 z = row * 500.0f - 500.0f -
                        cos(to_F32(Time::ElapsedMilliseconds()) * (col - row + 2) *
                            0.009f) *
                            200.0f +
                        10;
                F32 r = 1;
                F32 g = 1.0f - (row / 3.0f);
                F32 b = col / (_lightNodes.size() / 3.0f);

                _lightNodes[row * 10 + col]->getNode<Light>()->setDiffuseColour(vec3<F32>(r, g, b));
                _lightNodes[row * 10 + col]->get<TransformComponent>()->setPosition(vec3<F32>(x, y, z));
            }

        _taskTimers[0] = 0.0;
    }
    Scene::processTasks(deltaTimeUS);
}

I8 g_j = 1;
F32 g_i = 0;
bool _switch = false;

void CubeScene::processInput(PlayerIndex idx, const U64 deltaTimeUS) {
    if (g_i >= 360)
        _switch = true;
    else if (g_i <= 0)
        _switch = false;

    if (!_switch)
        g_i += 0.1f;
    else
        g_i -= 0.1f;

    g_i >= 180 ? g_j = -1 : g_j = 1;

    SceneGraphNode* cutia1(_sceneGraph->findNode("Cutia1"));
    SceneGraphNode* hellotext(_sceneGraph->findNode("HelloText"));
    SceneGraphNode* bila(_sceneGraph->findNode("Bila"));
    SceneGraphNode* dwarf(_sceneGraph->findNode("dwarf"));

    cutia1->get<TransformComponent>()->rotate(
        0.3f * g_i, 0.6f * g_i, 0);
    hellotext->get<TransformComponent>()->rotate(
        vec3<F32>(0.6f, 0.2f, 0.4f), g_i);
    bila->get<TransformComponent>()->translateY(g_j * 0.25f);
    dwarf->get<TransformComponent>()->rotate(vec3<F32>(0, 1, 0), g_i);

    Scene::processInput(idx, deltaTimeUS);
}

bool CubeScene::load(const stringImpl& name) {
    _context.gfx().setRenderer(RendererType::RENDERER_DEFERRED_SHADING);
    // Load scene resources
    return SCENE_LOAD(name, true, true);
}

bool CubeScene::loadResources(bool continueOnErrors) {
    // 30 lights? >:)
    static const U32 normalMask = to_base(SGNComponent::ComponentType::TRANSFORM) |
                                  to_base(SGNComponent::ComponentType::BOUNDS) |
                                  to_base(SGNComponent::ComponentType::RENDERING) |
                                  to_base(SGNComponent::ComponentType::NETWORKING);

    for (U8 row = 0; row < 3; row++) {
        for (U8 col = 0; col < 10; col++) {
            U8 lightID = to_U8(row * 10 + col);
            stringstreamImpl ss;
            ss << to_U32(lightID);
            ResourceDescriptor tempLight("Light Deferred " + ss.str());
            tempLight.setEnumValue(to_base(LightType::POINT));
            tempLight.setUserPtr(_lightPool);

            std::shared_ptr<Light> light = CreateResource<Light>(_resCache, tempLight);
            light->setDrawImpostor(true);
            light->setRange(30.0f);
            light->setCastShadows(false);
            _lightNodes.push_back(_sceneGraph->getRoot().addNode(light, normalMask, PhysicsGroup::GROUP_IGNORE));
        }
    }

    _taskTimers.push_back(0.0);

    return true;
}

};