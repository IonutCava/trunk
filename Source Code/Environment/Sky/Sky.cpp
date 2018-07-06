#include "stdafx.h"

#include "Headers/Sky.h"

#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

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

    SamplerDescriptor skyboxSampler;
    skyboxSampler.toggleMipMaps(false);
    skyboxSampler.setFilters(TextureFilter::LINEAR);
    skyboxSampler.setAnisotropy(0);
    skyboxSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    skyboxSampler.toggleSRGBColourSpace(true);

    ResourceDescriptor skyboxTextures("SkyboxTextures");
    skyboxTextures.setResourceName("skybox_1.jpg, skybox_2.jpg, skybox_3.jpg, skybox_4.jpg, skybox_5.jpg, skybox_6.jpg");
    skyboxTextures.setResourceLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    skyboxTextures.setEnumValue(to_base(TextureType::TEXTURE_CUBE_MAP));
    skyboxTextures.setPropertyDescriptor<SamplerDescriptor>(skyboxSampler);
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
    _skyShader->Uniform("enable_sun", true);
    _boundingBox.set(vec3<F32>(-radius), vec3<F32>(radius));
    Console::printfn(Locale::get(_ID("CREATE_SKY_RES_OK")));

    return SceneNode::load(onLoadCallback);
}

void Sky::postLoad(SceneGraphNode& sgn) {
    static const U32 normalMask = to_base(SGNComponent::ComponentType::PHYSICS) |
                                  to_base(SGNComponent::ComponentType::BOUNDS) |
                                  to_base(SGNComponent::ComponentType::RENDERING) |
                                  to_base(SGNComponent::ComponentType::NAVIGATION);

    assert(_sky != nullptr);

    sgn.addNode(_sky, normalMask, PhysicsGroup::GROUP_IGNORE);

    RenderingComponent* renderable = sgn.get<RenderingComponent>();
    if (renderable) {
        renderable->castsShadows(false);

        _skybox->flushTextureState();
        TextureData skyTextureData = _skybox->getData();
        skyTextureData.setHandleLow(to_base(ShaderProgram::TextureUsage::UNIT0));
        renderable->registerTextureDependency(skyTextureData);
    }

    SceneNode::postLoad(sgn);
}

void Sky::sceneUpdate(const U64 deltaTime,
                      SceneGraphNode& sgn,
                      SceneState& sceneState) {

    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}

bool Sky::onRender(const RenderStagePass& renderStagePass) {
    return _sky->onRender(renderStagePass);
}

void Sky::initialiseDrawCommands(SceneGraphNode& sgn,
                                 const RenderStagePass& renderStagePass,
                                 GenericDrawCommands& drawCommandsInOut) {
    GenericDrawCommand cmd;
    cmd.sourceBuffer(_sky->getGeometryVB());
    cmd.cmd().indexCount = _sky->getGeometryVB()->getIndexCount();
    drawCommandsInOut.push_back(cmd);

    SceneNode::initialiseDrawCommands(sgn, renderStagePass, drawCommandsInOut);
}

void Sky::updateDrawCommands(SceneGraphNode& sgn,
                             const RenderStagePass& renderStagePass,
                             const SceneRenderState& sceneRenderState,
                             GenericDrawCommands& drawCommandsInOut) {

    PipelineDescriptor pipeDesc;

    GenericDrawCommand& cmd = drawCommandsInOut.front();
    if (renderStagePass.pass() == RenderPassType::DEPTH_PASS) {
        pipeDesc._stateHash = _skyboxRenderStateHashPrePass;
        pipeDesc._shaderProgram = _skyShaderPrePass;
    }  else {
        pipeDesc._stateHash = (renderStagePass.stage() == RenderStage::REFLECTION
                                                        ? _skyboxRenderStateReflectedHash
                                                        : _skyboxRenderStateHash);
        pipeDesc._shaderProgram = _skyShader;
    }

    cmd.pipeline(_context.newPipeline(pipeDesc));
    SceneNode::updateDrawCommands(sgn, renderStagePass, sceneRenderState, drawCommandsInOut);
}

void Sky::setSunProperties(const vec3<F32>& sunVect,
                           const vec4<F32>& sunColour) {
    _skyShader->Uniform("sun_vector", sunVect);
    _skyShader->Uniform("sun_colour", sunColour.rgb());
}

};