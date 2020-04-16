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

Sky::Sky(GFXDevice& context, ResourceCache* parentCache, size_t descriptorHash, const Str128& name, U32 diameter)
    : SceneNode(parentCache, descriptorHash, name, name, "", SceneNodeType::TYPE_SKY, to_base(ComponentType::TRANSFORM) | to_base(ComponentType::BOUNDS) | to_base(ComponentType::SCRIPT)),
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

    _renderState.addToDrawExclusionMask(RenderStage::SHADOW, RenderPassType::COUNT, -1);
    _renderState.addToDrawExclusionMask(RenderStage::REFRACTION, RenderPassType::COUNT, -1);

    // Generate a render state
    RenderStateBlock skyboxRenderState;
    skyboxRenderState.setCullMode(CullMode::BACK);
    skyboxRenderState.setZFunc(ComparisonFunction::EQUAL);
    _skyboxRenderStateReflectedHash = skyboxRenderState.getHash();

    skyboxRenderState.setCullMode(CullMode::FRONT);
    _skyboxRenderStateHash = skyboxRenderState.getHash();

    skyboxRenderState.setZFunc(ComparisonFunction::LEQUAL);
    skyboxRenderState.setColourWrites(false, false, false, false);
    _skyboxRenderStateHashPrePass = skyboxRenderState.getHash();

    skyboxRenderState.setCullMode(CullMode::BACK);
    _skyboxRenderStateReflectedHashPrePass = skyboxRenderState.getHash();
}

Sky::~Sky()
{
}

bool Sky::load() {
    if (_sky != nullptr) {
        return false;
    }

    std::atomic_uint loadTasks = 0u;

    SamplerDescriptor skyboxSampler = {};
    skyboxSampler.wrapU(TextureWrap::CLAMP_TO_EDGE);
    skyboxSampler.wrapV(TextureWrap::CLAMP_TO_EDGE);
    skyboxSampler.wrapW(TextureWrap::CLAMP_TO_EDGE);
    skyboxSampler.minFilter(TextureFilter::LINEAR);
    skyboxSampler.magFilter(TextureFilter::LINEAR);
    skyboxSampler.anisotropyLevel(0);

    TextureDescriptor skyboxTexture(TextureType::TEXTURE_CUBE_MAP);
    skyboxTexture.samplerDescriptor(skyboxSampler);
    skyboxTexture.srgb(true);

    ResourceDescriptor skyboxTextures("SkyboxTextures");
    skyboxTextures.assetName("skybox_1.jpg, skybox_2.jpg, skybox_3.jpg, skybox_4.jpg, skybox_5.jpg, skybox_6.jpg");
    skyboxTextures.assetLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    skyboxTextures.propertyDescriptor(skyboxTexture);
    skyboxTextures.waitForReady(false);
    _skybox = CreateResource<Texture>(_parentCache, skyboxTextures, loadTasks);

    F32 radius = _diameter * 0.5f;

    ResourceDescriptor skybox("SkyBox");
    skybox.flag(true);  // no default material;
    skybox.ID(4); // resolution
    skybox.enumValue(to_U32(radius)); // radius
    _sky = CreateResource<Sphere3D>(_parentCache, skybox);
    _sky->renderState().drawState(false);


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
    skyShaderDescriptor.propertyDescriptor(shaderDescriptor);
    skyShaderDescriptor.waitForReady(false);
    _skyShader = CreateResource<ShaderProgram>(_parentCache, skyShaderDescriptor, loadTasks);

    fragModule._variant = "PrePass";

    shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor skyShaderPrePassDescriptor("sky_PrePass");
    skyShaderPrePassDescriptor.waitForReady(false);
    skyShaderPrePassDescriptor.propertyDescriptor(shaderDescriptor);
    _skyShaderPrePass = CreateResource<ShaderProgram>(_parentCache, skyShaderPrePassDescriptor, loadTasks);

    WAIT_FOR_CONDITION(loadTasks.load() == 0u);

    assert(_skyShader && _skyShaderPrePass);
    setBounds(BoundingBox(vec3<F32>(-radius), vec3<F32>(radius)));
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
    sgn.addChildNode(skyNodeDescriptor);

    RenderingComponent* renderable = sgn.get<RenderingComponent>();
    if (renderable) {
        renderable->lockLoD(0u);
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
        rebuildCommandsOut = true;
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
    bindDescriptorSetsCommand._set._textureData.setTexture(_skybox->data(), to_U8(TextureUsage::UNIT0));
    pkgInOut.addDescriptorSetsCommand(bindDescriptorSetsCommand);

    GFX::SendPushConstantsCommand pushConstantsCommand = {};
    pushConstantsCommand._constants.set(_ID("enable_sun"), GFX::PushConstantType::BOOL, _enableSun);
    pushConstantsCommand._constants.set(_ID("sun_vector"), GFX::PushConstantType::VEC3, _sunVector);
    pushConstantsCommand._constants.set(_ID("sun_colour"), GFX::PushConstantType::VEC3, _sunColour);
    pkgInOut.addPushConstantsCommand(pushConstantsCommand);

    GenericDrawCommand cmd = {};
    cmd._sourceBuffer = _sky->getGeometryVB()->handle();
    cmd._bufferIndex = renderStagePass.baseIndex();
    cmd._cmd.indexCount = to_U32(_sky->getGeometryVB()->getIndexCount());
    enableOption(cmd, CmdRenderOptions::RENDER_INDIRECT);

    GFX::DrawCommand drawCommand = {cmd};
    pkgInOut.addDrawCommand(drawCommand);

    SceneNode::buildDrawCommands(sgn, renderStagePass, pkgInOut);
}

};