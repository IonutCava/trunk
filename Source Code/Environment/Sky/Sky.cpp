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
      _skyShaderPrePass(nullptr),
      _skybox(nullptr)
{
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
    skyboxRenderState.setZFunc(ComparisonFunction::LEQUAL);
    skyboxRenderState.setZWrite(true);
    _skyboxRenderStateHashPrePass = skyboxRenderState.getHash();

    skyboxRenderState.setZWrite(false);
    _skyboxRenderStateReflectedHash = skyboxRenderState.getHash();
    _skyboxRenderStateHash = skyboxRenderState.getHash();
}

Sky::~Sky()
{
    RemoveResource(_skyShader);
    RemoveResource(_skyShaderPrePass);
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
    skyboxSampler.setFilters(TextureFilter::LINEAR);
    skyboxSampler.toggleSRGBColorSpace(true);
    skyboxSampler.setAnisotropy(16);
    skyboxSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);

    ResourceDescriptor skyboxTextures("SkyboxTextures");
    skyboxTextures.setResourceLocation(
        location + "skybox_1.jpg," + location + "skybox_2.jpg," + 
        location + "skybox_3.jpg," + location + "skybox_4.jpg," +
        location + "skybox_5.jpg," + location + "skybox_6.jpg");
    skyboxTextures.setEnumValue(to_uint(TextureType::TEXTURE_CUBE_MAP));
    skyboxTextures.setPropertyDescriptor<SamplerDescriptor>(skyboxSampler);
    skyboxTextures.setThreadedLoading(false);
    _skybox = CreateResource<Texture>(skyboxTextures);

    ResourceDescriptor skybox("SkyBox");
    skybox.setFlag(true);  // no default material;
    skybox.setID(4); // resolution
    skybox.setEnumValue(to_uint(_farPlane / 2.0f)); // radius
    _sky = CreateResource<Sphere3D>(skybox);
    
    ResourceDescriptor skyShaderDescriptor("sky");
    _skyShader = CreateResource<ShaderProgram>(skyShaderDescriptor);

    ResourceDescriptor skyShaderPrePassDescriptor("sky.PrePass");
    skyShaderPrePassDescriptor.setPropertyList("IS_PRE_PASS");
    _skyShaderPrePass = CreateResource<ShaderProgram>(skyShaderPrePassDescriptor);

    assert(_skyShader && _skyShaderPrePass);
    _skyShader->Uniform("texSky", ShaderProgram::TextureUsage::UNIT0);
    _skyShader->Uniform("enable_sun", true);
    _boundingBox.first.set(vec3<F32>(-_farPlane / 2), vec3<F32>(_farPlane / 2));
    _boundingBox.second = true;
    Console::printfn(Locale::get(_ID("CREATE_SKY_RES_OK")));
    return true;
}

void Sky::postLoad(SceneGraphNode& sgn) {
    if (_sky == nullptr) {
        load();
    }

    _sky->renderState().setDrawState(false);
    sgn.addNode(*_sky)->getComponent<PhysicsComponent>()->physicsGroup(
        PhysicsComponent::PhysicsGroup::NODE_COLLIDE_IGNORE);

    RenderingComponent* renderable = sgn.getComponent<RenderingComponent>();
    renderable->castsShadows(false);

    GenericDrawCommand cmd;
    cmd.sourceBuffer(_sky->getGeometryVB());
    cmd.cmd().indexCount = _sky->getGeometryVB()->getIndexCount();

    for (U32 i = 0; i < to_uint(RenderStage::COUNT); ++i) {
        GFXDevice::RenderPackage& pkg = 
            Attorney::RenderingCompSceneNode::getDrawPackage(*renderable, static_cast<RenderStage>(i));
        pkg._drawCommands.push_back(cmd);
    }

    _skybox->flushTextureState();
    TextureData skyTextureData = _skybox->getData();
    skyTextureData.setHandleLow(to_uint(ShaderProgram::TextureUsage::UNIT0));
    sgn.getComponent<RenderingComponent>()->registerTextureDependency(skyTextureData);

    SceneNode::postLoad(sgn);
}

void Sky::sceneUpdate(const U64 deltaTime,
                       SceneGraphNode& sgn,
                      SceneState& sceneState) {

    sgn.getComponent<PhysicsComponent>()->setPosition(sceneState.renderState().getCameraConst().getEye());

    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}

bool Sky::onDraw(SceneGraphNode& sgn, RenderStage currentStage) {
    return _sky->onDraw(sgn, currentStage);
}

bool Sky::getDrawCommands(SceneGraphNode& sgn,
                          RenderStage renderStage,
                          const SceneRenderState& sceneRenderState,
                          vectorImpl<GenericDrawCommand>& drawCommandsOut) {

    GenericDrawCommand& cmd = drawCommandsOut.front();

    RenderingComponent* renderable = sgn.getComponent<RenderingComponent>();
    cmd.renderGeometry(renderable->renderGeometry());
    cmd.renderWireframe(renderable->renderWireframe());
    cmd.stateHash(renderStage == RenderStage::REFLECTION
                              ? _skyboxRenderStateReflectedHash
                              : renderStage == RenderStage::Z_PRE_PASS
                                             ? _skyboxRenderStateHashPrePass
                                             : _skyboxRenderStateHash);
    cmd.shaderProgram(renderStage == RenderStage::Z_PRE_PASS ? _skyShaderPrePass : _skyShader);
    return SceneNode::getDrawCommands(sgn, renderStage, sceneRenderState, drawCommandsOut);
}

void Sky::setSunProperties(const vec3<F32>& sunVect,
                           const vec4<F32>& sunColor) {
    _skyShader->Uniform("sun_vector", sunVect);
    _skyShader->Uniform("sun_color", sunColor.rgb());
}

};