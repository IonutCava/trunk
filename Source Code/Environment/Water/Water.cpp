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

WaterPlane::WaterPlane(ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, I32 sideLength)
    : SceneNode(parentCache, descriptorHash, name, SceneNodeType::TYPE_WATER),
      _plane(nullptr),
      _sideLength(std::max(sideLength, 1)),
      _paramsDirty(true),
      _reflectionCam(nullptr)
{
    // Set water plane to be single-sided
    P32 quadMask;
    quadMask.i = 0;
    quadMask.b[0] = true;

    setParams(50.0f, vec2<F32>(10.0f, 10.0f), vec2<F32>(0.1f, 0.1f), 0.34f);

    ResourceDescriptor waterPlane("waterPlane");
    waterPlane.setFlag(true);  // No default material
    waterPlane.setBoolMask(quadMask);
    _plane = CreateResource<Quad3D>(parentCache, waterPlane);

    // The water doesn't cast shadows, doesn't need ambient occlusion and
    // doesn't have real "depth"
    renderState().addToDrawExclusionMask(RenderStage::SHADOW);
    Console::printfn(Locale::get(_ID("REFRACTION_INIT_FB")), nextPOW2(sideLength), nextPOW2(sideLength));

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

    _plane->setCorner(Quad3D::CornerLocation::TOP_LEFT,     vec3<F32>(-_sideLength, 0, -_sideLength));
    _plane->setCorner(Quad3D::CornerLocation::TOP_RIGHT,    vec3<F32>( _sideLength, 0, -_sideLength));
    _plane->setCorner(Quad3D::CornerLocation::BOTTOM_LEFT,  vec3<F32>(-_sideLength, 0,  _sideLength));
    _plane->setCorner(Quad3D::CornerLocation::BOTTOM_RIGHT, vec3<F32>( _sideLength, 0,  _sideLength));
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
    if (_paramsDirty) {
        _boundingBox.set(vec3<F32>(-_sideLength), vec3<F32>(_sideLength, 0, _sideLength));
    }

    SceneNode::updateBoundsInternal(sgn);
}

bool WaterPlane::unload() {
    bool state = false;
    state = SceneNode::unload();
    return state;
}

void WaterPlane::setParams(F32 shininess, const vec2<F32>& noiseTile,
                           const vec2<F32>& noiseFactor, F32 transparency) {
    _shininess = shininess;
    _noiseTile = noiseTile;
    _noiseFactor = noiseFactor;
    _paramsDirty = true;
    //ToDo: handle transparency?
}

void WaterPlane::sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn,  SceneState& sceneState) {
    if (_paramsDirty) {
        RenderingComponent* rComp = sgn.get<RenderingComponent>();
        for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
            for (U32 i = 0; i < to_base(RenderStage::COUNT); ++i) {
                const ShaderProgram_ptr& shader = rComp->getMaterialInstance()->getShaderInfo(RenderStagePass(static_cast<RenderStage>(i), static_cast<RenderPassType>(pass))).getProgram();
                shader->Uniform("_waterShininess", _shininess);
                shader->Uniform("_noiseFactor", _noiseFactor);
                shader->Uniform("_noiseTile", _noiseTile);
            }
        }
        _paramsDirty = false;
    }
    
    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}

bool WaterPlane::pointUnderwater(const SceneGraphNode& sgn, const vec3<F32>& point) {
    return sgn.get<BoundsComponent>()->getBoundingBox().containsPoint(point);
}

bool WaterPlane::onRender(const RenderStagePass& renderStagePass) {
    return _plane->onRender(renderStagePass);
}

void WaterPlane::initialiseDrawCommands(SceneGraphNode& sgn,
                                        const RenderStagePass& renderStagePass,
                                        GenericDrawCommands& drawCommandsInOut) {
    GenericDrawCommand cmd;
    cmd.primitiveType(PrimitiveType::TRIANGLE_STRIP);
    cmd.sourceBuffer(_plane->getGeometryVB());
    cmd.cmd().indexCount = to_U32(_plane->getGeometryVB()->getIndexCount());
    drawCommandsInOut.push_back(cmd);

    SceneNode::initialiseDrawCommands(sgn, renderStagePass, drawCommandsInOut);
}

void WaterPlane::updateDrawCommands(SceneGraphNode& sgn,
                                    const RenderStagePass& renderStagePass,
                                    const SceneRenderState& sceneRenderState,
                                    GenericDrawCommands& drawCommandsInOut) {
    SceneNode::updateDrawCommands(sgn, renderStagePass, sceneRenderState, drawCommandsInOut);
}

/// update water refraction
void WaterPlane::updateRefraction(RenderCbkParams& renderParams) {
    // If we are above water, process the plane's refraction.
    // If we are below, we render the scene normally
    bool underwater = pointUnderwater(renderParams._sgn, renderParams._camera->getEye());
    Plane<F32> refractionPlane;
    updatePlaneEquation(renderParams._sgn, refractionPlane, underwater);
    refractionPlane.active(true);

    RenderPassManager::PassParams params;
    params.doPrePass = true;
    params.occlusionCull = false;
    params.camera = renderParams._camera;
    params.stage = RenderStage::REFRACTION;
    params.target = renderParams._renderTarget;
    params.drawPolicy = params.doPrePass ? &RenderTarget::defaultPolicyKeepDepth() : &RenderTarget::defaultPolicy();
    params.pass = renderParams._passIndex;
    params.clippingPlanes[to_U32(underwater ? g_reflectionClipID : g_refractionClipID)] = refractionPlane;
    renderParams._context.parent().renderPassManager().doCustomPass(params);
}

/// Update water reflections
void WaterPlane::updateReflection(RenderCbkParams& renderParams) {
    // If we are above water, process the plane's refraction.
    // If we are below, we render the scene normally
    bool underwater = pointUnderwater(renderParams._sgn, renderParams._camera->getEye());

    Plane<F32> reflectionPlane;
    updatePlaneEquation(renderParams._sgn, reflectionPlane, !underwater);
    reflectionPlane.active(true);

    // Reset reflection cam
    _reflectionCam->fromCamera(*renderParams._camera);
    if (!underwater) {
        _reflectionCam->reflect(reflectionPlane);
    }

    RenderPassManager::PassParams params;
    params.doPrePass = true;
    params.occlusionCull = false;
    params.camera = _reflectionCam;
    params.stage = RenderStage::REFLECTION;
    params.target = renderParams._renderTarget;
    params.drawPolicy = params.doPrePass ? &RenderTarget::defaultPolicyKeepDepth() : &RenderTarget::defaultPolicy();
    params.pass = renderParams._passIndex;
    params.clippingPlanes[to_U32(g_reflectionClipID)].active(true);
    params.clippingPlanes[to_U32(underwater ? g_refractionClipID : g_reflectionClipID)] = reflectionPlane;
    renderParams._context.parent().renderPassManager().doCustomPass(params);
}

void WaterPlane::updatePlaneEquation(const SceneGraphNode& sgn, Plane<F32>& plane, bool reflection) {
    F32 waterLevel = sgn.get<PhysicsComponent>()->getPosition().y;
    const Quaternion<F32>& orientation = sgn.get<PhysicsComponent>()->getOrientation();

    vec3<F32> normal(orientation * (reflection ? WORLD_Y_AXIS : WORLD_Y_NEG_AXIS));
    normal.normalize();
    plane.set(normal, -waterLevel);
    plane.active(false);
}

};