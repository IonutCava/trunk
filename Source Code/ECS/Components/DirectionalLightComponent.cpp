#include "stdafx.h"

#include "Headers/DirectionalLightComponent.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "ECS/Components/Headers/TransformComponent.h"

namespace Divide {

namespace {
    constexpr F32 g_defaultLightDistance = 500.0f;
};

DirectionalLightComponent::DirectionalLightComponent(SceneGraphNode& sgn, PlatformContext& context)
    : BaseComponentType<DirectionalLightComponent, ComponentType::DIRECTIONAL_LIGHT>(sgn, context), 
      Light(sgn, -1, LightType::DIRECTIONAL, sgn.sceneGraph().parentScene().lightPool())
{
    Light::range(g_defaultLightDistance);

    _shadowProperties._lightDetails.y = to_F32(_csmSplitCount);
    _shadowProperties._lightDetails.z = 0.0f;
    csmSplitCount(context.config().rendering.shadowMapping.csm.splitCount);

    EditorComponentField directionField = {};
    directionField._name = "Direction";
    directionField._dataGetter = [this](void* dataOut) { static_cast<vec3<F32>*>(dataOut)->set(directionCache()); };
    directionField._dataSetter = [this](const void* data) { setDirection(*static_cast<const vec3<F32>*>(data)); };
    directionField._type = EditorComponentFieldType::PUSH_TYPE;
    directionField._readOnly = true;
    directionField._basicType = GFX::PushConstantType::VEC3;

    getEditorComponent().registerField(std::move(directionField));

    EditorComponentField sceneFitField = {};
    sceneFitField._name = "Fit CSM To AABB";
    sceneFitField._data = &_csmUseSceneAABBFit;
    sceneFitField._type = EditorComponentFieldType::PUSH_TYPE;
    sceneFitField._readOnly = false;
    sceneFitField._basicType = GFX::PushConstantType::BOOL;

    getEditorComponent().registerField(std::move(sceneFitField));

    EditorComponentField csmNearClipField = {};
    csmNearClipField._name = "CSM Near Clip Offset";
    csmNearClipField._data = &_csmNearClipOffset;
    csmNearClipField._range = { -g_defaultLightDistance, g_defaultLightDistance };
    csmNearClipField._type = EditorComponentFieldType::PUSH_TYPE;
    csmNearClipField._readOnly = false;
    csmNearClipField._basicType = GFX::PushConstantType::FLOAT;

    getEditorComponent().registerField(std::move(csmNearClipField));

    EditorComponentField showConeField = {};
    showConeField._name = "Show direction cone";
    showConeField._data = &_showDirectionCone;
    showConeField._type = EditorComponentFieldType::PUSH_TYPE;
    showConeField._readOnly = false;
    showConeField._basicType = GFX::PushConstantType::BOOL;

    getEditorComponent().registerField(std::move(showConeField));

    registerFields(getEditorComponent());

    BoundingBox bb = {};
    bb.setMin(-g_defaultLightDistance * 0.5f);
    bb.setMax(-g_defaultLightDistance * 0.5f);
    Attorney::SceneNodeLightComponent::setBounds(sgn.getNode(), bb);

    _positionCache.set(VECTOR3_ZERO);
    _feedbackContainers.resize(csmSplitCount());
}

void DirectionalLightComponent::PreUpdate(const U64 deltaTime) {
    using Parent = BaseComponentType<DirectionalLightComponent, ComponentType::DIRECTIONAL_LIGHT>;

    if (drawImpostor() || showDirectionCone()) {
        const F32 coneDist = 11.f;
        const Camera* playerCam = getSGN().sceneGraph().parentScene().playerCamera();
        // Try and place the cone in such a way that it's always in view, because directional lights have no "source"
        const vec3<F32> min = (-coneDist * directionCache()) + 
                              playerCam->getEye() + 
                             (playerCam->getForwardDir() * 10.0f) + 
                             (playerCam->getRightDir() * 2.0f);

        context().gfx().debugDrawCone(min, directionCache(), coneDist, 1.f, getDiffuseColour());
    }

    Parent::PreUpdate(deltaTime);
}

void DirectionalLightComponent::OnData(const ECS::CustomEvent& data) {
    if (data._type == ECS::CustomEvent::Type::TransformUpdated) {
        Light::updateCache(data);
    } else if (data._type == ECS::CustomEvent::Type::EntityFlagChanged) {
        const SceneGraphNode::Flags flag = static_cast<SceneGraphNode::Flags>(std::any_cast<U32>(data._flag));
        if (flag == SceneGraphNode::Flags::SELECTED) {
            const auto [state, recursive] = std::any_cast<std::pair<bool, bool>>(data._userData);
            drawImpostor(state);
        }
    }
}

void DirectionalLightComponent::setDirection(const vec3<F32>& direction) {
    TransformComponent* tComp = _parentSGN.get<TransformComponent>();
    if (tComp) {
        Quaternion<F32> rot = tComp->getOrientation();
        rot.fromRotation(directionCache(), direction, tComp->getUpVector());
        tComp->setRotation(rot);
    }
}

};