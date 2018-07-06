#include "Headers/Sky.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

namespace Divide {

Sky::Sky(const stringImpl& name)
    : SceneNode(name, SceneNodeType::TYPE_SKY),
      _sky(nullptr),
      _skyShader(nullptr),
      _skybox(nullptr)
{
    // The sky doesn't cast shadows, doesn't need ambient occlusion and doesn't
    // have real "depth"
    _renderState.addToDrawExclusionMask(RenderStage::SHADOW);
    _farPlane = 2.0f *
        GET_ACTIVE_SCENE()
        .state()
        .renderState()
        .getCameraConst()
        .getZPlanes()
        .y;
    // Generate a render state
    RenderStateBlock skyboxRenderState;
    skyboxRenderState.setCullMode(CullMode::CCW);
    // skyboxDesc.setZReadWrite(false, false); - not needed anymore. Using
    // gl_Position.z = gl_Position.w - 0.0001 in GLSL -Ionut
    _skyboxRenderStateHash = skyboxRenderState.getHash();
    skyboxRenderState.setCullMode(CullMode::CW);
    _skyboxRenderStateReflectedHash = skyboxRenderState.getHash();
}

Sky::~Sky()
{
    RemoveResource(_skyShader);
    RemoveResource(_skybox);
}

bool Sky::load() {
    if (_sky != nullptr) {
        return false;
    }
    stringImpl location(
        ParamHandler::getInstance().getParam<stringImpl>("assetsLocation") +
        "/misc_images/");

    SamplerDescriptor skyboxSampler;
    skyboxSampler.toggleMipMaps(false);
    skyboxSampler.toggleSRGBColorSpace(true);
    skyboxSampler.setAnisotropy(16);
    skyboxSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);

    ResourceDescriptor skyboxTextures("SkyboxTextures");
    skyboxTextures.setResourceLocation(
        location + "skybox_2.jpg," + location + "skybox_1.jpg," + location +
        "skybox_5.jpg," + location + "skybox_6.jpg," + location +
        "skybox_3.jpg," + location + "skybox_4.jpg");
    skyboxTextures.setEnumValue(to_uint(TextureType::TEXTURE_CUBE_MAP));
    skyboxTextures.setPropertyDescriptor<SamplerDescriptor>(skyboxSampler);
    skyboxTextures.setThreadedLoading(false);
    _skybox = CreateResource<Texture>(skyboxTextures);

    ResourceDescriptor skybox("SkyBox");
    skybox.setFlag(true);  // no default material;
    _sky = CreateResource<Sphere3D>(skybox);

    ResourceDescriptor skyShaderDescriptor("sky");
    _skyShader = CreateResource<ShaderProgram>(skyShaderDescriptor);

    assert(_skyShader);
    _skyShader->Uniform("texSky", ShaderProgram::TextureUsage::UNIT0);
    _skyShader->Uniform("enable_sun", true);
    _sky->setResolution(4);
    Console::printfn(Locale::get("CREATE_SKY_RES_OK"));
    return true;
}

void Sky::postLoad(SceneGraphNode& sgn) {
    if (_sky == nullptr) {
        load();
    }
    _sky->renderState().setDrawState(false);
    sgn.addNode(*_sky)->getComponent<PhysicsComponent>()->physicsGroup(
        PhysicsComponent::PhysicsGroup::NODE_COLLIDE_IGNORE);

    SceneNode::postLoad(sgn);
}

void Sky::sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn,
                      SceneState& sceneState) {}

bool Sky::onDraw(SceneGraphNode& sgn, RenderStage currentStage) {
    if (_sky->onDraw(sgn, currentStage)) {
        sgn.getComponent<RenderingComponent>()->makeTextureResident(
            *_skybox, static_cast<U8>(ShaderProgram::TextureUsage::UNIT0));
        return true;
    }
    return false;
}

void Sky::getDrawCommands(SceneGraphNode& sgn,
                          RenderStage renderStage,
                          SceneRenderState& sceneRenderState,
                          vectorImpl<GenericDrawCommand>& drawCommandsOut) {

    RenderingComponent* renderable = sgn.getComponent<RenderingComponent>();

    GenericDrawCommand cmd;
    cmd.renderGeometry(renderable->renderGeometry());
    cmd.renderWireframe(renderable->renderWireframe());
    cmd.stateHash(renderStage == RenderStage::REFLECTION
                      ? _skyboxRenderStateReflectedHash
                      : _skyboxRenderStateHash);
    cmd.shaderProgram(_skyShader);
    cmd.sourceBuffer(_sky->getGeometryVB());
    cmd.indexCount(_sky->getGeometryVB()->getIndexCount());
    drawCommandsOut.push_back(cmd);
}

void Sky::setSunProperties(const vec3<F32>& sunVect,
                           const vec4<F32>& sunColor) {
    _skyShader->Uniform("sun_vector", sunVect);
    _skyShader->Uniform("sun_color", sunColor.rgb());
}

bool Sky::computeBoundingBox(SceneGraphNode& sgn) {
    sgn.getBoundingBox().set(vec3<F32>(-_farPlane), vec3<F32>(_farPlane));
    return SceneNode::computeBoundingBox(sgn);
}

};