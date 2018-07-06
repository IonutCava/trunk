#include "Headers/CubeShadowMap.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

CubeShadowMap::CubeShadowMap(GFXDevice& context, Light* light, Camera* shadowCamera)
    : ShadowMap(context, light, shadowCamera, ShadowType::CUBEMAP)
{
    
    Console::printfn(Locale::get(_ID("LIGHT_CREATE_SHADOW_FB")), light->getGUID(), "Single Shadow Map");

    ResourceDescriptor shadowPreviewShader("fbPreview.Cube.LinearDepth.ScenePlanes");
    shadowPreviewShader.setThreadedLoading(false);
    shadowPreviewShader.setPropertyList("USE_SCENE_ZPLANES");
    _previewDepthMapShader = CreateResource<ShaderProgram>(light->parentResourceCache(), shadowPreviewShader);
    _previewDepthMapShader->Uniform("useScenePlanes", false);

    for (U32 i = 0; i < 6; ++i) {
        GFXDevice::DebugView_ptr shadow = std::make_shared<GFXDevice::DebugView>();
        shadow->_texture = getDepthMap().getAttachment(RTAttachment::Type::Depth, 0).asTexture();
        shadow->_shader = _previewDepthMapShader;
        shadow->_shaderData._intValues.push_back(std::make_pair("layer", _arrayOffset));
        shadow->_shaderData._intValues.push_back(std::make_pair("face", i));
        context.addDebugView(shadow);
    }
}

CubeShadowMap::~CubeShadowMap()
{
}

void CubeShadowMap::init(ShadowMapInfo* const smi) {
    _init = true;
}

void CubeShadowMap::render(GFXDevice& context, U32 passIdx) {
    context.generateCubeMap(getDepthMapID(),
                            _arrayOffset,
                            _light->getPosition(),
                            vec2<F32>(0.1f, _light->getRange()),
                            RenderStage::SHADOW,
                            passIdx);
}

};