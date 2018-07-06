#include "Headers/CubeScene.h"

#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/Headers/DeferredShadingRenderer.h"

#include "Core/Time/Headers/ApplicationTimer.h"
#include "Core/Math/Headers/Transform.h"

namespace Divide {

void CubeScene::processTasks(const U64 deltaTime) {
    D64 updateLights = Time::SecondsToMilliseconds(0.05);

    if (_taskTimers[0] >= updateLights) {
        for (U8 row = 0; row < 3; row++)
            for (U8 col = 0; col < _lightNodes.size() / 3.0f; col++) {
                F32 x = col * 150.0f - 5.0f +
                        cos(to_float(Time::ElapsedMilliseconds()) * (col - row + 2) *
                            0.008f) *
                            200.0f;
                F32 y = cos(to_float(Time::ElapsedSeconds()) * (col - row + 2) * 0.01f) *
                            200.0f +
                        20;
                ;
                F32 z = row * 500.0f - 500.0f -
                        cos(to_float(Time::ElapsedMilliseconds()) * (col - row + 2) *
                            0.009f) *
                            200.0f +
                        10;
                F32 r = 1;
                F32 g = 1.0f - (row / 3.0f);
                F32 b = col / (_lightNodes.size() / 3.0f);

                _lightNodes[row * 10 + col].lock()->getNode<Light>()->setDiffuseColor(vec3<F32>(r, g, b));
                _lightNodes[row * 10 + col].lock()->get<PhysicsComponent>()->setPosition(vec3<F32>(x, y, z));
            }

        _taskTimers[0] = 0.0;
    }
    Scene::processTasks(deltaTime);
}

I8 g_j = 1;
F32 g_i = 0;
bool _switch = false;

void CubeScene::processInput(const U64 deltaTime) {
    if (g_i >= 360)
        _switch = true;
    else if (g_i <= 0)
        _switch = false;

    if (!_switch)
        g_i += 0.1f;
    else
        g_i -= 0.1f;

    g_i >= 180 ? g_j = -1 : g_j = 1;

    SceneGraphNode_ptr cutia1(_sceneGraph.findNode("Cutia1").lock());
    SceneGraphNode_ptr hellotext(_sceneGraph.findNode("HelloText").lock());
    SceneGraphNode_ptr bila(_sceneGraph.findNode("Bila").lock());
    SceneGraphNode_ptr dwarf(_sceneGraph.findNode("dwarf").lock());

    cutia1->get<PhysicsComponent>()->rotate(
        0.3f * g_i, 0.6f * g_i, 0);
    hellotext->get<PhysicsComponent>()->rotate(
        vec3<F32>(0.6f, 0.2f, 0.4f), g_i);
    bila->get<PhysicsComponent>()->translateY(g_j * 0.25f);
    dwarf->get<PhysicsComponent>()->rotate(vec3<F32>(0, 1, 0), g_i);
}

bool CubeScene::load(const stringImpl& name, GUI* const gui) {
    SceneManager::instance().setRenderer(RendererType::RENDERER_DEFERRED_SHADING);
    // Load scene resources
    return SCENE_LOAD(name, gui, true, true);
}

bool CubeScene::loadResources(bool continueOnErrors) {
    // 30 lights? >:)
    static const U32 normalMask = to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                  to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                  to_const_uint(SGNComponent::ComponentType::RENDERING);

    for (U8 row = 0; row < 3; row++) {
        for (U8 col = 0; col < 10; col++) {
            U8 lightID = to_ubyte(row * 10 + col);
            stringstreamImpl ss;
            ss << to_uint(lightID);
            ResourceDescriptor tempLight("Light Deferred " + ss.str());
            tempLight.setEnumValue(to_const_uint(LightType::POINT));
            Light* light = CreateResource<Light>(tempLight);
            light->setDrawImpostor(true);
            light->setRange(30.0f);
            light->setCastShadows(false);
            _lightNodes.push_back(_sceneGraph.getRoot().addNode(*light, normalMask, PhysicsGroup::GROUP_IGNORE));
        }
    }

    _taskTimers.push_back(0.0);

    return true;
}

};