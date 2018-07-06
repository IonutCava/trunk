#include "Headers/CubeShadowMap.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Core/Headers/ParamHandler.h"
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
}

CubeShadowMap::~CubeShadowMap()
{
}

void CubeShadowMap::init(ShadowMapInfo* const smi) {
    _init = true;
}

void CubeShadowMap::render(GFXDevice& context, U32 passIdx) {
    context.generateCubeMap(getDepthMap(),
                            _arrayOffset,
                            _light->getPosition(),
                            vec2<F32>(0.1f, _light->getRange()),
                            RenderStage::SHADOW,
                            passIdx);
}

void CubeShadowMap::previewShadowMaps(GFXDevice& context, U32 rowIndex) {
    if (_previewDepthMapShader->getState() != ResourceState::RES_LOADED) {
        return;
    }

    const vec4<I32> viewport = getViewportForRow(rowIndex);

    getDepthMap().bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Depth, 0);
    _previewDepthMapShader->Uniform("layer", _arrayOffset);

    GenericDrawCommand triangleCmd;
    triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
    triangleCmd.drawCount(1);
    triangleCmd.stateHash(context.getDefaultStateBlock(true));
    triangleCmd.shaderProgram(_previewDepthMapShader);

    for (U32 i = 0; i < 6; ++i) {
        _previewDepthMapShader->Uniform("face", i);
        GFX::ScopedViewport sViewport(context, viewport.x * i, viewport.y, viewport.z, viewport.w);
        context.draw(triangleCmd);
    }
}

};