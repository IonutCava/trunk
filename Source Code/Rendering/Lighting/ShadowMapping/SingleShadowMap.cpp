#include "Headers/SingleShadowMap.h"

#include "Scenes/Headers/SceneState.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Headers/Renderer.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

SingleShadowMap::SingleShadowMap(Light* light, Camera* shadowCamera)
    : ShadowMap(light, shadowCamera, ShadowType::SINGLE) {
    Console::printfn(Locale::get("LIGHT_CREATE_SHADOW_FB"), light->getGUID(),
                     "Single Shadow Map");
    ResourceDescriptor shadowPreviewShader("fbPreview.Layered.LinearDepth");
    shadowPreviewShader.setThreadedLoading(false);
    _previewDepthMapShader = CreateResource<ShaderProgram>(shadowPreviewShader);

}

SingleShadowMap::~SingleShadowMap() { RemoveResource(_previewDepthMapShader); }

void SingleShadowMap::init(ShadowMapInfo* const smi) {
    _init = true;
}


void SingleShadowMap::render(SceneRenderState& renderState) {
    renderState.getCameraMgr().pushActiveCamera(_shadowCamera);
    _shadowCamera->lookAt(_light->getPosition(), _light->getPosition() * _light->getSpotDirection());
    _shadowCamera->setProjection(1.0f, 90.0f, vec2<F32>(1.0, _light->getRange()));
    _shadowCamera->renderLookAt();

    getDepthMap()->begin(Framebuffer::defaultPolicy());
        SceneManager::getInstance().renderVisibleNodes(RenderStage::SHADOW, true);
    getDepthMap()->end();
    renderState.getCameraMgr().popActiveCamera();
}

void SingleShadowMap::previewShadowMaps() {
    getDepthMap()->bind(to_ubyte(ShaderProgram::TextureUsage::UNIT0));
    _previewDepthMapShader->Uniform("layer", _arrayOffset);
    GFX_DEVICE.drawTriangle(GFX_DEVICE.getDefaultStateBlock(true), _previewDepthMapShader);
}
};