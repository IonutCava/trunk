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
      _enableSun(true),
      _rebuildDrawCommands(RebuildCommandsState::NONE),
      _sky(nullptr),
      _skyShader(nullptr),
      _skyShaderPrePass(nullptr),
      _skybox(nullptr),
      _diameter(diameter)
{
    _sunColour = FColour3(1.0f, 1.0f, 0.2f);

    _renderState.addToDrawExclusionMask(RenderStage::SHADOW);

    // Generate a render state
    RenderStateBlock skyboxRenderState;
    skyboxRenderState.setCullMode(CullMode::CW);
    skyboxRenderState.setZFunc(ComparisonFunction::EQUAL);
    _skyboxRenderStateReflectedHash = skyboxRenderState.getHash();

    skyboxRenderState.setCullMode(CullMode::CCW);
    _skyboxRenderStateHash = skyboxRenderState.getHash();

    skyboxRenderState.setZFunc(ComparisonFunction::LEQUAL);
    skyboxRenderState.setColourWrites(false, false, false, false);
    _skyboxRenderStateHashPrePass = skyboxRenderState.getHash();

    skyboxRenderState.setCullMode(CullMode::CW);
    _skyboxRenderStateReflectedHashPrePass = skyboxRenderState.getHash();
}

Sky::~Sky()
{
}

bool Sky::load() {
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
    skyboxTextures.waitForReady(false);
    _skybox = CreateResource<Texture>(_parentCache, skyboxTextures);

    F32 radius = _diameter * 0.5f;

    ResourceDescriptor skybox("SkyBox");
    skybox.setFlag(true);  // no default material;
    skybox.setID(4); // resolution
    skybox.setEnumValue(to_U32(radius)); // radius
    _sky = CreateResource<Sphere3D>(_parentCache, skybox);
    _sky->renderState().setDrawState(false);


    ShaderModuleDescriptor vertModule = {};
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "sky.glsl";

    ShaderModuleDescriptor fragModule = {};
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._sourceFile = "sky.glsl";
    fragModule._variant = "Display";

    ShaderProgramDescriptor shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor skyShaderDescriptor("sky_Display");
    skyShaderDescriptor.setPropertyDescriptor(shaderDescriptor);
    skyShaderDescriptor.waitForReady(false);
    _skyShader = CreateResource<ShaderProgram>(_parentCache, skyShaderDescriptor);

    fragModule._variant = "PrePass";

    shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor skyShaderPrePassDescriptor("sky_PrePass");
    skyShaderPrePassDescriptor.waitForReady(false);
    skyShaderPrePassDescriptor.setPropertyDescriptor(shaderDescriptor);
    _skyShaderPrePass = CreateResource<ShaderProgram>(_parentCache, skyShaderPrePassDescriptor);

    assert(_skyShader && _skyShaderPrePass);
    _boundingBox.set(vec3<F32>(-radius), vec3<F32>(radius));
    Console::printfn(Locale::get(_ID("CREATE_SKY_RES_OK")));

    return SceneNode::load();
}

void Sky::postLoad(SceneGraphNode& sgn) {
    assert(_sky != nullptr);

    SceneGraphNodeDescriptor skyNodeDescriptor;
    skyNodeDescriptor._serialize = false;
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
        renderable->lockLoD(true);
        renderable->toggleRenderOption(RenderingComponent::RenderOptions::CAST_SHADOWS, false);
    }

    SceneNode::postLoad(sgn);
}

void Sky::enableSun(bool state, const FColour3& sunColour, const vec3<F32>& sunVector) {
    if (_enableSun != state) {
        _enableSun = state;
        _rebuildDrawCommands = RebuildCommandsState::REQUESTED;
    }

    if (_sunColour != sunColour || _sunVector != sunVector) {
        _sunColour.set(sunColour);
        _sunVector.set(sunVector);
        _rebuildDrawCommands = RebuildCommandsState::REQUESTED;
    }
}

void Sky::sceneUpdate(const U64 deltaTimeUS, SceneGraphNode& sgn, SceneState& sceneState) {
    if (_rebuildDrawCommands == RebuildCommandsState::DONE) {
        _rebuildDrawCommands = RebuildCommandsState::NONE;
    }

    SceneNode::sceneUpdate(deltaTimeUS, sgn, sceneState);
}

bool Sky::preRender(SceneGraphNode& sgn,
                   const Camera& camera,
                   RenderStagePass renderStagePass,
                   bool refreshData,
                   bool& rebuildCommandsOut) {
    if (_rebuildDrawCommands == RebuildCommandsState::REQUESTED) {
        sgn.get<RenderingComponent>()->queueRebuildCommands(renderStagePass);
        _rebuildDrawCommands = RebuildCommandsState::DONE;
    }

    return SceneNode::preRender(sgn, camera, renderStagePass, refreshData, rebuildCommandsOut);
}

void Sky::buildDrawCommands(SceneGraphNode& sgn,
                            RenderStagePass renderStagePass,
                            RenderPackage& pkgInOut) {
    assert(renderStagePass._stage != RenderStage::SHADOW);

    PipelineDescriptor pipelineDescriptor = {};
    if (renderStagePass._passType == RenderPassType::PRE_PASS) {
        WAIT_FOR_CONDITION(_skyShaderPrePass->getState() == ResourceState::RES_LOADED);
        pipelineDescriptor._stateHash = (renderStagePass._stage == RenderStage::REFLECTION 
                                            ? _skyboxRenderStateReflectedHashPrePass
                                            : _skyboxRenderStateHashPrePass);
        pipelineDescriptor._shaderProgramHandle = _skyShaderPrePass->getGUID();
    } else {
        WAIT_FOR_CONDITION(_skyShader->getState() == ResourceState::RES_LOADED);
        pipelineDescriptor._stateHash = (renderStagePass._stage == RenderStage::REFLECTION
                                                      ? _skyboxRenderStateReflectedHash
                                                      : _skyboxRenderStateHash);
        pipelineDescriptor._shaderProgramHandle = _skyShader->getGUID();
    }

    GFX::BindPipelineCommand pipelineCommand = {};
    pipelineCommand._pipeline = _context.newPipeline(pipelineDescriptor);
    pkgInOut.addPipelineCommand(pipelineCommand);

    GFX::BindDescriptorSetsCommand bindDescriptorSetsCommand = {};
    bindDescriptorSetsCommand._set._textureData.setTexture(_skybox->getData(), to_U8(ShaderProgram::TextureUsage::UNIT0));
    pkgInOut.addDescriptorSetsCommand(bindDescriptorSetsCommand);

    GFX::SendPushConstantsCommand pushConstantsCommand = {};
    pushConstantsCommand._constants.set("enable_sun", GFX::PushConstantType::BOOL, _enableSun);
    pushConstantsCommand._constants.set("sun_vector", GFX::PushConstantType::VEC3, _sunVector);
    pushConstantsCommand._constants.set("sun_colour", GFX::PushConstantType::VEC3, _sunColour);
    pkgInOut.addPushConstantsCommand(pushConstantsCommand);

    GenericDrawCommand cmd = {};
    cmd._sourceBuffer = _sky->getGeometryVB()->handle();
    cmd._bufferIndex = renderStagePass.index();
    cmd._cmd.indexCount = _sky->getGeometryVB()->getIndexCount();
    enableOption(cmd, CmdRenderOptions::RENDER_INDIRECT);

    GFX::DrawCommand drawCommand = {};
    drawCommand._drawCommands.push_back(cmd);
    pkgInOut.addDrawCommand(drawCommand);

    SceneNode::buildDrawCommands(sgn, renderStagePass, pkgInOut);
}

};