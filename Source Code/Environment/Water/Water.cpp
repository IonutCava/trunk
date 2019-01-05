#include "stdafx.h"

#include "Headers/Water.h"

#include "Core/Headers/Kernel.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Geometry/Material/Headers/Material.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "ECS/Components/Headers/BoundsComponent.h"
#include "ECS/Components/Headers/TransformComponent.h"

namespace Divide {

namespace {
    ClipPlaneIndex g_reflectionClipID = ClipPlaneIndex::CLIP_PLANE_4;
    ClipPlaneIndex g_refractionClipID = ClipPlaneIndex::CLIP_PLANE_5;
};

WaterPlane::WaterPlane(ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name)
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


bool WaterPlane::load(const DELEGATE_CBK<void, CachedResource_wptr>& onLoadCallback) {
    if (_plane != nullptr) {
        return false;
    }

    setState(ResourceState::RES_LOADING);

    _reflectionCam = Camera::createCamera(resourceName() + "_reflectionCam", Camera::CameraType::FREE_FLY);

    const stringImpl& name = resourceName();

    SamplerDescriptor defaultSampler = {};
    defaultSampler._wrapU = TextureWrap::REPEAT;
    defaultSampler._wrapV = TextureWrap::REPEAT;
    defaultSampler._wrapW = TextureWrap::REPEAT;
    defaultSampler._minFilter = TextureFilter::LINEAR;
    defaultSampler._magFilter = TextureFilter::LINEAR;

    TextureDescriptor texDescriptor(TextureType::TEXTURE_2D);
    texDescriptor.setSampler(defaultSampler);

    ResourceDescriptor waterTexture("waterTexture_" + name);
    waterTexture.assetName("terrain_water_NM.jpg");
    waterTexture.assetLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    waterTexture.setPropertyDescriptor(texDescriptor);

    ResourceDescriptor waterTextureDUDV("waterTextureDUDV_" + name);
    waterTextureDUDV.assetName("water_dudv.jpg");
    waterTextureDUDV.assetLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    waterTextureDUDV.setPropertyDescriptor(texDescriptor);

    Texture_ptr waterNM = CreateResource<Texture>(_parentCache, waterTexture);
    assert(waterNM != nullptr);

    Texture_ptr waterDUDV = CreateResource<Texture>(_parentCache, waterTextureDUDV);
    assert(waterDUDV != nullptr);

    ResourceDescriptor waterMaterial("waterMaterial_" + name);
    Material_ptr waterMat = CreateResource<Material>(_parentCache, waterMaterial);
    assert(waterMat != nullptr);

    waterMat->setShadingMode(Material::ShadingMode::BLINN_PHONG);
    waterMat->setTexture(ShaderProgram::TextureUsage::UNIT0, waterNM);
    waterMat->setTexture(ShaderProgram::TextureUsage::UNIT1, waterDUDV);

    ShaderProgramDescriptor shaderDescriptor;
    shaderDescriptor._defines.push_back(std::make_pair("COMPUTE_TBN", true));

    ResourceDescriptor waterColourShader("water");
    waterColourShader.setPropertyDescriptor(shaderDescriptor);
    ShaderProgram_ptr waterColour = CreateResource<ShaderProgram>(_parentCache, waterColourShader);

    ResourceDescriptor waterPrePassShader("water.PrePass");
    waterPrePassShader.setPropertyDescriptor(shaderDescriptor);
    ShaderProgram_ptr waterPrePass = CreateResource<ShaderProgram>(_parentCache, waterPrePassShader);

    waterMat->setShaderProgram(waterColour, RenderPassType::MAIN_PASS);
    waterMat->setShaderProgram(waterPrePass, RenderPassType::PRE_PASS);

    setMaterialTpl(waterMat);
    
    ResourceDescriptor waterPlane("waterPlane");
    waterPlane.setFlag(true);  // No default material
    waterPlane.setThreadedLoading(false);

    _plane = CreateResource<Quad3D>(_parentCache, waterPlane);
    
    setBoundsChanged();

    return SceneNode::load(onLoadCallback);
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

void WaterPlane::updateBoundsInternal() {
    F32 halfWidth = _dimensions.width * 0.5f;
    F32 halfLength = _dimensions.height * 0.5f;

    _boundingBox.set(vec3<F32>(-halfWidth, -_dimensions.depth, -halfLength), vec3<F32>(halfWidth, 0, halfLength));

    SceneNode::updateBoundsInternal();
}

bool WaterPlane::pointUnderwater(const SceneGraphNode& sgn, const vec3<F32>& point) {
    return sgn.get<BoundsComponent>()->getBoundingBox().containsPoint(point);
}

void WaterPlane::buildDrawCommands(SceneGraphNode& sgn,
                                   RenderStagePass renderStagePass,
                                   RenderPackage& pkgInOut) {

    GFX::SendPushConstantsCommand pushConstantsCommand = {};
    pushConstantsCommand._constants.set("_waterShininess", GFX::PushConstantType::FLOAT, 50.0f);
    pushConstantsCommand._constants.set("_noiseFactor", GFX::PushConstantType::VEC2, vec2<F32>(0.15f, 0.15f));
    pushConstantsCommand._constants.set("_noiseTile", GFX::PushConstantType::VEC2, vec2<F32>(15.0f, 15.0f));
    pkgInOut.addPushConstantsCommand(pushConstantsCommand);

    GenericDrawCommand cmd = {};
    cmd._primitiveType = PrimitiveType::TRIANGLE_STRIP;
    cmd._cmd.indexCount = _plane->getGeometryVB()->getIndexCount();
    cmd._sourceBuffer = _plane->getGeometryVB();
    cmd._bufferIndex = renderStagePass.index();
    enableOption(cmd, CmdRenderOptions::RENDER_INDIRECT);
    {
        GFX::DrawCommand drawCommand = {};
        drawCommand._drawCommands.push_back(cmd);
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

    RenderPassManager::PassParams params;
    params._sourceNode = &renderParams._sgn;
    params._occlusionCull = false;
    params._camera = renderParams._camera;
    params._stage = RenderStage::REFRACTION;
    params._target = renderParams._renderTarget;
    params._drawPolicy = &RenderTarget::defaultPolicyKeepDepth();
    params._passIndex = renderParams._passIndex;
    params._clippingPlanes._planes[to_U32(underwater ? g_reflectionClipID : g_refractionClipID)] = refractionPlane;
    params._clippingPlanes._active[to_U32(g_refractionClipID)] = true;
    renderParams._context.parent().renderPassManager().doCustomPass(params, bufferInOut);
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

    RenderPassManager::PassParams params;
    params._sourceNode = &renderParams._sgn;
    params._occlusionCull = false;
    params._camera = _reflectionCam;
    params._stage = RenderStage::REFLECTION;
    params._target = renderParams._renderTarget;
    params._drawPolicy = &RenderTarget::defaultPolicyKeepDepth();
    params._passIndex = renderParams._passIndex;
    params._clippingPlanes._planes[to_U32(underwater ? g_refractionClipID : g_reflectionClipID)] = reflectionPlane;
    params._clippingPlanes._active[to_U32(g_reflectionClipID)] = true;
    renderParams._context.parent().renderPassManager().doCustomPass(params, bufferInOut);
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