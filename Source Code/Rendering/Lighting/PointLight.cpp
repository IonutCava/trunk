#include "stdafx.h"

#include "Headers/PointLight.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

PointLight::PointLight(ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, F32 range, LightPool& parentPool)
     : Light(parentCache, descriptorHash, name, range, LightType::POINT, parentPool)
{
    // +x
    _direction[0].set(WORLD_X_AXIS);
    // -x
    _direction[1].set(WORLD_X_NEG_AXIS);
    // +z
    _direction[2].set(WORLD_Z_AXIS);
    // -z
    _direction[3].set(WORLD_Z_NEG_AXIS);
    // +y
    _direction[4].set(WORLD_Y_AXIS);
    // -y
    _direction[5].set(WORLD_Y_NEG_AXIS);
}

void PointLight::initDebugViews(GFXDevice& context) {
    _debugViews.resize(6);

    ResourceDescriptor shadowPreviewShader("fbPreview.Cube.LinearDepth.ScenePlanes");
    shadowPreviewShader.setThreadedLoading(false);

    ShaderProgram_ptr previewCubeDepthMapShader = CreateResource<ShaderProgram>(parentResourceCache(), shadowPreviewShader);
    for (U32 i = 0; i < 6; ++i) {
        DebugView_ptr shadow = std::make_shared<DebugView>();
        shadow->_texture = ShadowMap::getDepthMap(LightType::POINT)._rt->getAttachment(RTAttachmentType::Depth, 0).texture();
        shadow->_shader = previewCubeDepthMapShader;
        shadow->_shaderData.set("layer", GFX::PushConstantType::INT, getShadowOffset());
        shadow->_shaderData.set("face", GFX::PushConstantType::INT, i);
        shadow->_shaderData.set("useScenePlanes", GFX::PushConstantType::BOOL, false);
        shadow->_shaderData.set("zPlanes", GFX::PushConstantType::VEC2, shadowCameras()[0]->getZPlanes());

        shadow->_name = Util::StringFormat("CubeSM_%d_face_%d", getShadowOffset(), i);
        _debugViews[i] = context.addDebugView(shadow);
    }
}

void PointLight::updateDebugViews(bool state, U32 arrayOffset) {
    for (U32 i = 0; i < 6; ++i) {
        _debugViews[i]->_enabled = state;
        if (state) {
            _debugViews[i]->_shaderData.set("layer", GFX::PushConstantType::INT, i + arrayOffset);
            _debugViews[i]->_name = Util::StringFormat("CSM_%d", i + arrayOffset);
            _debugViews[i]->_shaderData.set("zPlanes", GFX::PushConstantType::VEC2, shadowCameras()[0]->getZPlanes());
        }
    }
}

};