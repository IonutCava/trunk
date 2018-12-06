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

InfinitePlane::InfinitePlane(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, const vec2<U16>& dimensions)
    : SceneNode(parentCache, descriptorHash, name, SceneNodeType::TYPE_INFINITEPLANE),
      _context(context),
      _dimensions(dimensions),
      _plane(nullptr)
{
    _renderState.addToDrawExclusionMask(RenderStage::SHADOW);
    _renderState.addToDrawExclusionMask(RenderStage::REFLECTION);
    _renderState.addToDrawExclusionMask(RenderStage::REFRACTION);
}

InfinitePlane::~InfinitePlane() 
{

}

bool InfinitePlane::load(const DELEGATE_CBK<void, CachedResource_wptr>& onLoadCallback) {
    if (_plane != nullptr) {
        return false;
    }
    
    ResourceDescriptor planeMaterialDescriptor("infinitePlaneMaterial");
    Material_ptr planeMaterial = CreateResource<Material>(_parentCache, planeMaterialDescriptor);
    planeMaterial->setDiffuse(FColour(DefaultColours::WHITE.rgb() * 0.5f, 1.0f));
    planeMaterial->setSpecular(FColour(0.1f, 0.1f, 0.1f, 1.0f));
    planeMaterial->setShininess(20.0f);
    planeMaterial->setShadingMode(Material::ShadingMode::BLINN_PHONG);

    SamplerDescriptor albedoSampler = {};
    albedoSampler._wrapU = TextureWrap::REPEAT;
    albedoSampler._wrapV = TextureWrap::REPEAT;
    albedoSampler._wrapW = TextureWrap::REPEAT;
    albedoSampler._minFilter = TextureFilter::LINEAR;
    albedoSampler._magFilter = TextureFilter::LINEAR;
    albedoSampler._anisotropyLevel = 8;

    TextureDescriptor miscTexDescriptor(TextureType::TEXTURE_2D);
    miscTexDescriptor.setSampler(albedoSampler);
    miscTexDescriptor._srgb = true;

    ResourceDescriptor textureWaterCaustics("Plane Water Caustics");
    textureWaterCaustics.assetLocation(Paths::g_assetsLocation + Paths::g_imagesLocation);
    textureWaterCaustics.assetName("terrain_water_caustics.jpg");
    textureWaterCaustics.setPropertyDescriptor(miscTexDescriptor);

    planeMaterial->setTexture(ShaderProgram::TextureUsage::UNIT0, CreateResource<Texture>(_parentCache, textureWaterCaustics));

    planeMaterial->addShaderDefine("UNDERWATER_DIFFUSE_SCALE 100", true);
    planeMaterial->setShaderProgram("terrainPlane.Colour", RenderStage::DISPLAY, true);
    planeMaterial->setShaderProgram("terrainPlane.Depth", RenderPassType::DEPTH_PASS, true);
    setMaterialTpl(planeMaterial);

    ResourceDescriptor infinitePlane("infinitePlane");
    infinitePlane.setFlag(true);  // No default material

    _plane = CreateResource<Quad3D>(_parentCache, infinitePlane);
    
    _plane->setCorner(Quad3D::CornerLocation::TOP_LEFT, vec3<F32>(-_dimensions.x * 1.5f, 0.0f, -_dimensions.y * 1.5f));
    _plane->setCorner(Quad3D::CornerLocation::TOP_RIGHT, vec3<F32>(_dimensions.x * 1.5f, 0.0f, -_dimensions.y * 1.5f));
    _plane->setCorner(Quad3D::CornerLocation::BOTTOM_LEFT, vec3<F32>(-_dimensions.x * 1.5f, 0.0f, _dimensions.y * 1.5f));
    _plane->setCorner(Quad3D::CornerLocation::BOTTOM_RIGHT, vec3<F32>(_dimensions.x * 1.5f, 0.0f, _dimensions.y * 1.5f));
    _boundingBox.set(vec3<F32>(-_dimensions.x * 1.5f, -0.5f, -_dimensions.y * 1.5f),
                     vec3<F32>( _dimensions.x * 1.5f,  0.5f,  _dimensions.y * 1.5f));

    return SceneNode::load(onLoadCallback);
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
    TransformComponent* tComp = sgn.get<TransformComponent>();

    const vec3<F32>& newEye = sceneState.parentScene().playerCamera()->getEye();
    
    tComp->setPositionX(newEye.x);
    tComp->setPositionZ(newEye.z);
}

bool InfinitePlane::onRender(SceneGraphNode& sgn,
                             const SceneRenderState& sceneRenderState,
                             RenderStagePass renderStagePass) {

    return _plane->onRender(sgn, sceneRenderState, renderStagePass);
}


void InfinitePlane::buildDrawCommands(SceneGraphNode& sgn,
                                      RenderStagePass renderStagePass,
                                      RenderPackage& pkgInOut) {
   
     //infinite plane
    GenericDrawCommand planeCmd = {};
    planeCmd._primitiveType = PrimitiveType::TRIANGLE_STRIP;
    planeCmd._cmd.firstIndex = 0;
    planeCmd._cmd.indexCount = _plane->getGeometryVB()->getIndexCount();
    planeCmd._sourceBuffer = _plane->getGeometryVB();
    planeCmd._bufferIndex = renderStagePass.index();

    {
        GFX::DrawCommand drawCommand;
        drawCommand._drawCommands.push_back(planeCmd);
        pkgInOut.addDrawCommand(drawCommand);
    }

    SceneNode::buildDrawCommands(sgn, renderStagePass, pkgInOut);
}

}; //namespace Divide