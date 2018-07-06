#include "stdafx.h"

#include "Headers/SingleShadowMap.h"

#include "Core/Headers/StringHelper.h"
#include "Scenes/Headers/SceneState.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Rendering/Headers/Renderer.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

SingleShadowMap::SingleShadowMap(GFXDevice& context, Light* light, const ShadowCameraPool& shadowCameras)
    : ShadowMap(context, light, shadowCameras, ShadowType::SINGLE) {
    Console::printfn(Locale::get(_ID("LIGHT_CREATE_SHADOW_FB")), light->getGUID(),
                     "Single Shadow Map");
    ResourceDescriptor shadowPreviewShader("fbPreview.Single.LinearDepth");
    shadowPreviewShader.setThreadedLoading(false);
    _previewDepthMapShader = CreateResource<ShaderProgram>(light->parentResourceCache(), shadowPreviewShader);

    GFXDevice::DebugView_ptr shadow = std::make_shared<GFXDevice::DebugView>();
    shadow->_texture = getDepthMap().getAttachment(RTAttachmentType::Depth, 0).texture();
    shadow->_shader = _previewDepthMapShader;
    shadow->_shaderData.set("layer", GFX::PushConstantType::INT, _arrayOffset);
    shadow->_shaderData.set("useScenePlanes", GFX::PushConstantType::BOOL, false);
    shadow->_name = Util::StringFormat("SM_%d", _arrayOffset);
    context.addDebugView(shadow);
}

SingleShadowMap::~SingleShadowMap()
{ 
}

void SingleShadowMap::init(ShadowMapInfo* const smi) {
    _init = true;
}


void SingleShadowMap::render(U32 passIdx, GFX::CommandBuffer& bufferInOut) {
    _shadowCameras[0]->lookAt(_light->getPosition(), _light->getSpotDirection());
    _shadowCameras[0]->setProjection(1.0f, 90.0f, vec2<F32>(1.0, _light->getRange()));

    RenderPassManager& passMgr = _context.parent().renderPassManager();
    RenderPassManager::PassParams params;
    params._doPrePass = false;
    params._occlusionCull = false;
    params._camera = _shadowCameras[0];
    params._stage = RenderStage::SHADOW;
    params._target = RenderTargetID(RenderTargetUsage::SHADOW, to_U32(getShadowMapType()));
    params._drawPolicy = &RenderTarget::defaultPolicy();
    params._passIndex = passIdx;

    passMgr.doCustomPass(params, bufferInOut);
}

};