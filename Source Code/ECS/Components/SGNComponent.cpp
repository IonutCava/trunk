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
    SGNComponent::SGNComponent(Key key, ComponentType type, SceneGraphNode* parentSGN, PlatformContext& context)
        : PlatformContextComponent(context),
          _type(type),
          _parentSGN(parentSGN),
          _editorComponent(type, type._to_string())
    {
        ACKNOWLEDGE_UNUSED(key);
        std::atomic_init(&_enabled, true);
        std::atomic_init(&_hasChanged, false);
    }

    SGNComponent::~SGNComponent()
    {
    }

    bool SGNComponent::saveCache(ByteBuffer& outputBuffer) const {
        outputBuffer << uniqueID();
        return true;
    }

    bool SGNComponent::loadCache(ByteBuffer& inputBuffer) {
        U64 tempID = 0u;
        inputBuffer >> tempID;
        if (tempID != uniqueID()) {
            // corrupt save
            return false;
        }

        return true;
    }

    U64 SGNComponent::uniqueID() const {
        return _ID(Util::StringFormat("%s_%s", _parentSGN->name().c_str(), _editorComponent.name().c_str()).c_str());
    }

    void SGNComponent::PreUpdate(const U64 deltaTime) {
        ACKNOWLEDGE_UNUSED(deltaTime);
    }

    bool SGNComponent::enabled() const {
        return _enabled;
    }

    void SGNComponent::enabled(const bool state) {
        _enabled = state;
    }

    void SGNComponent::Update(const U64 deltaTime) {
        ACKNOWLEDGE_UNUSED(deltaTime);
    }

    void SGNComponent::PostUpdate(const U64 deltaTime) {
        ACKNOWLEDGE_UNUSED(deltaTime);
    }

    void SGNComponent::OnData(const ECS::CustomEvent& data) {
        ACKNOWLEDGE_UNUSED(data);
    }

}; //namespace Divide