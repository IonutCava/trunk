#include "Headers/SingleShadowMap.h"

#include "Scenes/Headers/SceneState.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
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


void SingleShadowMap::render(SceneRenderState& renderState) {
    _shadowCamera->lookAt(_light->getPosition(), _light->getSpotDirection() + _light->getPosition());
    _shadowCamera->setProjection(1.0f, 90.0f, vec2<F32>(1.0, _light->getRange()));

    renderState.getCameraMgr().pushActiveCamera(_shadowCamera);
    _shadowCamera->renderLookAt();

    getDepthMap()->begin(RenderTarget::defaultPolicy());
        SceneManager::instance().renderVisibleNodes(RenderStage::SHADOW, true);
    getDepthMap()->end();
    renderState.getCameraMgr().popActiveCamera();
}

void SingleShadowMap::previewShadowMaps(U32 rowIndex) {
    if (_previewDepthMapShader->getState() != ResourceState::RES_LOADED) {
        return;
    }

    const vec4<I32> viewport = getViewportForRow(rowIndex);

    getDepthMap()->bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Depth, 0);
    _previewDepthMapShader->Uniform("layer", _arrayOffset);

    GFX::ScopedViewport sViewport(viewport);
    GFX_DEVICE.drawTriangle(GFX_DEVICE.getDefaultStateBlock(true), _previewDepthMapShader);
}

};