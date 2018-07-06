#include "stdafx.h"

#include "Headers/CubeShadowMap.h"

#include "Core/Headers/StringHelper.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

CubeShadowMap::CubeShadowMap(GFXDevice& context, Light* light, const ShadowCameraPool& shadowCameras)
    : ShadowMap(context, light, shadowCameras, ShadowType::CUBEMAP)
{
    
    Console::printfn(Locale::get(_ID("LIGHT_CREATE_SHADOW_FB")), light->getGUID(), "Single Shadow Map");

    ResourceDescriptor shadowPreviewShader("fbPreview.Cube.LinearDepth.ScenePlanes");
    shadowPreviewShader.setThreadedLoading(false);
    _previewDepthMapShader = CreateResource<ShaderProgram>(light->parentResourceCache(), shadowPreviewShader);

    for (U32 i = 0; i < 6; ++i) {
        DebugView_ptr shadow = std::make_shared<DebugView>();
        shadow->_texture = getDepthMap().getAttachment(RTAttachmentType::Depth, 0).texture();
        shadow->_shader = _previewDepthMapShader;
        shadow->_shaderData.set("layer", GFX::PushConstantType::INT, _arrayOffset);
        shadow->_shaderData.set("face", GFX::PushConstantType::INT, i);
        shadow->_shaderData.set("useScenePlanes", GFX::PushConstantType::BOOL, false);
        shadow->_shaderData.set("zPlanes", GFX::PushConstantType::VEC2, _shadowCameras[0]->getZPlanes());

        shadow->_name = Util::StringFormat("CubeSM_%d_face_%d", _arrayOffset, i);
        context.addDebugView(shadow);
    }
}

CubeShadowMap::~CubeShadowMap()
{
}

void CubeShadowMap::init(ShadowMapInfo* const smi) {
    _init = true;
}

void CubeShadowMap::render(U32 passIdx, GFX::CommandBuffer& bufferInOut) {
    _context.generateCubeMap(getDepthMapID(),
                             _arrayOffset,
                             _light->getPosition(),
                             vec2<F32>(0.1f, _light->getRange()),
                             RenderStagePass(RenderStage::SHADOW, RenderPassType::DEPTH_PASS),
                             passIdx,
                             bufferInOut,
                             _shadowCameras[0]);
}

};