#include "stdafx.h"

#include "Headers/Sky.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Headers/Sun.h"

#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Headers/RenderPackage.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Geometry/Shapes/Predefined/Headers/Sphere3D.h"
#include "ECS/Components/Headers/RenderingComponent.h"
#include "ECS/Components/Headers/TransformComponent.h"

namespace Divide {

Sky::Sky(GFXDevice& context, ResourceCache* parentCache, size_t descriptorHash, const Str256& name, U32 diameter)
    : SceneNode(parentCache, descriptorHash, name, name, "", SceneNodeType::TYPE_SKY, to_base(ComponentType::TRANSFORM) | to_base(ComponentType::BOUNDS) | to_base(ComponentType::SCRIPT)),
      _context(context),
      _diameter(diameter)
{
    _sun = eastl::make_unique<Sun>();

    time_t t = time(NULL);
    _sun->SetLocation(-2.589910f, 51.45414); // Bristol :D
    _sun->SetDate(localtime(&t));

    _renderState.addToDrawExclusionMask(RenderStage::SHADOW);

    // Generate a render state
    RenderStateBlock skyboxRenderState = {};
    skyboxRenderState.setCullMode(CullMode::FRONT);
    skyboxRenderState.setZFunc(ComparisonFunction::EQUAL);
    _skyboxRenderStateHash = skyboxRenderState.getHash();

    skyboxRenderState.setZFunc(ComparisonFunction::LEQUAL);
    skyboxRenderState.setColourWrites(false, false, false, false);
    _skyboxRenderStateHashPrePass = skyboxRenderState.getHash();

    RenderStateBlock skyboxRenderStateReflection = {};
    skyboxRenderStateReflection.setCullMode(CullMode::BACK);
    skyboxRenderStateReflection.setZFunc(ComparisonFunction::EQUAL);
    _skyboxRenderStateReflectedHash = skyboxRenderStateReflection.getHash();

    skyboxRenderStateReflection.setZFunc(ComparisonFunction::LEQUAL);
    skyboxRenderStateReflection.setColourWrites(false, false, false, false);
    _skyboxRenderStateReflectedHashPrePass = skyboxRenderStateReflection.getHash();

    getEditorComponent().onChangedCbk([this](std::string_view field) {
        if (field == "Reset To Default") {
            _atmosphere = defaultAtmosphere();
        }

        _atmosphereChanged = EditorDataState::QUEUED; 
    });

    EditorComponentField separatorField = {};
    separatorField._name = "Atmosphere";
    separatorField._type = EditorComponentFieldType::SEPARATOR;
    getEditorComponent().registerField(MOV(separatorField));

    EditorComponentField sunIntensityField = {};
    sunIntensityField._name = "Sun Intensity";
    sunIntensityField._tooltip = "(0.01x - 100.0x)";
    sunIntensityField._data = &_atmosphere._sunIntensity;
    sunIntensityField._type = EditorComponentFieldType::PUSH_TYPE;
    sunIntensityField._readOnly = false;
    sunIntensityField._range = { 0.01f, 100.0f };
    sunIntensityField._basicType = GFX::PushConstantType::FLOAT;
    getEditorComponent().registerField(MOV(sunIntensityField));

    EditorComponentField distanceMultiplierField = {};
    distanceMultiplierField._name = "Distance Multplier x10";
    distanceMultiplierField._tooltip = "Affects atmosphere and ray offsets";
    distanceMultiplierField._data = &_atmosphere._distanceMultiplier;
    distanceMultiplierField._type = EditorComponentFieldType::PUSH_TYPE;
    distanceMultiplierField._readOnly = false;
    distanceMultiplierField._range = { 10, 2000 };
    distanceMultiplierField._basicType = GFX::PushConstantType::INT;
    getEditorComponent().registerField(MOV(distanceMultiplierField));

    EditorComponentField rayOffsetField = {};
    rayOffsetField._name = "Ray Offset: [-10...10]m";
    rayOffsetField._tooltip = "Adds an offset from the planet radius to the atmosphere ray origin";
    rayOffsetField._data = &_atmosphere._rayOriginDistance;
    rayOffsetField._type = EditorComponentFieldType::PUSH_TYPE;
    rayOffsetField._readOnly = false;
    rayOffsetField._range = { -10.f, 10.f };
    rayOffsetField._basicType = GFX::PushConstantType::FLOAT;
    getEditorComponent().registerField(MOV(rayOffsetField));

    EditorComponentField planetRadiusField = {};
    planetRadiusField._name = "Planet Radius [0.1...100]x";
    planetRadiusField._tooltip = "x Times the radius of the Earth (6371e3m)";
    planetRadiusField._data = &_atmosphere._planetRadius;
    planetRadiusField._type = EditorComponentFieldType::PUSH_TYPE;
    planetRadiusField._readOnly = false;
    planetRadiusField._range = { 0.1f, 10.f };
    planetRadiusField._basicType = GFX::PushConstantType::FLOAT;
    getEditorComponent().registerField(MOV(planetRadiusField));

    EditorComponentField atmosphereOffsetField = {};
    atmosphereOffsetField._name = "Atmosphere Offset [1m - 1000m]";
    atmosphereOffsetField._tooltip = "Atmosphere distance in meters starting from the planet radius";
    atmosphereOffsetField._data = &_atmosphere._atmosphereOffset;
    atmosphereOffsetField._type = EditorComponentFieldType::PUSH_TYPE;
    atmosphereOffsetField._readOnly = false;
    atmosphereOffsetField._range = { 1.f, 1000.f };
    atmosphereOffsetField._basicType = GFX::PushConstantType::FLOAT;
    getEditorComponent().registerField(MOV(atmosphereOffsetField));

    EditorComponentField rayleighCoeffField = {};
    rayleighCoeffField._name = "Rayleigh Coefficients";
    rayleighCoeffField._tooltip = "[0.01...100] x e-6";
    rayleighCoeffField._data = _atmosphere._RayleighCoeff._v;
    rayleighCoeffField._type = EditorComponentFieldType::PUSH_TYPE;
    rayleighCoeffField._readOnly = false;
    rayleighCoeffField._range = { 0.01f, 100.0f };
    rayleighCoeffField._basicType = GFX::PushConstantType::VEC3;
    getEditorComponent().registerField(MOV(rayleighCoeffField));

    EditorComponentField rayleighScaleField = {};
    rayleighScaleField._name = "Rayleigh Scale Field";
    rayleighScaleField._tooltip = "[-12...12] x 1e3";
    rayleighScaleField._data = &_atmosphere._RayleighScale;
    rayleighScaleField._type = EditorComponentFieldType::PUSH_TYPE;
    rayleighScaleField._range = { 0.01f, 12.f };
    rayleighScaleField._readOnly = true;
    rayleighScaleField._basicType = GFX::PushConstantType::FLOAT;
    getEditorComponent().registerField(MOV(rayleighScaleField));

    EditorComponentField mieCoeffField = {};
    mieCoeffField._name = "Mie Coefficients";
    mieCoeffField._tooltip = "[0.01...100] x e-6";
    mieCoeffField._data = &_atmosphere._MieCoeff;
    mieCoeffField._type = EditorComponentFieldType::PUSH_TYPE;
    mieCoeffField._readOnly = true;
    mieCoeffField._range = { 0.01f, 100.0f };
    mieCoeffField._basicType = GFX::PushConstantType::FLOAT;
    getEditorComponent().registerField(MOV(rayleighCoeffField));

    EditorComponentField mieScaleField = {};
    mieScaleField._name = "Mie Scale Height";
    mieScaleField._tooltip = "[0.01...10]x 1e3";
    mieScaleField._data = &_atmosphere._MieScaleHeight;
    mieScaleField._type = EditorComponentFieldType::PUSH_TYPE;
    mieScaleField._readOnly = false;
    mieScaleField._range = { 0.01f, 10.0f };
    mieScaleField._basicType = GFX::PushConstantType::FLOAT;
    getEditorComponent().registerField(MOV(mieScaleField));

    EditorComponentField mieScatterDir = {};
    mieScatterDir._name = "Mie Scater Direction";
    mieScatterDir._data = &_atmosphere._MieScatterDir;
    mieScatterDir._type = EditorComponentFieldType::PUSH_TYPE;
    mieScatterDir._readOnly = true;
    mieScatterDir._format = "%e";
    mieScatterDir._basicType = GFX::PushConstantType::FLOAT;

    getEditorComponent().registerField(MOV(mieScaleField));
    EditorComponentField resetField = {};
    resetField._name = "Reset To Default";
    resetField._tooltip = "Default = whatever value was set at load time";
    resetField._type = EditorComponentFieldType::BUTTON;
    getEditorComponent().registerField(MOV(resetField));

    EditorComponentField separatorField2 = {};
    separatorField2._name = "Skybox";
    separatorField2._type = EditorComponentFieldType::SEPARATOR;
    getEditorComponent().registerField(MOV(separatorField2));

    EditorComponentField useDaySkyboxField = {};
    useDaySkyboxField._name = "Use Day Skybox";
    useDaySkyboxField._data = &_useDaySkybox;
    useDaySkyboxField._type = EditorComponentFieldType::PUSH_TYPE;
    useDaySkyboxField._readOnly = false;
    useDaySkyboxField._basicType = GFX::PushConstantType::BOOL;
    getEditorComponent().registerField(MOV(useDaySkyboxField));

    EditorComponentField useNightSkyboxField = {};
    useNightSkyboxField._name = "Use Night Skybox";
    useNightSkyboxField._data = &_useNightSkybox;
    useNightSkyboxField._type = EditorComponentFieldType::PUSH_TYPE;
    useNightSkyboxField._readOnly = false;
    useNightSkyboxField._basicType = GFX::PushConstantType::BOOL;
    getEditorComponent().registerField(MOV(useNightSkyboxField));

    EditorComponentField nightColourField = {};
    nightColourField._name = "Night Colour";
    nightColourField._data = &_nightSkyColour;
    nightColourField._type = EditorComponentFieldType::PUSH_TYPE;
    nightColourField._readOnly = false;
    nightColourField._basicType = GFX::PushConstantType::FCOLOUR4;
    getEditorComponent().registerField(MOV(nightColourField));
}

Sky::~Sky()
{
}

const std::array<vec4<F32>, 3> Sky::atmoToShaderData() const noexcept {
    return {
        vec4<F32>
        {
            _atmosphere._sunIntensity,
            _atmosphere._planetRadius * 6371e3f,
            _atmosphere._atmosphereOffset * _atmosphere._distanceMultiplier,
            _atmosphere._MieCoeff * 1e-6f,
        },
        vec4<F32>
        {
            _atmosphere._RayleighCoeff * 1e-6f,
            _atmosphere._RayleighScale * 1e3f
        },
        vec4<F32>
        {
            _atmosphere._MieScaleHeight * 1e3f,
            _atmosphere._MieScatterDir,
            _atmosphere._rayOriginDistance * _atmosphere._distanceMultiplier
        }
    };
}

bool Sky::load() {
    if (_sky != nullptr) {
        return false;
    }

    std::atomic_uint loadTasks = 0u;

    SamplerDescriptor skyboxSampler = {};
    skyboxSampler.wrapUVW(TextureWrap::CLAMP_TO_EDGE);
    skyboxSampler.minFilter(TextureFilter::LINEAR);
    skyboxSampler.magFilter(TextureFilter::LINEAR);
    skyboxSampler.anisotropyLevel(0);
    _skyboxSampler = skyboxSampler.getHash();

    TextureDescriptor skyboxTexture(TextureType::TEXTURE_CUBE_MAP);
    skyboxTexture.srgb(true);
    {
        ResourceDescriptor skyboxTextures("DayTextures");
        skyboxTextures.assetName("skyboxDay_FRONT.jpg, skyboxDay_BACK.jpg, skyboxDay_UP.jpg, skyboxDay_DOWN.jpg, skyboxDay_LEFT.jpg, skyboxDay_RIGHT.jpg");
        skyboxTextures.assetLocation(Paths::g_assetsLocation + Paths::g_imagesLocation + "/SkyBoxes/");
        skyboxTextures.propertyDescriptor(skyboxTexture);
        skyboxTextures.waitForReady(false);
        _skybox[0] = CreateResource<Texture>(_parentCache, skyboxTextures, loadTasks);

        ResourceDescriptor nightTextures("NightTextures");
        nightTextures.assetName("Milkyway_posx.jpg, Milkyway_negx.jpg, Milkyway_posy.jpg, Milkyway_negy.jpg, Milkyway_posz.jpg, Milkyway_negz.jpg");
        nightTextures.assetLocation(Paths::g_assetsLocation + Paths::g_imagesLocation + "/SkyBoxes/");
        nightTextures.propertyDescriptor(skyboxTexture);
        nightTextures.waitForReady(false);
        _skybox[1] = CreateResource<Texture>(_parentCache, nightTextures, loadTasks);
    }

    const F32 radius = _diameter * 0.5f;

    ResourceDescriptor skybox("SkyBox");
    skybox.flag(true);  // no default material;
    skybox.ID(4); // resolution
    skybox.enumValue(to_U32(radius)); // radius
    _sky = CreateResource<Sphere3D>(_parentCache, skybox);
    _sky->renderState().drawState(false);


    ShaderModuleDescriptor vertModule = {};
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "sky.glsl";
    vertModule._defines.emplace_back("HAS_CLIPPING_OUT", true);

    ShaderModuleDescriptor fragModule = {};
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._sourceFile = "sky.glsl";
    fragModule._variant = "Display";

    ShaderProgramDescriptor shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor skyShaderDescriptor("sky_Display");
    skyShaderDescriptor.propertyDescriptor(shaderDescriptor);
    skyShaderDescriptor.waitForReady(false);
    _skyShader = CreateResource<ShaderProgram>(_parentCache, skyShaderDescriptor, loadTasks);

    fragModule._variant = "PrePass";

    shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);

    ResourceDescriptor skyShaderPrePassDescriptor("sky_PrePass");
    skyShaderPrePassDescriptor.waitForReady(false);
    skyShaderPrePassDescriptor.propertyDescriptor(shaderDescriptor);
    _skyShaderPrePass = CreateResource<ShaderProgram>(_parentCache, skyShaderPrePassDescriptor, loadTasks);

    WAIT_FOR_CONDITION(loadTasks.load() == 0u);

    assert(_skyShader && _skyShaderPrePass);
    setBounds(BoundingBox(vec3<F32>(-radius), vec3<F32>(radius)));
    Console::printfn(Locale::get(_ID("CREATE_SKY_RES_OK")));

    return SceneNode::load();
}

void Sky::postLoad(SceneGraphNode* sgn) {
    assert(_sky != nullptr);

    SceneGraphNodeDescriptor skyNodeDescriptor;
    skyNodeDescriptor._serialize = false;
    skyNodeDescriptor._node = _sky;
    skyNodeDescriptor._name = sgn->name() + "_geometry";
    skyNodeDescriptor._usageContext = NodeUsageContext::NODE_DYNAMIC;
    skyNodeDescriptor._componentMask = to_base(ComponentType::TRANSFORM) |
                                       to_base(ComponentType::BOUNDS) |
                                       to_base(ComponentType::RENDERING) |
                                       to_base(ComponentType::NAVIGATION);
    sgn->addChildNode(skyNodeDescriptor);

    RenderingComponent* renderable = sgn->get<RenderingComponent>();
    if (renderable) {
        renderable->lockLoD(0u);
        renderable->toggleRenderOption(RenderingComponent::RenderOptions::CAST_SHADOWS, false);
    }

    _defaultAtmosphere = atmosphere();

    SceneNode::postLoad(sgn);
}

SunDetails Sky::setDateTime(struct tm *dateTime) {
    _sun->SetDate(dateTime);
    return getCurrentDetails();
}

SunDetails Sky::getCurrentDetails() const noexcept {
    return _sun->GetDetails();
}

void Sky::setAtmosphere(const Atmosphere& atmosphere) noexcept {
    _atmosphere = atmosphere;
    _atmosphereChanged = EditorDataState::QUEUED;
}

const Texture_ptr& Sky::activeSkyBox() const noexcept {
    return _skybox[getCurrentDetails()._intensity <= 0.0f ? 1 : 0];
}

void Sky::sceneUpdate(const U64 deltaTimeUS, SceneGraphNode* sgn, SceneState& sceneState) {
    if (_atmosphereChanged == EditorDataState::QUEUED) {
        _atmosphereChanged = EditorDataState::CHANGED;
    } else if (_atmosphereChanged == EditorDataState::PROCESSED) {
        _atmosphereChanged = EditorDataState::IDLE;
    }

    sgn->get<TransformComponent>()->rotateY(0.3f * Time::MicrosecondsToSeconds<F32>(deltaTimeUS));

    SceneNode::sceneUpdate(deltaTimeUS, sgn, sceneState);
};

bool Sky::prepareRender(SceneGraphNode* sgn,
                RenderingComponent& rComp,
                const RenderStagePass& renderStagePass,
                const Camera& camera,
                const bool refreshData)  {

    RenderPackage& pkg = rComp.getDrawPackage(renderStagePass);
    if (!pkg.empty()) {
        PushConstants constants = pkg.pushConstants(0);

        constants.set(_ID("dvd_nightSkyColour"), GFX::PushConstantType::FCOLOUR3, nightSkyColour().rgb);
        if (_atmosphereChanged == EditorDataState::CHANGED || _atmosphereChanged == EditorDataState::PROCESSED) {
            constants.set(_ID("dvd_useSkyboxes"), GFX::PushConstantType::IVEC2, vec2<I32>(useDaySkybox() ? 1 : 0, useNightSkybox() ? 1 : 0));
            constants.set(_ID("dvd_raySteps"), GFX::PushConstantType::UINT, renderStagePass._stage == RenderStage::DISPLAY ? 16 : 8);
            constants.set(_ID("dvd_atmosphereData"), GFX::PushConstantType::VEC4, atmoToShaderData());
            _atmosphereChanged = EditorDataState::PROCESSED;
        }
         pkg.pushConstants(0, constants);
    }

    return SceneNode::prepareRender(sgn, rComp, renderStagePass, camera, refreshData);
}

void Sky::buildDrawCommands(SceneGraphNode* sgn,
                            const RenderStagePass& renderStagePass,
                            const Camera& crtCamera,
                            RenderPackage& pkgInOut) {
    assert(renderStagePass._stage != RenderStage::SHADOW);

    PipelineDescriptor pipelineDescriptor = {};
    if (renderStagePass._passType == RenderPassType::PRE_PASS) {
        WAIT_FOR_CONDITION(_skyShaderPrePass->getState() == ResourceState::RES_LOADED);
        pipelineDescriptor._stateHash = (renderStagePass._stage == RenderStage::REFLECTION && renderStagePass._variant != to_U8(ReflectorType::CUBE)
                                            ? _skyboxRenderStateReflectedHashPrePass
                                            : _skyboxRenderStateHashPrePass);
        pipelineDescriptor._shaderProgramHandle = _skyShaderPrePass->getGUID();
    } else {
        WAIT_FOR_CONDITION(_skyShader->getState() == ResourceState::RES_LOADED);
        pipelineDescriptor._stateHash = (renderStagePass._stage == RenderStage::REFLECTION && renderStagePass._variant != to_U8(ReflectorType::CUBE)
                                                      ? _skyboxRenderStateReflectedHash
                                                      : _skyboxRenderStateHash);
        pipelineDescriptor._shaderProgramHandle = _skyShader->getGUID();
    }

    GFX::BindPipelineCommand pipelineCommand = {};
    pipelineCommand._pipeline = _context.newPipeline(pipelineDescriptor);
    pkgInOut.add(pipelineCommand);

    GFX::BindDescriptorSetsCommand bindDescriptorSetsCommand = {};
    bindDescriptorSetsCommand._set._textureData.setTexture(_skybox[0]->data(), _skyboxSampler, TextureUsage::UNIT0);
    bindDescriptorSetsCommand._set._textureData.setTexture(_skybox[1]->data(), _skyboxSampler, TextureUsage::UNIT1);
    pkgInOut.add(bindDescriptorSetsCommand);

    GFX::SendPushConstantsCommand pushConstantsCommand = {};
    pushConstantsCommand._constants.set(_ID("dvd_raySteps"), GFX::PushConstantType::UINT, renderStagePass._stage == RenderStage::DISPLAY ? 16 : 8);
    pushConstantsCommand._constants.set(_ID("dvd_atmosphereData"), GFX::PushConstantType::VEC4, atmoToShaderData());
    pushConstantsCommand._constants.set(_ID("dvd_nightSkyColour"), GFX::PushConstantType::FCOLOUR3, nightSkyColour().rgb);
    pushConstantsCommand._constants.set(_ID("dvd_useSkyboxes"), GFX::PushConstantType::IVEC2, vec2<I32>(useDaySkybox() ? 1 : 0, useNightSkybox() ? 1 : 0));
    pkgInOut.add(pushConstantsCommand);

    GenericDrawCommand cmd = {};
    cmd._primitiveType = PrimitiveType::TRIANGLE_STRIP;
    cmd._sourceBuffer = _sky->getGeometryVB()->handle();
    cmd._bufferIndex = renderStagePass.baseIndex();
    cmd._cmd.indexCount = to_U32(_sky->getGeometryVB()->getIndexCount());
    enableOption(cmd, CmdRenderOptions::RENDER_INDIRECT);

    pkgInOut.add(GFX::DrawCommand{ cmd });

    SceneNode::buildDrawCommands(sgn, renderStagePass, crtCamera, pkgInOut);
}

};