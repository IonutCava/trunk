#include "Headers/Sky.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/LightManager.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

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
    _skyboxRenderState = GFX_DEVICE.getOrCreateStateBlock(skyboxDesc);
    skyboxDesc.setCullMode(CULL_MODE_CW);
    _skyboxRenderStateReflected = GFX_DEVICE.getOrCreateStateBlock(skyboxDesc);
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
    skyboxTextures.setResourceLocation(location+"skybox_2.jpg "+ location+"skybox_1.jpg "+
                                       location+"skybox_5.jpg "+ location+"skybox_6.jpg "+
                                       location+"skybox_3.jpg "+ location+"skybox_4.jpg");
    skyboxTextures.setEnumValue(TEXTURE_CUBE_MAP);
    skyboxTextures.setPropertyDescriptor<SamplerDescriptor>(skyboxSampler);
    _skybox =  CreateResource<Texture>(skyboxTextures);

    ResourceDescriptor skybox("SkyBox");
    skybox.setFlag(true); //no default material;
    _sky = CreateResource<Sphere3D>(skybox);

    ResourceDescriptor sun("Sun");
    sun.setFlag(true);
    _sun = CreateResource<Sphere3D>(sun);

    ResourceDescriptor skyShaderDescriptor("sky");
    _skyShader = CreateResource<ShaderProgram>(skyShaderDescriptor);

    assert(_skyShader);
    _skyShader->UniformTexture("texSky", 0);
    _sky->setResolution(4);
    _sun->setResolution(16);
    _sun->setRadius(0.1f);
    setRenderingOptions(true,true);
    PRINT_FN(Locale::get("CREATE_SKY_RES_OK"));
    setState(RES_LOADED);
    return true;
}

void Sky::postLoad(SceneGraphNode* const sgn){
    if(getState() != RES_LOADED) load();

    _sunNode = sgn->addNode(_sun);
    _skyGeom = sgn->addNode(_sky);
    _sky->getSceneNodeRenderState().setDrawState(false);
    _sun->getSceneNodeRenderState().setDrawState(false);
    _sky->setCustomShader(_skyShader);
    _sun->setCustomShader(_skyShader);

    SceneNode::postLoad(sgn);
}

bool Sky::prepareMaterial(SceneGraphNode* const sgn) {
    SET_STATE_BLOCK(*(GFX_DEVICE.isCurrentRenderStage(REFLECTION_STAGE) ? _skyboxRenderStateReflected : _skyboxRenderState));

    _skyShader->Uniform("sun_color", LightManager::getInstance().getLight(0)->getDiffuseColor());
    return true;
}

void Sky::sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState) {
    Light* l = LightManager::getInstance().getLight(0);
    if (l && _sun->getMaterial()) _sun->getMaterial()->setDiffuse(l->getDiffuseColor());
}

bool Sky::onDraw(SceneGraphNode* const sgn, const RenderStage& currentStage){
    _sun->onDraw(sgn, currentStage);
    return _sky->onDraw(sgn, currentStage);
}

void Sky::render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState){
    const vec3<F32>& eyeTemp = sceneRenderState.getCameraConst().getEye();
    sgn->getTransform()->setPosition(eyeTemp);

    _sunNode->getTransform()->setPosition(eyeTemp - _sunVect);
    _sun->renderInstance()->transform(_sunNode->getTransform());

    if (_drawSky){
        _sky->renderInstance()->transform(sgn->getTransform());

        _skyShader->bind();
        _skybox->Bind(0);
        GFX_DEVICE.renderInstance(_sky->renderInstance());
    }else{
        if (_drawSun) GFX_DEVICE.renderInstance(_sun->renderInstance());
    }
}

void Sky::setRenderingOptions(bool drawSun, bool drawSky) {
    _drawSun = drawSun;
    _drawSky = drawSky;
    _skyShader->Uniform("enable_sun", _drawSun);
}

void Sky::setSunVector(const vec3<F32>& sunVect) {
    _sunVect = sunVect;
    _skyShader->Uniform("sun_vector", _sunVect);
}