#include "Headers/Sky.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/LightManager.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

namespace Divide {

Sky::Sky(const std::string& name) : SceneNode(name, TYPE_SKY),
                                    _skyShader(nullptr),
                                    _skybox(nullptr),
                                    _skyGeom(nullptr),
                                    _exclusionMask(0)
{
    //The sky doesn't cast shadows, doesn't need ambient occlusion and doesn't have real "depth"
    _renderState.addToDrawExclusionMask(SHADOW_STAGE);

    //Generate a render state
    RenderStateBlockDescriptor skyboxDesc;
    skyboxDesc.setCullMode(CULL_MODE_CCW);
    //skyboxDesc.setZReadWrite(false, false); - not needed anymore. Using gl_Position.z = gl_Position.w - 0.0001 in GLSL -Ionut
    _skyboxRenderStateHash = GFX_DEVICE.getOrCreateStateBlock(skyboxDesc);
    skyboxDesc.setCullMode(CULL_MODE_CW);
    _skyboxRenderStateReflectedHash = GFX_DEVICE.getOrCreateStateBlock(skyboxDesc);
}

Sky::~Sky(){
    RemoveResource(_skyShader);
    RemoveResource(_skybox);
}

bool Sky::load() {
    if (getState() == RES_LOADED) return false;

    std::string location = ParamHandler::getInstance().getParam<std::string>("assetsLocation")+"/misc_images/";

    SamplerDescriptor skyboxSampler;
    skyboxSampler.toggleMipMaps(false);
    skyboxSampler.setAnisotropy(16);
    skyboxSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);

    ResourceDescriptor skyboxTextures("SkyboxTextures");
    skyboxTextures.setResourceLocation(location + "skybox_2.jpg,"+ location + "skybox_1.jpg,"+
                                       location + "skybox_5.jpg,"+ location + "skybox_6.jpg,"+
                                       location + "skybox_3.jpg,"+ location + "skybox_4.jpg");
    skyboxTextures.setEnumValue(TEXTURE_CUBE_MAP);
    skyboxTextures.setPropertyDescriptor<SamplerDescriptor>(skyboxSampler);
    skyboxTextures.setThreadedLoading(false);
    _skybox =  CreateResource<Texture>(skyboxTextures);

    ResourceDescriptor skybox("SkyBox");
    skybox.setFlag(true); //no default material;
    _sky = CreateResource<Sphere3D>(skybox);

    ResourceDescriptor skyShaderDescriptor("sky");
    _skyShader = CreateResource<ShaderProgram>(skyShaderDescriptor);

    assert(_skyShader);
    _skyShader->UniformTexture("texSky", 0);
    _skyShader->Uniform("enable_sun", true);
    _sky->setResolution(4);
    PRINT_FN(Locale::get("CREATE_SKY_RES_OK"));
    setState(RES_LOADED);
    return true;
}

void Sky::postLoad(SceneGraphNode* const sgn){
    if(getState() != RES_LOADED) load();

    _skyGeom = sgn->addNode(_sky);
    _skyGeom->getComponent<PhysicsComponent>()->physicsGroup(PhysicsComponent::NODE_COLLIDE_IGNORE);
    _sky->getSceneNodeRenderState().setDrawState(false);

    SceneNode::postLoad(sgn);
}

void Sky::sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState) {
    _skyShader->Uniform("sun_color", LightManager::getInstance().getLight(0)->getDiffuseColor());
}

bool Sky::onDraw(SceneGraphNode* const sgn, const RenderStage& currentStage){
    return _sky->onDraw(sgn, currentStage);
}

void Sky::render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState, const RenderStage& currentRenderStage){
    _skybox->Bind(ShaderProgram::TEXTURE_UNIT0);

    GenericDrawCommand cmd;
    cmd.renderWireframe(sgn->renderWireframe());
    cmd.stateHash(GFX_DEVICE.isCurrentRenderStage(REFLECTION_STAGE) ? _skyboxRenderStateReflectedHash : _skyboxRenderStateHash);
    cmd.drawID(GFX_DEVICE.getDrawID(sgn->getGUID()));
    cmd.shaderProgram(_skyShader);
    GFX_DEVICE.submitRenderCommand(_sky->getGeometryVB(), cmd);
}

void Sky::setSunVector(const vec3<F32>& sunVect) {
    _skyShader->Uniform("sun_vector", sunVect);
}

};