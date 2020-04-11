#include "stdafx.h"

#include "Headers/SGNComponent.h"
#include "Graphs/Headers/SceneGraph.h"

#include "ECS/Components/Headers/AnimationComponent.h"
#include "ECS/Components/Headers/BoundsComponent.h"
#include "ECS/Components/Headers/DirectionalLightComponent.h"
#include "ECS/Components/Headers/IKComponent.h"
#include "ECS/Components/Headers/NavigationComponent.h"
#include "ECS/Components/Headers/NetworkingComponent.h"
#include "ECS/Components/Headers/PointLightComponent.h"
#include "ECS/Components/Headers/RagdollComponent.h"
#include "ECS/Components/Headers/RenderingComponent.h"
#include "ECS/Components/Headers/RigidBodyComponent.h"
#include "ECS/Components/Headers/ScriptComponent.h"
#include "ECS/Components/Headers/SelectionComponent.h"
#include "ECS/Components/Headers/SpotLightComponent.h"
#include "ECS/Components/Headers/TransformComponent.h"
#include "ECS/Components/Headers/UnitComponent.h"


namespace Divide {
    SGNComponent::SGNComponent(Key key, ComponentType type, SceneGraphNode& parentSGN, PlatformContext& context)
        : ECS::Event::IEventListener(parentSGN.sceneGraph().GetECSEngine()),
          PlatformContextComponent(context),
          _type(type),
          _parentSGN(parentSGN),
          _editorComponent(type, type._to_string())
    {
        ACKNOWLEDGE_UNUSED(key);
        std::atomic_init(&_enabled, true);
        std::atomic_init(&_hasChanged, false);

        RegisterEventCallbacks();
    }

    SGNComponent::~SGNComponent() {
        UnregisterAllEventCallbacks();
    }

    void SGNComponent::RegisterEventCallbacks() {
    }

    bool SGNComponent::saveCache(ByteBuffer& outputBuffer) const {
        ACKNOWLEDGE_UNUSED(outputBuffer);
        return true;
    }

    bool SGNComponent::loadCache(ByteBuffer& inputBuffer) {
        ACKNOWLEDGE_UNUSED(inputBuffer);
        return true;
    }

    I64 SGNComponent::uniqueID() const {
        return _ID((_parentSGN.name() + "_" + _editorComponent.name().c_str()).c_str());
    }

    void SGNComponent::PreUpdate(const U64 deltaTime) {
        OPTICK_EVENT();
        ACKNOWLEDGE_UNUSED(deltaTime);
    }

    bool SGNComponent::enabled() const {
        return _enabled;
    }

    void SGNComponent::enabled(const bool state) {
        _enabled = state;
    }

    void SGNComponent::Update(const U64 deltaTime) {
        OPTICK_EVENT();
        ACKNOWLEDGE_UNUSED(deltaTime);
    }

    void SGNComponent::PostUpdate(const U64 deltaTime) {
        OPTICK_EVENT();
        ACKNOWLEDGE_UNUSED(deltaTime);
    }

    void SGNComponent::OnUpdateLoop() {

    }

    void SGNComponent::OnData(const ECS::Data& data) {
        ACKNOWLEDGE_UNUSED(data);
    }

}; //namespace Divide