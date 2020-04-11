#include "stdafx.h"

#include "Headers/InfinitePlane.h"

#include "Platform/File/Headers/FileManagement.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderPackage.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"
#include "ECS/Components/Headers/TransformComponent.h"
#include "ECS/Components/Headers/RenderingComponent.h"

namespace Divide {

InfinitePlane::InfinitePlane(GFXDevice& context, ResourceCache* parentCache, size_t descriptorHash, const Str128& name, const vec2<U16>& dimensions)
    : SceneNode(parentCache, descriptorHash, name, name, "", SceneNodeType::TYPE_INFINITEPLANE, to_base(ComponentType::TRANSFORM) | to_base(ComponentType::BOUNDS)),
      _planeRenderStateHash(0),
      _planeRenderStateHashPrePass(0),
      _context(context),
      _dimensions(dimensions),
      _plane(nullptr)
{
    _renderState.addToDrawExclusionMask(RenderStage::SHADOW, RenderPassType::COUNT, -1);
    _renderState.addToDrawExclusionMask(RenderStage::REFLECTION, RenderPassType::COUNT, -1);
    _renderState.addToDrawExclusionMask(RenderStage::REFRACTION, RenderPassType::COUNT, -1);
}

InfinitePlane::~InfinitePlane() 
{

}

bool InfinitePlane::load() {
    if (_plane != nullptr) {
        return false;
    }

    setState(ResourceState::RES_LOADING);

    ResourceDescriptor planeMaterialDescriptor("infinitePlaneMaterial");
    Material_ptr planeMaterial = CreateResource<Material>(_parentCache, planeMaterialDescriptor);
    planeMaterial->setShadingMode(ShadingMode::BLINN_PHONG);
    planeMaterial->getColourData().baseColour(FColour4(DefaultColours::WHITE.rgb() * 0.5f, 1.0f));
    planeMaterial->getColourData().specular(FColour3(0.1f, 0.1f, 0.1f));
    planeMaterial->getColourData().shininess(20.0f);

    SamplerDescriptor albedoSampler = {};
    albedoSampler.wrapU(TextureWrap::REPEAT);
    albedoSampler.wrapV(TextureWrap::REPEAT);
    albedoSampler.wrapW(TextureWrap::REPEAT);
    albedoSampler.minFilter(TextureFilter::LINEAR);
    albedoSampler.magFilter(TextureFilter::LINEAR);
    albedoSampler.anisotropyLevel(8);

    TextureDescriptor miscTexDescriptor(TextureType::TEXTURE_2D);
    miscTexDescriptor.samplerDescriptor(albedoSampler);
    miscTexDescriptor.srgb(true);

    ResourceDescriptor textureWaterCaustics("Plane Water Caustics");
    textureWaterCaustics.assetLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    textureWaterCaustics.assetName("terrain_water_caustics.jpg");
    textureWaterCaustics.propertyDescriptor(miscTexDescriptor);

    planeMaterial->setTexture(TextureUsage::UNIT0, CreateResource<Texture>(_parentCache, textureWaterCaustics));

    ShaderModuleDescriptor vertModule = {};
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "terrainPlane.glsl";
    vertModule._variant = "Colour";
    vertModule._defines.emplace_back("UNDERWATER_TILE_SCALE 100", true);
    vertModule._defines.emplace_back("NODE_STATIC", true);

    ShaderModuleDescriptor fragModule = {};
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._sourceFile = "terrainPlane.glsl";
    fragModule._variant = "Colour";
    fragModule._defines.emplace_back("NODE_STATIC", true);

    ShaderProgramDescriptor shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor terrainShader("terrainPlane_Colour");
    terrainShader.propertyDescriptor(shaderDescriptor);
    ShaderProgram_ptr terrainColourShader = CreateResource<ShaderProgram>(_parentCache, terrainShader);

    planeMaterial->setShaderProgram(terrainColourShader, RenderStage::DISPLAY, RenderPassType::COUNT, 0u);

    vertModule._variant = "PrePass";
    fragModule._variant = "PrePass";

    shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor terrainShaderPrePass("terrainPlane_PrePass");
    terrainShaderPrePass.propertyDescriptor(shaderDescriptor);
    ShaderProgram_ptr terrainPrePassShader = CreateResource<ShaderProgram>(_parentCache, terrainShaderPrePass);
    planeMaterial->setShaderProgram(terrainPrePassShader, RenderStage::COUNT, RenderPassType::PRE_PASS, 0u);

    setMaterialTpl(planeMaterial);

    ResourceDescriptor infinitePlane("infinitePlane");
    infinitePlane.flag(true);  // No default material
    infinitePlane.threaded(false);

    _plane = CreateResource<Quad3D>(_parentCache, infinitePlane);
    
    _plane->setCorner(Quad3D::CornerLocation::TOP_LEFT, vec3<F32>(-_dimensions.x * 1.5f, 0.0f, -_dimensions.y * 1.5f));
    _plane->setCorner(Quad3D::CornerLocation::TOP_RIGHT, vec3<F32>(_dimensions.x * 1.5f, 0.0f, -_dimensions.y * 1.5f));
    _plane->setCorner(Quad3D::CornerLocation::BOTTOM_LEFT, vec3<F32>(-_dimensions.x * 1.5f, 0.0f, _dimensions.y * 1.5f));
    _plane->setCorner(Quad3D::CornerLocation::BOTTOM_RIGHT, vec3<F32>(_dimensions.x * 1.5f, 0.0f, _dimensions.y * 1.5f));
    _boundingBox.set(vec3<F32>(-_dimensions.x * 1.5f, -0.5f, -_dimensions.y * 1.5f),
                     vec3<F32>( _dimensions.x * 1.5f,  0.5f,  _dimensions.y * 1.5f));

    return SceneNode::load();
}

void InfinitePlane::postLoad(SceneGraphNode& sgn) {
    assert(_plane != nullptr);

    RenderingComponent* renderable = sgn.get<RenderingComponent>();
    if (renderable) {
        renderable->toggleRenderOption(RenderingComponent::RenderOptions::CAST_SHADOWS, false);
    }

    SceneNode::postLoad(sgn);
}

void InfinitePlane::sceneUpdate(const U64 deltaTimeUS, SceneGraphNode& sgn, SceneState& sceneState) {
    OPTICK_EVENT();

    TransformComponent* tComp = sgn.get<TransformComponent>();

    const vec3<F32>& newEye = sceneState.parentScene().playerCamera()->getEye();

    tComp->setPosition(newEye.x, tComp->getPosition().y, newEye.z);
}

void InfinitePlane::buildDrawCommands(SceneGraphNode& sgn,
                                      RenderStagePass renderStagePass,
                                      RenderPackage& pkgInOut) {

    //infinite plane
    GenericDrawCommand planeCmd = {};
    planeCmd._primitiveType = PrimitiveType::TRIANGLE_STRIP;
    planeCmd._cmd.firstIndex = 0u;
    planeCmd._cmd.indexCount = to_U32(_plane->getGeometryVB()->getIndexCount());
    planeCmd._sourceBuffer = _plane->getGeometryVB()->handle();
    planeCmd._bufferIndex = renderStagePass.baseIndex();
    enableOption(planeCmd, CmdRenderOptions::RENDER_INDIRECT);
    {
        GFX::DrawCommand drawCommand = { planeCmd };
        pkgInOut.addDrawCommand(drawCommand);
    }

    SceneNode::buildDrawCommands(sgn, renderStagePass, pkgInOut);
}

}; //namespace Divide