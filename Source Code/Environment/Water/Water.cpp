#include "stdafx.h"

#include "Headers/Water.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Headers/Configuration.h"

#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Geometry/Material/Headers/Material.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "ECS/Components/Headers/BoundsComponent.h"
#include "ECS/Components/Headers/TransformComponent.h"

namespace Divide {

namespace {
    constexpr bool g_occlusionCullReflection = false;
    // how far to offset the clipping planes for reflections in order to avoid artefacts at water/geometry intersections with high wave noise factors
    constexpr F32 g_reflectionPlaneCorrectionHeight = 1.0f;
};

WaterPlane::WaterPlane(ResourceCache& parentCache, size_t descriptorHash, const Str128& name)
    : SceneNode(parentCache, descriptorHash, name, SceneNodeType::TYPE_WATER),
      _plane(nullptr),
      _reflectionCam(nullptr)
{
    // The water doesn't cast shadows, doesn't need ambient occlusion and doesn't have real "depth"
    renderState().addToDrawExclusionMask(RenderStage::SHADOW);
}

WaterPlane::~WaterPlane()
{
    Camera::destroyCamera(_reflectionCam);
}


bool WaterPlane::load() {
    if (_plane != nullptr) {
        return false;
    }

    setState(ResourceState::RES_LOADING);

    _reflectionCam = Camera::createCamera(resourceName() + "_reflectionCam", Camera::CameraType::FREE_FLY);

    const Str128& name = resourceName();

    SamplerDescriptor defaultSampler = {};
    defaultSampler.wrapU(TextureWrap::REPEAT);
    defaultSampler.wrapV(TextureWrap::REPEAT);
    defaultSampler.wrapW(TextureWrap::REPEAT);
    defaultSampler.minFilter(TextureFilter::LINEAR);
    defaultSampler.magFilter(TextureFilter::LINEAR);

    TextureDescriptor texDescriptor(TextureType::TEXTURE_2D);
    texDescriptor.samplerDescriptor(defaultSampler);

    ResourceDescriptor waterTexture("waterTexture_" + name);
    waterTexture.assetName("terrain_water_NM.jpg");
    waterTexture.assetLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    waterTexture.propertyDescriptor(texDescriptor);

    ResourceDescriptor waterTextureDUDV("waterTextureDUDV_" + name);
    waterTextureDUDV.assetName("water_dudv.jpg");
    waterTextureDUDV.assetLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    waterTextureDUDV.propertyDescriptor(texDescriptor);

    Texture_ptr waterNM = CreateResource<Texture>(_parentCache, waterTexture);
    assert(waterNM != nullptr);

    Texture_ptr waterDUDV = CreateResource<Texture>(_parentCache, waterTextureDUDV);
    assert(waterDUDV != nullptr);

    ResourceDescriptor waterMaterial("waterMaterial_" + name);
    Material_ptr waterMat = CreateResource<Material>(_parentCache, waterMaterial);
    assert(waterMat != nullptr);

    waterMat->setShadingMode(Material::ShadingMode::BLINN_PHONG);
    waterMat->setTexture(ShaderProgram::TextureUsage::UNIT0, waterDUDV);
    waterMat->setTexture(ShaderProgram::TextureUsage::NORMALMAP, waterNM);

    ShaderModuleDescriptor vertModule = {};
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "water.glsl";
    vertModule._defines.push_back(std::make_pair("COMPUTE_TBN", true));

    ShaderModuleDescriptor fragModule = {};
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._sourceFile = "water.glsl";
    fragModule._defines.push_back(std::make_pair("COMPUTE_TBN", true));

    if (!_parentCache.context().config().rendering.shadowMapping.enabled) {
        vertModule._defines.push_back(std::make_pair("DISABLE_SHADOW_MAPPING", true));
        fragModule._defines.push_back(std::make_pair("DISABLE_SHADOW_MAPPING", true));
    }

    ShaderProgramDescriptor shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor waterColourShader("water");
    waterColourShader.propertyDescriptor(shaderDescriptor);
    waterColourShader.waitForReady(false);
    ShaderProgram_ptr waterColour = CreateResource<ShaderProgram>(_parentCache, waterColourShader);

    vertModule._defines.push_back(std::make_pair("PRE_PASS", true));
    fragModule._variant = "PrePass";
    fragModule._defines.push_back(std::make_pair("PRE_PASS", true));

    shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor waterPrePassShader("waterPrePass");
    waterPrePassShader.propertyDescriptor(shaderDescriptor);
    waterPrePassShader.waitForReady(false);
    ShaderProgram_ptr waterPrePass = CreateResource<ShaderProgram>(_parentCache, waterPrePassShader);

    waterMat->setShaderProgram(waterColour, RenderPassType::MAIN_PASS);
    waterMat->setShaderProgram(waterPrePass, RenderPassType::PRE_PASS);
    waterMat->getColourData().shininess(75.0f);

    setMaterialTpl(waterMat);
    
    ResourceDescriptor waterPlane("waterPlane");
    waterPlane.flag(true);  // No default material
    waterPlane.threaded(false);

    _plane = CreateResource<Quad3D>(_parentCache, waterPlane);
    
    F32 halfWidth = _dimensions.width * 0.5f;
    F32 halfLength = _dimensions.height * 0.5f;

    setBounds(BoundingBox(vec3<F32>(-halfWidth, -_dimensions.depth, -halfLength), vec3<F32>(halfWidth, 0, halfLength)));

    return SceneNode::load();
}

void WaterPlane::postLoad(SceneGraphNode& sgn) {

    F32 halfWidth = _dimensions.width * 0.5f;
    F32 halfLength = _dimensions.height * 0.5f;

    _plane->setCorner(Quad3D::CornerLocation::TOP_LEFT, vec3<F32>(-halfWidth, 0, -halfLength));
    _plane->setCorner(Quad3D::CornerLocation::TOP_RIGHT, vec3<F32>(halfWidth, 0, -halfLength));
    _plane->setCorner(Quad3D::CornerLocation::BOTTOM_LEFT, vec3<F32>(-halfWidth, 0, halfLength));
    _plane->setCorner(Quad3D::CornerLocation::BOTTOM_RIGHT, vec3<F32>(halfWidth, 0, halfLength));
    _plane->setNormal(Quad3D::CornerLocation::CORNER_ALL, WORLD_Y_AXIS);
    _boundingBox.set(vec3<F32>(-halfWidth, -_dimensions.depth, -halfLength), vec3<F32>(halfWidth, 0, halfLength));

    RenderingComponent* renderable = sgn.get<RenderingComponent>();

    // If the reflector is reasonibly sized, we should keep LoD fixed so that we always update reflections
    if (sgn.context().config().rendering.lodThresholds.x < std::max(halfWidth, halfLength)) {
        renderable->lockLoD(true);
    }

    renderable->setReflectionCallback([this](RenderCbkParams& params, GFX::CommandBuffer& commandsInOut) {
        updateReflection(params, commandsInOut);
    });

    renderable->setRefractionCallback([this](RenderCbkParams& params, GFX::CommandBuffer& commandsInOut) {
        updateRefraction(params, commandsInOut);
    });

    renderable->setReflectionAndRefractionType(ReflectorType::PLANAR_REFLECTOR);
    renderable->toggleRenderOption(RenderingComponent::RenderOptions::CAST_SHADOWS, false);

    SceneNode::postLoad(sgn);
}

bool WaterPlane::pointUnderwater(const SceneGraphNode& sgn, const vec3<F32>& point) {
    return sgn.get<BoundsComponent>()->getBoundingBox().containsPoint(point);
}

void WaterPlane::buildDrawCommands(SceneGraphNode& sgn,
                                   RenderStagePass renderStagePass,
                                   RenderPackage& pkgInOut) {

    GFX::SendPushConstantsCommand pushConstantsCommand = {};
    pushConstantsCommand._constants.set("_noiseFactor", GFX::PushConstantType::VEC2, vec2<F32>(0.10f, 0.10f));
    pushConstantsCommand._constants.set("_noiseTile", GFX::PushConstantType::VEC2, vec2<F32>(15.0f, 15.0f));
    pkgInOut.addPushConstantsCommand(pushConstantsCommand);

    GenericDrawCommand cmd = {};
    cmd._primitiveType = PrimitiveType::TRIANGLE_STRIP;
    cmd._cmd.indexCount = _plane->getGeometryVB()->getIndexCount();
    cmd._sourceBuffer = _plane->getGeometryVB()->handle();
    cmd._bufferIndex = renderStagePass.index();
    enableOption(cmd, CmdRenderOptions::RENDER_INDIRECT);
    {
        GFX::DrawCommand drawCommand = {cmd};
        pkgInOut.addDrawCommand(drawCommand);
    }
    SceneNode::buildDrawCommands(sgn, renderStagePass, pkgInOut);
}

/// update water refraction
void WaterPlane::updateRefraction(RenderCbkParams& renderParams, GFX::CommandBuffer& bufferInOut) {
    // If we are above water, process the plane's refraction.
    // If we are below, we render the scene normally
    bool underwater = pointUnderwater(renderParams._sgn, renderParams._camera->getEye());
    Plane<F32> refractionPlane;
    updatePlaneEquation(renderParams._sgn, refractionPlane, underwater);

    //Don't clear colour attachment because we'll always draw something for every texel, even if that something is just the sky
    // This may not hold true forever (e.g. may run into fillrate issues) so this needs checking if rendering changes somehow
    RTClearDescriptor clearDescriptor = {};
    clearDescriptor.clearColour(0, false);

    refractionPlane._distance += g_reflectionPlaneCorrectionHeight;

    RenderPassManager::PassParams params = {};
    params._sourceNode = &renderParams._sgn;
    params._targetHIZ = RenderTargetID(RenderTargetUsage::HI_Z_REFRACT);
    params._camera = renderParams._camera;
    params._minExtents.set(0.75f);
    params._stage = RenderStage::REFRACTION;
    params._clearDescriptor = &clearDescriptor;
    params._target = renderParams._renderTarget;
    params._passIndex = renderParams._passIndex;
    params._clippingPlanes._planes[0] = refractionPlane;
    renderParams._context.parent().renderPassManager()->doCustomPass(params, bufferInOut);
}

/// Update water reflections
void WaterPlane::updateReflection(RenderCbkParams& renderParams, GFX::CommandBuffer& bufferInOut) {
    // If we are above water, process the plane's refraction.
    // If we are below, we render the scene normally
    bool underwater = pointUnderwater(renderParams._sgn, renderParams._camera->getEye());

    Plane<F32> reflectionPlane;
    updatePlaneEquation(renderParams._sgn, reflectionPlane, !underwater);

    // Reset reflection cam
    _reflectionCam->fromCamera(*renderParams._camera);
    if (!underwater) {
        _reflectionCam->setReflection(reflectionPlane);
    }

    reflectionPlane._distance += g_reflectionPlaneCorrectionHeight;

    //Don't clear colour attachment because we'll always draw something for every texel, even if that something is just the sky
    RTClearDescriptor clearDescriptor = {};
    clearDescriptor.clearColour(0, false);

    RenderPassManager::PassParams params = {};
    params._sourceNode = &renderParams._sgn;
    params._targetHIZ = RenderTargetID(RenderTargetUsage::HI_Z_REFLECT);
    params._camera = _reflectionCam;
    params._minExtents.set(1.25f);
    params._stage = RenderStage::REFLECTION;
    params._target = renderParams._renderTarget;
    params._passIndex = renderParams._passIndex;
    params._clearDescriptor = &clearDescriptor;
    params._clippingPlanes._planes[0] = reflectionPlane;
    renderParams._context.parent().renderPassManager()->doCustomPass(params, bufferInOut);

    RenderTarget& reflectTarget = renderParams._context.renderTargetPool().renderTarget(renderParams._renderTarget);
    RenderTarget& reflectBlurTarget = renderParams._context.renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::REFLECTION_PLANAR_BLUR));

    RenderTargetHandle source(renderParams._renderTarget, &reflectTarget);
    RenderTargetHandle buffer(RenderTargetID(RenderTargetUsage::REFLECTION_PLANAR_BLUR), &reflectBlurTarget);
    /*renderParams._context.blurTarget(source,
                                     buffer,
                                     source,
                                     RTAttachmentType::Colour,
                                     0, 
                                     9,
                                     bufferInOut);*/
}

void WaterPlane::updatePlaneEquation(const SceneGraphNode& sgn, Plane<F32>& plane, bool reflection) {
    F32 waterLevel = sgn.get<TransformComponent>()->getPosition().y;
    const Quaternion<F32>& orientation = sgn.get<TransformComponent>()->getOrientation();

    vec3<F32> normal(orientation * (reflection ? WORLD_Y_AXIS : WORLD_Y_NEG_AXIS));
    normal.normalize();
    plane.set(normal, -waterLevel);
}

const vec3<U16>& WaterPlane::getDimensions() const {
    return _dimensions;
}

void WaterPlane::saveToXML(boost::property_tree::ptree& pt) const {
    pt.put("dimensions.<xmlattr>.width", _dimensions.width);
    pt.put("dimensions.<xmlattr>.length", _dimensions.height);
    pt.put("dimensions.<xmlattr>.depth", _dimensions.depth);

    SceneNode::saveToXML(pt);
}

void WaterPlane::loadFromXML(const boost::property_tree::ptree& pt) {
    _dimensions.width = pt.get<U16>("dimensions.<xmlattr>.width", 500u);
    _dimensions.height = pt.get<U16>("dimensions.<xmlattr>.length", 500u);
    _dimensions.depth = pt.get<U16>("dimensions.<xmlattr>.depth", 500u);

    SceneNode::loadFromXML(pt);
}

};