#include "stdafx.h"

#include "Headers/SGNComponent.h"

namespace Divide {
    SGNComponent::SGNComponent(Key key, ComponentType type, SceneGraphNode& parentSGN, PlatformContext& context)
        : ECS::Event::IEventListener(&parentSGN.GetECSEngine()),
          PlatformContextComponent(context),
          _type(type),
          _parentSGN(parentSGN),
          _editorComponent(type._to_string()),
          _enabled(true),
          _hasChanged(false)
    {
        ACKNOWLEDGE_UNUSED(key);
       
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

    void SGNComponent::OnUpdateLoop() {

    }

}; //namespace Divide