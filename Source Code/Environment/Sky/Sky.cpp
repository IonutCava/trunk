#include "Headers/Sky.h"

#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

namespace Divide {

Sky::Sky(ResourceCache& parentCache, const stringImpl& name, U32 diameter)
    : SceneNode(parentCache, name, SceneNodeType::TYPE_SKY),
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
    _skyboxRenderStateReflectedHash = _skyboxRenderStateHash;
}

Sky::~Sky()
{
}

bool Sky::load() {
    if (_sky != nullptr) {
        return false;
    }

    SamplerDescriptor skyboxSampler;
    skyboxSampler.toggleMipMaps(false);
    skyboxSampler.setFilters(TextureFilter::LINEAR);
    skyboxSampler.toggleSRGBColourSpace(true);
    skyboxSampler.setAnisotropy(0);
    skyboxSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);

    ResourceDescriptor skyboxTextures("SkyboxTextures");
    skyboxTextures.setResourceName("skybox_1.jpg, skybox_2.jpg, skybox_3.jpg, skybox_4.jpg, skybox_5.jpg, skybox_6.jpg");
    skyboxTextures.setResourceLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    skyboxTextures.setEnumValue(to_const_uint(TextureType::TEXTURE_CUBE_MAP));
    skyboxTextures.setPropertyDescriptor<SamplerDescriptor>(skyboxSampler);
    _skybox = CreateResource<Texture>(_parentCache, skyboxTextures);

    F32 radius = _diameter * 0.5f;

    ResourceDescriptor skybox("SkyBox");
    skybox.setFlag(true);  // no default material;
    skybox.setID(4); // resolution
    skybox.setEnumValue(to_uint(radius)); // radius
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
    return Resource::load();
}

void Sky::postLoad(SceneGraphNode& sgn) {
    static const U32 normalMask = to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                  to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                  to_const_uint(SGNComponent::ComponentType::RENDERING);

    assert(_sky != nullptr);

    sgn.addNode(_sky, normalMask, PhysicsGroup::GROUP_IGNORE);

    RenderingComponent* renderable = sgn.get<RenderingComponent>();
    if (renderable) {
        renderable->castsShadows(false);

        _skybox->flushTextureState();
        TextureData skyTextureData = _skybox->getData();
        skyTextureData.setHandleLow(to_const_uint(ShaderProgram::TextureUsage::UNIT0));
        renderable->registerTextureDependency(skyTextureData);
    }

    SceneNode::postLoad(sgn);
}

void Sky::onCameraUpdate(SceneGraphNode& sgn,
                         const I64 cameraGUID,
                         const vec3<F32>& posOffset,
                         const mat4<F32>& rotationOffset) {
    SceneNode::onCameraUpdate(sgn, cameraGUID, posOffset, rotationOffset);

    sgn.get<PhysicsComponent>()->setPosition(posOffset);
}

void Sky::onCameraChange(SceneGraphNode& sgn,
                         const Camera& cam) {
    SceneNode::onCameraChange(sgn, cam);

    sgn.get<PhysicsComponent>()->setPosition(cam.getEye());
}

void Sky::sceneUpdate(const U64 deltaTime,
                      SceneGraphNode& sgn,
                      SceneState& sceneState) {

    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}

bool Sky::onRender(RenderStage currentStage) {
    return _sky->onRender(currentStage);
}

void Sky::initialiseDrawCommands(SceneGraphNode& sgn,
                                 RenderStage renderStage,
                                 GenericDrawCommands& drawCommandsInOut) {
    GenericDrawCommand cmd;
    cmd.sourceBuffer(_sky->getGeometryVB());
    cmd.cmd().indexCount = _sky->getGeometryVB()->getIndexCount();
    drawCommandsInOut.push_back(cmd);

    SceneNode::initialiseDrawCommands(sgn, renderStage, drawCommandsInOut);
}

void Sky::updateDrawCommands(SceneGraphNode& sgn,
                             RenderStage renderStage,
                             const SceneRenderState& sceneRenderState,
                             GenericDrawCommands& drawCommandsInOut) {

    GenericDrawCommand& cmd = drawCommandsInOut.front();
    cmd.stateHash(renderStage == RenderStage::REFLECTION
                              ? _skyboxRenderStateReflectedHash
                              : renderStage == RenderStage::Z_PRE_PASS
                                             ? _skyboxRenderStateHashPrePass
                                             : _skyboxRenderStateHash);
    cmd.shaderProgram(renderStage == RenderStage::Z_PRE_PASS ? _skyShaderPrePass : _skyShader);
    SceneNode::updateDrawCommands(sgn, renderStage, sceneRenderState, drawCommandsInOut);
}

void Sky::setSunProperties(const vec3<F32>& sunVect,
                           const vec4<F32>& sunColour) {
    _skyShader->Uniform("sun_vector", sunVect);
    _skyShader->Uniform("sun_colour", sunColour.rgb());
}

};