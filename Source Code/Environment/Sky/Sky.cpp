#include "Headers/Sky.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

namespace Divide {

Sky::Sky(const stringImpl& name) : SceneNode(name, TYPE_SKY),
									_sky(nullptr),
                                    _skyShader(nullptr),
                                    _skybox(nullptr),
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

Sky::~Sky()
{
    RemoveResource(_skyShader);
    RemoveResource(_skybox);
}

bool Sky::load() {
	if ( _sky != nullptr ) {
		return false;
	}
	stringImpl location(ParamHandler::getInstance().getParam<stringImpl>("assetsLocation") + "/misc_images/");

    SamplerDescriptor skyboxSampler;
    skyboxSampler.toggleMipMaps(false);
	skyboxSampler.toggleSRGBColorSpace(true);
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
    return true;
}

void Sky::postLoad( SceneGraphNode* const sgn ) {
    if ( _sky == nullptr ) {
        load();
    }
    _sky->renderState().setDrawState( false );
    sgn->addNode( _sky )->getComponent<PhysicsComponent>()->physicsGroup( PhysicsComponent::NODE_COLLIDE_IGNORE );

    SceneNode::postLoad( sgn );
}

void Sky::sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState) {
}

bool Sky::onDraw(SceneGraphNode* const sgn, const RenderStage& currentStage) {
    return _sky->onDraw(sgn, currentStage);
}

void Sky::getDrawCommands(SceneGraphNode* const sgn, 
                          const RenderStage& currentRenderStage, 
                          SceneRenderState& sceneRenderState, 
                          vectorImpl<GenericDrawCommand>& drawCommandsOut) {
    GenericDrawCommand cmd;
    cmd.renderWireframe(sgn->getComponent<RenderingComponent>()->renderWireframe());
    cmd.stateHash(GFX_DEVICE.isCurrentRenderStage(REFLECTION_STAGE) ? _skyboxRenderStateReflectedHash : _skyboxRenderStateHash);
    cmd.shaderProgram(_skyShader);
    cmd.sourceBuffer(_sky->getGeometryVB());
    cmd.indexCount(_sky->getGeometryVB()->getIndexCount());
    drawCommandsOut.push_back(cmd);
}

void Sky::render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState, const RenderStage& currentRenderStage){
    _skybox->Bind(ShaderProgram::TEXTURE_UNIT0);
    GFX_DEVICE.submitRenderCommand(sgn->getComponent<RenderingComponent>()->getDrawCommands());
}

void Sky::setSunProperties(const vec3<F32>& sunVect, const vec4<F32>& sunColor) {
    _skyShader->Uniform("sun_vector", sunVect);
	_skyShader->Uniform("sun_color", sunColor.rgb());
}

};