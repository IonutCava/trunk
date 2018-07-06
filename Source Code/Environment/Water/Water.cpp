#include "Headers/Water.h"

#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

namespace {
    ClipPlaneIndex g_reflectionClipID = ClipPlaneIndex::CLIP_PLANE_0;
    ClipPlaneIndex g_refractionClipID = ClipPlaneIndex::CLIP_PLANE_1;
};

WaterPlane::WaterPlane(ResourceCache& parentCache, const stringImpl& name, I32 sideLength)
    : SceneNode(parentCache, name, SceneNodeType::TYPE_WATER),
      _plane(nullptr),
      _sideLength(std::max(sideLength, 1)),
      _updateSelf(false),
      _paramsDirty(true),
      _excludeSelfReflection(true),
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
    static const U32 normalMask = to_const_uint(SGNComponent::ComponentType::NAVIGATION) |
                                  to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                  to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                  to_const_uint(SGNComponent::ComponentType::RENDERING) |
                                  to_const_uint(SGNComponent::ComponentType::NETWORKING);

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
        const ShaderProgram_ptr& shader =
                rComp->getMaterialInstance()
                     ->getShaderInfo(RenderStage::DISPLAY)
                     .getProgram();
        shader->Uniform("_waterShininess", _shininess);
        shader->Uniform("_noiseFactor", _noiseFactor);
        shader->Uniform("_noiseTile", _noiseTile);
        _paramsDirty = false;
    }
    
    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}

bool WaterPlane::pointUnderwater(const SceneGraphNode& sgn, const vec3<F32>& point) {
    return sgn.get<BoundsComponent>()->getBoundingBox().containsPoint(point);
}

bool WaterPlane::onRender(RenderStage currentStage) {
    if (currentStage == RenderStage::REFLECTION) {
        // mark ourselves as reflection target only if we do not wish to reflect
        // ourself back
        _updateSelf = !_excludeSelfReflection;
        STUBBED("ToDo: Set a reflection pass GUID to match the current reflective node so that we can avoid drawing ourselves -Ionut");
    } else {
        // unmark from reflection target
        _updateSelf = true;
    }

    return _plane->onRender(currentStage);
}

void WaterPlane::initialiseDrawCommands(SceneGraphNode& sgn,
                                        RenderStage renderStage,
                                        GenericDrawCommands& drawCommandsInOut) {
    GenericDrawCommand cmd;
    cmd.primitiveType(PrimitiveType::TRIANGLE_STRIP);
    cmd.sourceBuffer(_plane->getGeometryVB());
    cmd.cmd().indexCount = to_uint(_plane->getGeometryVB()->getIndexCount());
    drawCommandsInOut.push_back(cmd);

    SceneNode::initialiseDrawCommands(sgn, renderStage, drawCommandsInOut);
}

void WaterPlane::updateDrawCommands(SceneGraphNode& sgn,
                                    RenderStage renderStage,
                                    const SceneRenderState& sceneRenderState,
                                    GenericDrawCommands& drawCommandsInOut) {
    SceneNode::updateDrawCommands(sgn, renderStage, sceneRenderState, drawCommandsInOut);
}


bool WaterPlane::getDrawState(RenderStage currentStage) {
    // Make sure we are not drawing our self unless this is desired
    if (currentStage == RenderStage::REFLECTION && !_updateSelf) {
        return false;
    }

    // Else, process normal exclusion
    return SceneNode::getDrawState(currentStage);
}

/// update water refraction
void WaterPlane::updateRefraction(RenderCbkParams& renderParams) {
    GFXDevice& gfx = renderParams._context;
    Plane<F32> refractionPlane;
    updatePlaneEquation(renderParams._sgn, refractionPlane, false);
    gfx.setClipPlane(g_refractionClipID, refractionPlane);

    bool underwater = pointUnderwater(renderParams._sgn, renderParams._camera->getEye());

    // If we are above water, process the plane's refraction.
    // If we are below, we render the scene normally
    RenderPassManager::PassParams params;
    params.doPrePass = true;
    params.occlusionCull = false;
    params.camera = renderParams._camera;
    params.stage = RenderStage::REFRACTION;
    params.target = renderParams._renderTarget;
    params.drawPolicy = params.doPrePass ? &RenderTarget::defaultPolicyKeepDepth() : &RenderTarget::defaultPolicy();
    params.pass = renderParams._passIndex;
    params.clippingPlanes[to_uint(/*underwater ? g_reflectionClipID : */g_refractionClipID)] = true;
    gfx.parent().renderPassManager().doCustomPass(params);
}

/// Update water reflections
void WaterPlane::updateReflection(RenderCbkParams& renderParams) {
    GFXDevice& gfx = renderParams._context;
    Plane<F32> reflectionPlane;
    updatePlaneEquation(renderParams._sgn, reflectionPlane, true);
    gfx.setClipPlane(g_reflectionClipID, reflectionPlane);

    // Reset reflection cam
    _reflectionCam->fromCamera(*renderParams._camera);
    if (!pointUnderwater(renderParams._sgn, renderParams._camera->getEye())) {
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
    params.clippingPlanes[to_uint(g_reflectionClipID)] = true;
    gfx.parent().renderPassManager().doCustomPass(params);
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