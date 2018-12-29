#include "stdafx.h"

#include "Headers/Sky.h"

#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Headers/RenderPackage.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Geometry/Shapes/Predefined/Headers/Sphere3D.h"
#include "ECS/Components/Headers/RenderingComponent.h"

namespace Divide {

Sky::Sky(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, U32 diameter)
    : SceneNode(parentCache, descriptorHash, name, SceneNodeType::TYPE_SKY),
      _context(context),
      _sky(nullptr),
      _skyShader(nullptr),
      _skyShaderPrePass(nullptr),
      _skybox(nullptr),
      _diameter(diameter)
{
    _renderState.addToDrawExclusionMask(RenderStage::SHADOW);

    // Generate a render state
    RenderStateBlock skyboxRenderState;
    skyboxRenderState.setCullMode(CullMode::CCW);
    skyboxRenderState.setZFunc(ComparisonFunction::LESS);
    _skyboxRenderStateHashPrePass = skyboxRenderState.getHash();
    skyboxRenderState.setZFunc(ComparisonFunction::LEQUAL);
    _skyboxRenderStateHash = skyboxRenderState.getHash();

    skyboxRenderState.setCullMode(CullMode::CW);
    _skyboxRenderStateReflectedHash = skyboxRenderState.getHash();
}

Sky::~Sky()
{
}

bool Sky::load(const DELEGATE_CBK<void, CachedResource_wptr>& onLoadCallback) {
    if (_sky != nullptr) {
        return false;
    }

    SamplerDescriptor skyboxSampler = {};
    skyboxSampler._wrapU = TextureWrap::CLAMP_TO_EDGE;
    skyboxSampler._wrapV = TextureWrap::CLAMP_TO_EDGE;
    skyboxSampler._wrapW = TextureWrap::CLAMP_TO_EDGE;
    skyboxSampler._minFilter = TextureFilter::LINEAR;
    skyboxSampler._magFilter = TextureFilter::LINEAR;
    skyboxSampler._anisotropyLevel = 0;

    TextureDescriptor skyboxTexture(TextureType::TEXTURE_CUBE_MAP);
    skyboxTexture.setSampler(skyboxSampler);
    skyboxTexture._srgb = true;

    ResourceDescriptor skyboxTextures("SkyboxTextures");
    skyboxTextures.assetName("skybox_1.jpg, skybox_2.jpg, skybox_3.jpg, skybox_4.jpg, skybox_5.jpg, skybox_6.jpg");
    skyboxTextures.assetLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    skyboxTextures.setPropertyDescriptor(skyboxTexture);

    _skybox = CreateResource<Texture>(_parentCache, skyboxTextures);

    F32 radius = _diameter * 0.5f;

    ResourceDescriptor skybox("SkyBox");
    skybox.setFlag(true);  // no default material;
    skybox.setID(4); // resolution
    skybox.setEnumValue(to_U32(radius)); // radius
    _sky = CreateResource<Sphere3D>(_parentCache, skybox);
    _sky->renderState().setDrawState(false);

    ResourceDescriptor skyShaderDescriptor("sky.Display");
    _skyShader = CreateResource<ShaderProgram>(_parentCache, skyShaderDescriptor);

    ResourceDescriptor skyShaderPrePassDescriptor("sky.PrePass");
    _skyShaderPrePass = CreateResource<ShaderProgram>(_parentCache, skyShaderPrePassDescriptor);

    assert(_skyShader && _skyShaderPrePass);
    _boundingBox.set(vec3<F32>(-radius), vec3<F32>(radius));
    Console::printfn(Locale::get(_ID("CREATE_SKY_RES_OK")));

    return SceneNode::load(onLoadCallback);
}

void Sky::postLoad(SceneGraphNode& sgn) {
    assert(_sky != nullptr);

    SceneGraphNodeDescriptor skyNodeDescriptor;
    skyNodeDescriptor._node = _sky;
    skyNodeDescriptor._name = sgn.name() + "_geometry";
    skyNodeDescriptor._usageContext = NodeUsageContext::NODE_DYNAMIC;
    skyNodeDescriptor._componentMask = to_base(ComponentType::TRANSFORM) |
                                       to_base(ComponentType::BOUNDS) |
                                       to_base(ComponentType::RENDERING) |
                                       to_base(ComponentType::NAVIGATION);
    sgn.addNode(skyNodeDescriptor);

    RenderingComponent* renderable = sgn.get<RenderingComponent>();
    if (renderable) {
        renderable->toggleRenderOption(RenderingComponent::RenderOptions::CAST_SHADOWS, false);
    }

    SceneNode::postLoad(sgn);
}
void Sky::buildDrawCommands(SceneGraphNode& sgn,
                            RenderStagePass renderStagePass,
                            RenderPackage& pkgInOut) {
    if (renderStagePass._stage == RenderStage::SHADOW) {
        return;
    }

    GenericDrawCommand cmd;
    cmd._sourceBuffer = _sky->getGeometryVB();
    cmd._bufferIndex = renderStagePass.index();
    cmd._cmd.indexCount = _sky->getGeometryVB()->getIndexCount();
    enableOption(cmd, CmdRenderOptions::RENDER_INDIRECT);

    GFX::DrawCommand drawCommand;
    drawCommand._drawCommands.push_back(cmd);
    pkgInOut.addDrawCommand(drawCommand);

    DescriptorSet set = pkgInOut.descriptorSet(0);
    set._textureData.addTexture(_skybox->getData(), to_U8(ShaderProgram::TextureUsage::UNIT0));
    pkgInOut.descriptorSet(0, set);

    const Pipeline* pipeline = pkgInOut.pipeline(0);
    PipelineDescriptor pipeDesc = pipeline->descriptor();
    if (renderStagePass._passType == RenderPassType::DEPTH_PASS) {
        pipeDesc._stateHash = _skyboxRenderStateHashPrePass;
        pipeDesc._shaderProgramHandle = _skyShaderPrePass->getID();
    } else {
        pipeDesc._stateHash = (renderStagePass._stage == RenderStage::REFLECTION
                                                      ? _skyboxRenderStateReflectedHash
                                                      : _skyboxRenderStateHash);
        pipeDesc._shaderProgramHandle = _skyShader->getID();
    }

    pkgInOut.pipeline(0, *_context.newPipeline(pipeDesc));

    SceneNode::buildDrawCommands(sgn, renderStagePass, pkgInOut);
}

};