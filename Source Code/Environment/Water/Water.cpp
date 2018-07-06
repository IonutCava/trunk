#include "stdafx.h"

#include "Headers/Water.h"

#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

namespace {
    ClipPlaneIndex g_reflectionClipID = ClipPlaneIndex::CLIP_PLANE_4;
    ClipPlaneIndex g_refractionClipID = ClipPlaneIndex::CLIP_PLANE_5;
};

WaterPlane::WaterPlane(ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, const vec3<F32>& dimensions)
    : SceneNode(parentCache, descriptorHash, name, SceneNodeType::TYPE_WATER),
      _plane(nullptr),
      _dimensions(dimensions),
      _reflectionCam(nullptr)
{
    // Set water plane to be single-sided
    P32 quadMask;
    quadMask.i = 0;
    quadMask.b[0] = true;

    ResourceDescriptor waterPlane("waterPlane");
    waterPlane.setFlag(true);  // No default material
    waterPlane.setBoolMask(quadMask);
    _plane = CreateResource<Quad3D>(parentCache, waterPlane);

    // The water doesn't cast shadows, doesn't need ambient occlusion and
    // doesn't have real "depth"
    renderState().addToDrawExclusionMask(RenderStage::SHADOW);

    U32 sideLength = nextPOW2(std::max(to_U32(dimensions.width), to_U32(dimensions.height)));

    Console::printfn(Locale::get(_ID("REFRACTION_INIT_FB")), sideLength, sideLength);

    setFlag(UpdateFlag::BOUNDS_CHANGED);

    _reflectionCam = Camera::createCamera(name + "_reflectionCam", Camera::CameraType::FREE_FLY);
}

WaterPlane::~WaterPlane()
{
    Camera::destroyCamera(_reflectionCam);
}

void WaterPlane::postLoad(SceneGraphNode& sgn) {
    static const U32 normalMask = to_base(SGNComponent::ComponentType::NAVIGATION) |
                                  to_base(SGNComponent::ComponentType::PHYSICS) |
                                  to_base(SGNComponent::ComponentType::BOUNDS) |
                                  to_base(SGNComponent::ComponentType::RENDERING) |
                                  to_base(SGNComponent::ComponentType::NETWORKING);

    F32 halfWidth = _dimensions.width * 0.5f;
    F32 halfLength = _dimensions.height * 0.5f;

    _plane->setCorner(Quad3D::CornerLocation::TOP_LEFT,     vec3<F32>(-halfWidth, 0, -halfLength));
    _plane->setCorner(Quad3D::CornerLocation::TOP_RIGHT,    vec3<F32>( halfWidth, 0, -halfLength));
    _plane->setCorner(Quad3D::CornerLocation::BOTTOM_LEFT,  vec3<F32>(-halfWidth, 0,  halfLength));
    _plane->setCorner(Quad3D::CornerLocation::BOTTOM_RIGHT, vec3<F32>( halfWidth, 0,  halfLength));
    _plane->setNormal(Quad3D::CornerLocation::CORNER_ALL, WORLD_Y_AXIS);
    _plane->renderState().setDrawState(false);
    
    sgn.addNode(_plane, normalMask, PhysicsGroup::GROUP_STATIC);

    RenderingComponent* renderable = sgn.get<RenderingComponent>();
    renderable->setReflectionCallback(DELEGATE_BIND(&WaterPlane::updateReflection,
                                                    this,
                                                    std::placeholders::_1));
    renderable->setRefractionCallback(DELEGATE_BIND(&WaterPlane::updateRefraction,
                                                    this,
                                                    std::placeholders::_1));

    renderable->setReflectionAndRefractionType(ReflectorType::PLANAR_REFLECTOR);

    SceneNode::postLoad(sgn);
}

void WaterPlane::updateBoundsInternal(SceneGraphNode& sgn) {
    F32 halfWidth = _dimensions.width * 0.5f;
    F32 halfLength = _dimensions.height * 0.5f;

    _boundingBox.set(vec3<F32>(-halfWidth, _dimensions.depth, -halfLength), vec3<F32>(halfWidth, 0, halfLength));

    SceneNode::updateBoundsInternal(sgn);
}

bool WaterPlane::unload() {
    bool state = false;
    state = SceneNode::unload();
    return state;
}

bool WaterPlane::pointUnderwater(const SceneGraphNode& sgn, const vec3<F32>& point) {
    return sgn.get<BoundsComponent>()->getBoundingBox().containsPoint(point);
}

bool WaterPlane::onRender(const RenderStagePass& renderStagePass) {
    return _plane->onRender(renderStagePass);
}

void WaterPlane::buildDrawCommands(SceneGraphNode& sgn,
                                   const RenderStagePass& renderStagePass,
                                   RenderPackage& pkgInOut) {
    GenericDrawCommand cmd;
    cmd.primitiveType(PrimitiveType::TRIANGLE_STRIP);
    cmd.sourceBuffer(_plane->getGeometryVB());
    cmd.cmd().indexCount = to_U32(_plane->getGeometryVB()->getIndexCount());

    DrawCommand drawCommand;
    drawCommand._drawCommands.push_back(cmd);
    pkgInOut._commands.add(drawCommand);

    SceneNode::buildDrawCommands(sgn, renderStagePass, pkgInOut);
}

/// update water refraction
void WaterPlane::updateRefraction(RenderCbkParams& renderParams) {
    // If we are above water, process the plane's refraction.
    // If we are below, we render the scene normally
    bool underwater = pointUnderwater(renderParams._sgn, renderParams._camera->getEye());
    Plane<F32> refractionPlane;
    updatePlaneEquation(renderParams._sgn, refractionPlane, underwater);

    RenderPassManager::PassParams params;
    params.doPrePass = true;
    params.occlusionCull = false;
    params.camera = renderParams._camera;
    params.stage = RenderStage::REFRACTION;
    params.target = renderParams._renderTarget;
    params.drawPolicy = params.doPrePass ? &RenderTarget::defaultPolicyKeepDepth() : &RenderTarget::defaultPolicy();
    params.pass = renderParams._passIndex;
    params.clippingPlanes._planes[to_U32(underwater ? g_reflectionClipID : g_refractionClipID)] = refractionPlane;
    params.clippingPlanes._active[to_U32(g_refractionClipID)] = true;
    renderParams._context.parent().renderPassManager().doCustomPass(params);
}

/// Update water reflections
void WaterPlane::updateReflection(RenderCbkParams& renderParams) {
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
    params.doPrePass = true;
    params.occlusionCull = false;
    params.camera = _reflectionCam;
    params.stage = RenderStage::REFLECTION;
    params.target = renderParams._renderTarget;
    params.drawPolicy = params.doPrePass ? &RenderTarget::defaultPolicyKeepDepth() : &RenderTarget::defaultPolicy();
    params.pass = renderParams._passIndex;
    params.clippingPlanes._planes[to_U32(underwater ? g_refractionClipID : g_reflectionClipID)] = reflectionPlane;
    params.clippingPlanes._active[to_U32(g_reflectionClipID)] = true;
    renderParams._context.parent().renderPassManager().doCustomPass(params);
}

void WaterPlane::updatePlaneEquation(const SceneGraphNode& sgn, Plane<F32>& plane, bool reflection) {
    F32 waterLevel = sgn.get<PhysicsComponent>()->getPosition().y;
    const Quaternion<F32>& orientation = sgn.get<PhysicsComponent>()->getOrientation();

    vec3<F32> normal(orientation * (reflection ? WORLD_Y_AXIS : WORLD_Y_NEG_AXIS));
    normal.normalize();
    plane.set(normal, -waterLevel);
}

const vec3<F32>& WaterPlane::getDimensions() const {
    return _dimensions;
}

};