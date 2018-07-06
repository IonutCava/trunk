#include "stdafx.h"

#include "Headers/DirectionalLight.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "ShadowMapping/Headers/ShadowMap.h"
#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {

DirectionalLight::DirectionalLight(ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, LightPool& parentPool)
    : Light(parentCache, descriptorHash, name, -1, LightType::DIRECTIONAL, parentPool),
      _csmSplitCount(3),
      _csmSplitLogFactor(0.95f),
      _csmNearClipOffset(100.0f)
{
    // Down the Y and Z axis at DIRECTIONAL_LIGHT_DISTANCE units away;
    _positionAndRange.set(0, -1, -1, to_F32(Config::Lighting::DIRECTIONAL_LIGHT_DISTANCE));
    _shadowProperties._lightDetails.y = to_U32(_csmSplitCount);
}

DirectionalLight::~DirectionalLight()
{
}


void DirectionalLight::initDebugViews(GFXDevice& context)
{
    _debugViews.resize(_csmSplitCount);

    ResourceDescriptor shadowPreviewShader("fbPreview.Layered.LinearDepth.ESM.ScenePlanes");
    shadowPreviewShader.setThreadedLoading(false);
    ShaderProgram_ptr previewDepthMapShader = CreateResource<ShaderProgram>(parentResourceCache(), shadowPreviewShader);
    for (U32 i = 0; i < _csmSplitCount; ++i) {
        DebugView_ptr shadow = std::make_shared<DebugView>();
        shadow->_texture = ShadowMap::getDepthMap(LightType::DIRECTIONAL)._rt->getAttachment(RTAttachmentType::Colour, 0).texture();
        shadow->_shader = previewDepthMapShader;
        shadow->_shaderData.set("layer", GFX::PushConstantType::INT, i + getShadowOffset());
        shadow->_shaderData.set("useScenePlanes", GFX::PushConstantType::BOOL, false);
        shadow->_name = Util::StringFormat("CSM_%d", i + getShadowOffset());
        _debugViews[i] = context.addDebugView(shadow);
    }
}

void DirectionalLight::updateDebugViews(bool state, U32 arrayOffset) {
    for (U32 i = 0; i < _csmSplitCount; ++i) {
        _debugViews[i]->_enabled = state;
        if (state) {
            _debugViews[i]->_shaderData.set("layer", GFX::PushConstantType::INT, i + arrayOffset);
            _debugViews[i]->_name = Util::StringFormat("CSM_%d", i + arrayOffset);
        }
    }
}
};