#include "Headers/SingleShadowMap.h"

#include "Scenes/Headers/SceneState.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Rendering/Headers/Renderer.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

SingleShadowMap::SingleShadowMap(Light* light, Camera* shadowCamera)
    : ShadowMap(light, shadowCamera, ShadowType::SINGLE) {
    Console::printfn(Locale::get(_ID("LIGHT_CREATE_SHADOW_FB")), light->getGUID(),
                     "Single Shadow Map");
    ResourceDescriptor shadowPreviewShader("fbPreview.Single.LinearDepth");
    shadowPreviewShader.setThreadedLoading(false);
    _previewDepthMapShader = CreateResource<ShaderProgram>(shadowPreviewShader);

}

SingleShadowMap::~SingleShadowMap()
{ 
}

void SingleShadowMap::init(ShadowMapInfo* const smi) {
    _init = true;
}


void SingleShadowMap::render(U32 passIdx) {
    _shadowCamera->lookAt(_light->getPosition(), _light->getSpotDirection() + _light->getPosition());
    _shadowCamera->setProjection(1.0f, 90.0f, vec2<F32>(1.0, _light->getRange()));

    RenderPassManager& passMgr = RenderPassManager::instance();
    RenderPassManager::PassParams params;
    params.doPrePass = false;
    params.occlusionCull = false;
    params.camera = _shadowCamera;
    params.stage = RenderStage::SHADOW;
    params.target = RenderTargetID(RenderTargetUsage::SHADOW, to_uint(getShadowMapType()));
    params.drawPolicy = &RenderTarget::defaultPolicy();
    params.pass = passIdx;

    passMgr.doCustomPass(params);
}

void SingleShadowMap::previewShadowMaps(U32 rowIndex) {
    if (_previewDepthMapShader->getState() != ResourceState::RES_LOADED) {
        return;
    }

    const vec4<I32> viewport = getViewportForRow(rowIndex);

    getDepthMap().bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Depth, 0);
    _previewDepthMapShader->Uniform("layer", _arrayOffset);

    GFX::ScopedViewport sViewport(viewport);

    GenericDrawCommand triangleCmd;
    triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
    triangleCmd.drawCount(1);
    triangleCmd.stateHash(GFXDevice::instance().getDefaultStateBlock(true));
    triangleCmd.shaderProgram(_previewDepthMapShader);

    GFXDevice::instance().draw(triangleCmd);
}

};