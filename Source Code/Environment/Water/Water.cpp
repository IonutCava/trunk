#include "stdafx.h"

#include "Headers/Water.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Headers/Configuration.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"

#include "Geometry/Material/Headers/Material.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "ECS/Components/Headers/BoundsComponent.h"
#include "ECS/Components/Headers/TransformComponent.h"
#include "ECS/Components/Headers/RigidBodyComponent.h"
#include "ECS/Components/Headers/NavigationComponent.h"

namespace Divide {

namespace {
    // how far to offset the clipping planes for reflections in order to avoid artefacts at water/geometry intersections with high wave noise factors
    constexpr F32 g_reflectionPlaneCorrectionHeight = -1.0f;
};

WaterPlane::WaterPlane(ResourceCache* parentCache, size_t descriptorHash, const Str256& name)
    : SceneNode(parentCache, descriptorHash, name, name, "", SceneNodeType::TYPE_WATER, to_base(ComponentType::TRANSFORM))
{
    _noiseTile = { 15.0f, 15.0f };
    _noiseFactor = { 0.1f, 0.1f };
    _refractionTint = {0.f, 0.467f, 0.745f};

    // The water doesn't cast shadows, doesn't need ambient occlusion and doesn't have real "depth"
    renderState().addToDrawExclusionMask(RenderStage::SHADOW);

    renderState().addToDrawExclusionMask(RenderStage::REFLECTION, RenderPassType::COUNT, to_U8(ReflectorType::CUBE));

    EditorComponentField blurReflectionField = {};
    blurReflectionField._name = "Blur reflections";
    blurReflectionField._data = &_blurReflections;
    blurReflectionField._type = EditorComponentFieldType::PUSH_TYPE;
    blurReflectionField._readOnly = false;
    blurReflectionField._basicType = GFX::PushConstantType::BOOL;

    getEditorComponent().registerField(std::move(blurReflectionField));
    
    EditorComponentField blurKernelSizeField = {};
    blurKernelSizeField._name = "Blur kernel size";
    blurKernelSizeField._data = &_blurKernelSize;
    blurKernelSizeField._type = EditorComponentFieldType::SLIDER_TYPE;
    blurKernelSizeField._readOnly = false;
    blurKernelSizeField._basicType = GFX::PushConstantType::UINT;
    blurKernelSizeField._basicTypeSize = GFX::PushConstantSize::WORD;
    blurKernelSizeField._range = { 2.f, 20.f };
    blurKernelSizeField._step = 1.f;

    getEditorComponent().registerField(std::move(blurKernelSizeField));

    EditorComponentField reflPlaneOffsetField = {};
    reflPlaneOffsetField._name = "Reflection Plane Offset";
    reflPlaneOffsetField._data = &_reflPlaneOffset;
    reflPlaneOffsetField._range = { -5.0f, 5.0f };
    reflPlaneOffsetField._type = EditorComponentFieldType::PUSH_TYPE;
    reflPlaneOffsetField._readOnly = false;
    reflPlaneOffsetField._basicType = GFX::PushConstantType::FLOAT;

    getEditorComponent().registerField(std::move(reflPlaneOffsetField));

    EditorComponentField refrPlaneOffsetField = {};
    refrPlaneOffsetField._name = "Refraction Plane Offset";
    refrPlaneOffsetField._data = &_refrPlaneOffset;
    refrPlaneOffsetField._range = { -5.0f, 5.0f };
    refrPlaneOffsetField._type = EditorComponentFieldType::PUSH_TYPE;
    refrPlaneOffsetField._readOnly = false;
    refrPlaneOffsetField._basicType = GFX::PushConstantType::FLOAT;

    getEditorComponent().registerField(std::move(refrPlaneOffsetField));

    EditorComponentField noiseTileSizeField = {};
    noiseTileSizeField._name = "Noise tile factor";
    noiseTileSizeField._data = &_noiseTile;
    noiseTileSizeField._range = { 0.0f, 1000.0f };
    noiseTileSizeField._type = EditorComponentFieldType::PUSH_TYPE;
    noiseTileSizeField._readOnly = false;
    noiseTileSizeField._basicType = GFX::PushConstantType::VEC2;

    getEditorComponent().registerField(std::move(noiseTileSizeField));

    EditorComponentField noiseFactorField = {};
    noiseFactorField._name = "Noise factor";
    noiseFactorField._data = &_noiseFactor;
    noiseFactorField._range = { 0.0f, 10.0f };
    noiseFactorField._type = EditorComponentFieldType::PUSH_TYPE;
    noiseFactorField._readOnly = false;
    noiseFactorField._basicType = GFX::PushConstantType::VEC2;

    getEditorComponent().registerField(std::move(noiseFactorField));

    EditorComponentField refractionTintField = {};
    refractionTintField._name = "Refraction tint";
    refractionTintField._data = &_refractionTint;
    refractionTintField._type = EditorComponentFieldType::PUSH_TYPE;
    refractionTintField._readOnly = false;
    refractionTintField._basicType = GFX::PushConstantType::FCOLOUR3;

    getEditorComponent().registerField(std::move(refractionTintField));

    
    getEditorComponent().onChangedCbk([this](const std::string_view field) {onEditorChange(field); });
}

WaterPlane::~WaterPlane()
{
    Camera::destroyCamera(_reflectionCam);
}

void WaterPlane::onEditorChange(std::string_view field) {
    _editorDataDirtyState = EditorDataState::QUEUED;
}

bool WaterPlane::load() {
    if (_plane != nullptr) {
        return false;
    }

    setState(ResourceState::RES_LOADING);

    _reflectionCam = Camera::createCamera<StaticCamera>(resourceName() + "_reflectionCam");

    const Str256& name = resourceName();

    SamplerDescriptor defaultSampler = {};
    defaultSampler.wrapUVW(TextureWrap::MIRROR_REPEAT);
    defaultSampler.minFilter(TextureFilter::LINEAR_MIPMAP_LINEAR);
    defaultSampler.magFilter(TextureFilter::LINEAR);
    defaultSampler.anisotropyLevel(4);

    TextureDescriptor texDescriptor(TextureType::TEXTURE_2D);

    std::atomic_uint loadTasks = 0u;

    ResourceDescriptor waterTexture("waterTexture_" + name);
    waterTexture.assetName("terrain_water_NM_old.jpg");
    waterTexture.assetLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    waterTexture.propertyDescriptor(texDescriptor);
    waterTexture.waitForReady(false);

    ResourceDescriptor waterTextureDUDV("waterTextureDUDV_" + name);
    waterTextureDUDV.assetName("water_dudv.jpg");
    waterTextureDUDV.assetLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    waterTextureDUDV.propertyDescriptor(texDescriptor);
    waterTextureDUDV.waitForReady(false);

    Texture_ptr waterNM = CreateResource<Texture>(_parentCache, waterTexture, loadTasks);
    Texture_ptr waterDUDV = CreateResource<Texture>(_parentCache, waterTextureDUDV, loadTasks);

    ResourceDescriptor waterMaterial("waterMaterial_" + name);
    Material_ptr waterMat = CreateResource<Material>(_parentCache, waterMaterial);

    waterMat->shadingMode(ShadingMode::COOK_TORRANCE);
    
    ShaderModuleDescriptor vertModule = {};
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "water.glsl";
    vertModule._defines.emplace_back("COMPUTE_TBN", true);
    vertModule._defines.emplace_back("NODE_STATIC", true);

    ShaderModuleDescriptor fragModule = {};
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._sourceFile = "water.glsl";
    fragModule._defines.emplace_back("COMPUTE_TBN", true);
    fragModule._defines.emplace_back("NODE_STATIC", true);

    if (!_parentCache->context().config().rendering.shadowMapping.enabled) {
        vertModule._defines.emplace_back("DISABLE_SHADOW_MAPPING", true);
        fragModule._defines.emplace_back("DISABLE_SHADOW_MAPPING", true);
    }

    ShaderProgramDescriptor shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor waterColourShader("water");
    waterColourShader.propertyDescriptor(shaderDescriptor);
    waterColourShader.waitForReady(false);
    ShaderProgram_ptr waterColour = CreateResource<ShaderProgram>(_parentCache, waterColourShader, loadTasks);

    vertModule._defines.emplace_back("PRE_PASS", true);
    fragModule._variant = "PrePass";

    shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);
    shaderDescriptor._modules.back()._defines.emplace_back("PRE_PASS", true);

    ResourceDescriptor waterPrePassShaderLQ("waterPrePassLQ");
    waterPrePassShaderLQ.propertyDescriptor(shaderDescriptor);
    waterPrePassShaderLQ.waitForReady(false);
    ShaderProgram_ptr waterPrePassLQ = CreateResource<ShaderProgram>(_parentCache, waterPrePassShaderLQ, loadTasks);

    shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);
    shaderDescriptor._modules.back()._defines.emplace_back("PRE_PASS", true);
    shaderDescriptor._modules.back()._defines.emplace_back("USE_DEFERRED_NORMALS", true);

    ResourceDescriptor waterPrePassShader("waterPrePass");
    waterPrePassShader.propertyDescriptor(shaderDescriptor);
    waterPrePassShader.waitForReady(false);
    ShaderProgram_ptr waterPrePass = CreateResource<ShaderProgram>(_parentCache, waterPrePassShader, loadTasks);

    WAIT_FOR_CONDITION(loadTasks.load() == 0u);

    waterMat->setTexture(TextureUsage::UNIT0, waterDUDV, defaultSampler.getHash());
    waterMat->setTexture(TextureUsage::NORMALMAP, waterNM, defaultSampler.getHash());
    waterMat->setShaderProgram(waterPrePassLQ, RenderStage::COUNT, RenderPassType::PRE_PASS);
    waterMat->setShaderProgram(waterPrePass,   RenderStage::DISPLAY, RenderPassType::PRE_PASS);
    waterMat->setShaderProgram(waterColour,    RenderStage::COUNT, RenderPassType::MAIN_PASS);
    waterMat->roughness(0.01f);

    setMaterialTpl(waterMat);
    
    ResourceDescriptor waterPlane("waterPlane");
    waterPlane.flag(true);  // No default material
    waterPlane.threaded(false);

    _plane = CreateResource<Quad3D>(_parentCache, waterPlane);
    
    const F32 halfWidth = _dimensions.width * 0.5f;
    const F32 halfLength = _dimensions.height * 0.5f;

    setBounds(BoundingBox(vec3<F32>(-halfWidth, -_dimensions.depth, -halfLength), vec3<F32>(halfWidth, 0, halfLength)));

    return SceneNode::load();
}

void WaterPlane::postLoad(SceneGraphNode* sgn) {
    NavigationComponent* nComp = sgn->get<NavigationComponent>();
    if (nComp != nullptr) {
        nComp->navigationContext(NavigationComponent::NavigationContext::NODE_OBSTACLE);
    }

    RigidBodyComponent* rComp = sgn->get<RigidBodyComponent>();
    if (rComp != nullptr) {
        rComp->physicsGroup(PhysicsGroup::GROUP_STATIC);
    }

    const F32 halfWidth = _dimensions.width * 0.5f;
    const F32 halfLength = _dimensions.height * 0.5f;

    _plane->setCorner(Quad3D::CornerLocation::TOP_LEFT, vec3<F32>(-halfWidth, 0, -halfLength));
    _plane->setCorner(Quad3D::CornerLocation::TOP_RIGHT, vec3<F32>(halfWidth, 0, -halfLength));
    _plane->setCorner(Quad3D::CornerLocation::BOTTOM_LEFT, vec3<F32>(-halfWidth, 0, halfLength));
    _plane->setCorner(Quad3D::CornerLocation::BOTTOM_RIGHT, vec3<F32>(halfWidth, 0, halfLength));
    _plane->setNormal(Quad3D::CornerLocation::CORNER_ALL, WORLD_Y_AXIS);
    _boundingBox.set(vec3<F32>(-halfWidth, -_dimensions.depth, -halfLength), vec3<F32>(halfWidth, 0, halfLength));

    RenderingComponent* renderable = sgn->get<RenderingComponent>();

    // If the reflector is reasonably sized, we should keep LoD fixed so that we always update reflections
    if (sgn->context().config().rendering.lodThresholds.x < std::max(halfWidth, halfLength)) {
        renderable->lockLoD(0u);
    }

    renderable->setReflectionCallback([this](RenderCbkParams& params, GFX::CommandBuffer& commandsInOut) {
        updateReflection(params, commandsInOut);
    });

    renderable->setRefractionCallback([this](RenderCbkParams& params, GFX::CommandBuffer& commandsInOut) {
        updateRefraction(params, commandsInOut);
    });

    renderable->setReflectionAndRefractionType(ReflectorType::PLANAR, RefractorType::PLANAR);
    renderable->toggleRenderOption(RenderingComponent::RenderOptions::CAST_SHADOWS, false);

    SceneNode::postLoad(sgn);
}

void WaterPlane::sceneUpdate(const U64 deltaTimeUS, SceneGraphNode* sgn, SceneState& sceneState) {

    switch (_editorDataDirtyState) {
        case EditorDataState::QUEUED:
            _editorDataDirtyState = EditorDataState::CHANGED;
            break;
        case EditorDataState::PROCESSED:
            _editorDataDirtyState = EditorDataState::IDLE;
            break;
        default: break;
    };
    WaterBodyData data;
    data._positionW = sgn->get<TransformComponent>()->getPosition();
    data._extents.xyz = { to_F32(_dimensions.width),
                          to_F32(_dimensions.depth),
                          to_F32(_dimensions.height) };

    sceneState.waterBodies().push_back(data);
    SceneNode::sceneUpdate(deltaTimeUS, sgn, sceneState);
}

bool WaterPlane::prepareRender(SceneGraphNode* sgn,
                               RenderingComponent& rComp,
                               const RenderStagePass& renderStagePass,
                               const Camera& camera,
                               bool refreshData) {
    if (_editorDataDirtyState == EditorDataState::CHANGED || _editorDataDirtyState == EditorDataState::PROCESSED) {
        RenderPackage& pkg = rComp.getDrawPackage(renderStagePass);
        PushConstants& constants = pkg.pushConstants(0);
        constants.set(_ID("_noiseFactor"), GFX::PushConstantType::VEC2, _noiseFactor);
        constants.set(_ID("_noiseTile"), GFX::PushConstantType::VEC2, _noiseTile);
        constants.set(_ID("_refractionTint"), GFX::PushConstantType::FCOLOUR3, _refractionTint);
        
    }
    return SceneNode::prepareRender(sgn, rComp, renderStagePass, camera, refreshData);
}

bool WaterPlane::pointUnderwater(const SceneGraphNode* sgn, const vec3<F32>& point) {
    return sgn->get<BoundsComponent>()->getBoundingBox().containsPoint(point);
}

void WaterPlane::buildDrawCommands(SceneGraphNode* sgn,
                                   const RenderStagePass& renderStagePass,
                                   const Camera& crtCamera,
                                   RenderPackage& pkgInOut) {

    GFX::SendPushConstantsCommand pushConstantsCommand = {};
    pushConstantsCommand._constants.set(_ID("_noiseFactor"), GFX::PushConstantType::VEC2, _noiseFactor);
    pushConstantsCommand._constants.set(_ID("_noiseTile"), GFX::PushConstantType::VEC2, _noiseTile);
    pushConstantsCommand._constants.set(_ID("_refractionTint"), GFX::PushConstantType::FCOLOUR3, _refractionTint);
    pkgInOut.add(pushConstantsCommand);

    GenericDrawCommand cmd = {};
    cmd._primitiveType = PrimitiveType::TRIANGLE_STRIP;
    cmd._cmd.indexCount = to_U32(_plane->getGeometryVB()->getIndexCount());
    cmd._sourceBuffer = _plane->getGeometryVB()->handle();
    cmd._bufferIndex = renderStagePass.baseIndex();
    enableOption(cmd, CmdRenderOptions::RENDER_INDIRECT);
    {
        pkgInOut.add(GFX::DrawCommand{ cmd });
    }
    SceneNode::buildDrawCommands(sgn, renderStagePass, crtCamera, pkgInOut);
}

/// update water refraction
void WaterPlane::updateRefraction(RenderCbkParams& renderParams, GFX::CommandBuffer& bufferInOut) {
    static RTClearColourDescriptor clearColourDescriptor;
    clearColourDescriptor._customClearColour[0] = DefaultColours::BLUE;

    // If we are above water, process the plane's refraction.
    // If we are below, we render the scene normally
    const bool underwater = pointUnderwater(renderParams._sgn, renderParams._camera->getEye());
    Plane<F32> refractionPlane;
    updatePlaneEquation(renderParams._sgn, refractionPlane, underwater, refrPlaneOffset());

    RTClearDescriptor clearDescriptor = {};
    clearDescriptor.customClearColour(&clearColourDescriptor);

    refractionPlane._distance += g_reflectionPlaneCorrectionHeight;

    Configuration& config = renderParams._context.context().config();

    RenderPassParams params = {};
    params._sourceNode = renderParams._sgn;
    params._targetHIZ = {}; // We don't need to HiZ cull refractions
    params._targetOIT = {}; // We don't need to draw refracted transparents using woit 
    params._camera = renderParams._camera;
    params._minExtents.set(0.75f);
    params._stagePass = RenderStagePass(RenderStage::REFRACTION, RenderPassType::COUNT, to_U8(RefractorType::PLANAR), renderParams._passIndex);
    params._target = renderParams._renderTarget;
    params._clippingPlanes._planes[0] = refractionPlane;
    params._passName = "Refraction";
    params._shadowMappingEnabled = underwater && config.rendering.shadowMapping.enabled;

    GFX::ClearRenderTargetCommand clearMainTarget = {};
    clearMainTarget._target = params._target;
    clearMainTarget._descriptor = clearDescriptor;
    GFX::EnqueueCommand(bufferInOut, clearMainTarget);

    renderParams._context.parent().renderPassManager()->doCustomPass(params, bufferInOut);
}

/// Update water reflections
void WaterPlane::updateReflection(RenderCbkParams& renderParams, GFX::CommandBuffer& bufferInOut) {
    static RTClearColourDescriptor clearColourDescriptor;
    clearColourDescriptor._customClearColour[0] = DefaultColours::BLUE;

    // If we are above water, process the plane's refraction.
    // If we are below, we render the scene normally
    const bool underwater = pointUnderwater(renderParams._sgn, renderParams._camera->getEye());
    if (underwater) {
        return;
    }

    Plane<F32> reflectionPlane;
    updatePlaneEquation(renderParams._sgn, reflectionPlane, !underwater, reflPlaneOffset());

    // Reset reflection cam
    renderParams._camera->updateLookAt();
    _reflectionCam->fromCamera(*renderParams._camera);
    if (!underwater) {
        reflectionPlane._distance += g_reflectionPlaneCorrectionHeight;
        _reflectionCam->setReflection(reflectionPlane);
    }

    //Don't clear colour attachment because we'll always draw something for every texel, even if that something is just the sky
    RTClearDescriptor clearDescriptor = {};
    clearDescriptor.customClearColour(&clearColourDescriptor);

    RenderPassParams params = {};
    params._sourceNode = renderParams._sgn;
    params._targetHIZ = RenderTargetID(RenderTargetUsage::HI_Z_REFLECT);
    params._targetOIT = RenderTargetID(RenderTargetUsage::OIT_REFLECT);
    params._camera = _reflectionCam;
    params._minExtents.set(1.25f);
    params._stagePass = RenderStagePass(RenderStage::REFLECTION, RenderPassType::COUNT, to_U8(ReflectorType::PLANAR), renderParams._passIndex);
    params._target = renderParams._renderTarget;
    params._clippingPlanes._planes[0] = reflectionPlane;
    params._passName = "Reflection";

    GFX::ClearRenderTargetCommand clearMainTarget = {};
    clearMainTarget._target = params._target;
    clearMainTarget._descriptor = clearDescriptor;
    GFX::EnqueueCommand(bufferInOut, clearMainTarget);

    renderParams._context.parent().renderPassManager()->doCustomPass(params, bufferInOut);

    if (_blurReflections) {
        RenderTarget& reflectTarget = renderParams._context.renderTargetPool().renderTarget(renderParams._renderTarget);
        RenderTargetHandle reflectionTargetHandle(renderParams._renderTarget, &reflectTarget);

        RenderTarget& reflectBlurTarget = renderParams._context.renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::REFLECTION_PLANAR_BLUR));
        RenderTargetHandle reflectionBlurBuffer(RenderTargetID(RenderTargetUsage::REFLECTION_PLANAR_BLUR), &reflectBlurTarget);

        renderParams._context.blurTarget(reflectionTargetHandle,
                                         reflectionBlurBuffer,
                                         reflectionTargetHandle,
                                         RTAttachmentType::Colour,
                                         0, 
                                         _blurKernelSize,
                                         true,
                                         bufferInOut);
    }
}

void WaterPlane::updatePlaneEquation(const SceneGraphNode* sgn, Plane<F32>& plane, bool reflection, F32 offset) {
    const F32 waterLevel = sgn->get<TransformComponent>()->getPosition().y * (reflection ? -1.f : 1.f);
    const Quaternion<F32>& orientation = sgn->get<TransformComponent>()->getOrientation();

    plane.set(Normalized(vec3<F32>(orientation * (reflection ? WORLD_Y_AXIS : WORLD_Y_NEG_AXIS))), 
              offset + waterLevel);
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