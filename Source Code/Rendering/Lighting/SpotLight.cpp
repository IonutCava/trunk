#include "stdafx.h"

#include "Headers/SpotLight.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "ShadowMapping/Headers/ShadowMap.h"
#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

SpotLight::SpotLight(ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, F32 range, LightPool& parentPool)
     : Light(parentCache, descriptorHash, name, range, LightType::SPOT, parentPool),
      _debugView(nullptr)
{
    setRange(2.0f);
    setSpotAngle(35.0f);
    setSpotCosOuterConeAngle(0.65f); // 49.5 degrees
    _spotProperties.xyz(WORLD_Z_NEG_AXIS);
}


void SpotLight::initDebugViews(GFXDevice& context) {
    ResourceDescriptor shadowPreviewShader("fbPreview.Single.LinearDepth");
    shadowPreviewShader.setThreadedLoading(false);
    ShaderProgram_ptr previewDepthMapShader = CreateResource<ShaderProgram>(parentResourceCache(), shadowPreviewShader);

    DebugView_ptr shadow = std::make_shared<DebugView>();
    shadow->_texture = ShadowMap::getDepthMap(LightType::SPOT)._rt->getAttachment(RTAttachmentType::Depth, 0).texture();
    shadow->_shader = previewDepthMapShader;
    shadow->_shaderData.set("layer", GFX::PushConstantType::INT, getShadowOffset());
    shadow->_shaderData.set("useScenePlanes", GFX::PushConstantType::BOOL, false);
    shadow->_shaderData.set("zPlanes", GFX::PushConstantType::VEC2, shadowCameras()[0]->getZPlanes());
    shadow->_name = Util::StringFormat("SM_%d", getShadowOffset());
    _debugView = context.addDebugView(shadow);
}

void SpotLight::updateDebugViews(bool state, U32 arrayOffset) {
    _debugView->_enabled = state;
    if (state) {
        _debugView->_shaderData.set("layer", GFX::PushConstantType::INT, arrayOffset);
        _debugView->_name = Util::StringFormat("SM_%d", arrayOffset);
        _debugView->_shaderData.set("zPlanes", GFX::PushConstantType::VEC2, shadowCameras()[0]->getZPlanes());
    }
}
};