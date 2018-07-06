#include "Headers/Water.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

namespace {
    ClipPlaneIndex g_reflectionClipID = ClipPlaneIndex::CLIP_PLANE_0;
    ClipPlaneIndex g_refractionClipID = ClipPlaneIndex::CLIP_PLANE_1;
};

WaterPlane::WaterPlane(const stringImpl& name, I32 sideLength)
    : SceneNode(name, SceneNodeType::TYPE_WATER),
      _plane(nullptr),
      _sideLength(std::max(sideLength, 1)),
      _reflectionRendering(false),
      _refractionRendering(false),
      _updateSelf(false),
      _paramsDirty(true),
      _excludeSelfReflection(true),
      _cameraMgr(Application::instance().kernel().getCameraMgr()) 
{
    // Set water plane to be single-sided
    P32 quadMask;
    quadMask.i = 0;
    quadMask.b[0] = true;

    setParams(50.0f, vec2<F32>(10.0f, 10.0f), vec2<F32>(0.1f, 0.1f), 0.34f);

    ResourceDescriptor waterPlane("waterPlane");
    waterPlane.setFlag(true);  // No default material
    waterPlane.setBoolMask(quadMask);
    _plane = CreateResource<Quad3D>(waterPlane);

    // The water doesn't cast shadows, doesn't need ambient occlusion and
    // doesn't have real "depth"
    renderState().addToDrawExclusionMask(RenderStage::SHADOW);
    Console::printfn(Locale::get(_ID("REFRACTION_INIT_FB")), nextPOW2(sideLength), nextPOW2(sideLength));

    setFlag(UpdateFlag::BOUNDS_CHANGED);
}

WaterPlane::~WaterPlane()
{
}

void WaterPlane::postLoad(SceneGraphNode& sgn) {
    static const U32 normalMask = to_const_uint(SGNComponent::ComponentType::NAVIGATION) |
                                  to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                  to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                  to_const_uint(SGNComponent::ComponentType::RENDERING);
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
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    std::placeholders::_3));
    renderable->setRefractionCallback(DELEGATE_BIND(&WaterPlane::updateRefraction,
                                                    this,
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    std::placeholders::_3));
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

bool WaterPlane::cameraUnderwater(const SceneGraphNode& sgn, const SceneRenderState& sceneRenderState) {
    return sgn.get<BoundsComponent>()->getBoundingBox().containsPoint(sceneRenderState.getCameraConst().getEye());
}

bool WaterPlane::onRender(SceneGraphNode& sgn, RenderStage currentStage) {
    if (currentStage == RenderStage::REFLECTION) {
        // mark ourselves as reflection target only if we do not wish to reflect
        // ourself back
        _updateSelf = !_excludeSelfReflection;
    } else {
        // unmark from reflection target
        _updateSelf = true;
    }

    return _plane->onRender(currentStage);
}

bool WaterPlane::getDrawCommands(SceneGraphNode& sgn,
                                 RenderStage renderStage,
                                 const SceneRenderState& sceneRenderState,
                                 vectorImpl<GenericDrawCommand>& drawCommandsOut) {

    RenderingComponent* const renderable = sgn.get<RenderingComponent>();
    assert(renderable != nullptr);

    const ShaderProgram_ptr& drawShader =
        renderable->getDrawShader(GFX_DEVICE.isDepthStage() ? RenderStage::Z_PRE_PASS : RenderStage::DISPLAY);

    drawShader->Uniform("underwater", cameraUnderwater(sgn, sceneRenderState));

    GenericDrawCommand& cmd = drawCommandsOut.front();
    cmd.primitiveType(PrimitiveType::TRIANGLE_STRIP);
    cmd.renderGeometry(renderable->renderGeometry());
    cmd.renderWireframe(renderable->renderWireframe());
    cmd.stateHash(renderable->getMaterialInstance()->getRenderStateBlock(RenderStage::DISPLAY));
    cmd.shaderProgram(drawShader);
    cmd.sourceBuffer(_plane->getGeometryVB());

    return SceneNode::getDrawCommands(sgn, renderStage, sceneRenderState, drawCommandsOut);
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
void WaterPlane::updateRefraction(const SceneGraphNode& sgn,
                                  const SceneRenderState& sceneRenderState,
                                  RenderTarget& renderTarget) {
    if (cameraUnderwater(sgn, sceneRenderState)) {
        return;
    }
    // We need a rendering method to create refractions
    _refractionRendering = true;
    // If we are above water, process the plane's refraction.
    // If we are below, we render the scene normally
    RenderStage prevRenderStage = GFX_DEVICE.setRenderStage(RenderStage::DISPLAY);
    GFX_DEVICE.toggleClipPlane(g_refractionClipID, true);
    _cameraMgr.getActiveCamera().renderLookAt();
    // bind the refractive texture
    renderTarget.begin(RenderTarget::defaultPolicy());
    // render to the reflective texture
    SceneManager::instance().renderVisibleNodes(RenderStage::DISPLAY, true, 0);
    renderTarget.end();

    GFX_DEVICE.toggleClipPlane(g_refractionClipID, false);
    GFX_DEVICE.setRenderStage(prevRenderStage);

    _refractionRendering = false;
}

/// Update water reflections
void WaterPlane::updateReflection(const SceneGraphNode& sgn,
                                  const SceneRenderState& sceneRenderState,
                                  RenderTarget& renderTarget) {
    // ToDo: this will cause problems later with multiple reflectors.
    // Fix it! -Ionut
    _reflectionRendering = true;

    Plane<F32> reflectionPlane, refractionPlane;
    updatePlaneEquation(sgn, reflectionPlane, refractionPlane);


    GFX_DEVICE.setClipPlane(ClipPlaneIndex::CLIP_PLANE_0, reflectionPlane);
    GFX_DEVICE.setClipPlane(ClipPlaneIndex::CLIP_PLANE_1, refractionPlane);

    bool underwater = cameraUnderwater(sgn, sceneRenderState);
    RenderStage prevRenderStage = GFX_DEVICE.setRenderStage(underwater ? RenderStage::DISPLAY : RenderStage::REFLECTION);
    GFX_DEVICE.toggleClipPlane(g_reflectionClipID, true);

    underwater ? _cameraMgr.getActiveCamera().renderLookAt()
               : _cameraMgr.getActiveCamera().renderLookAtReflected(reflectionPlane);

    renderTarget.begin(RenderTarget::defaultPolicy());
    SceneManager::instance().renderVisibleNodes(RenderStage::REFLECTION, true, 0);
    renderTarget.end();

    GFX_DEVICE.toggleClipPlane(g_reflectionClipID, false);
    GFX_DEVICE.setRenderStage(prevRenderStage);

    _reflectionRendering = false;
}

void WaterPlane::updatePlaneEquation(const SceneGraphNode& sgn, Plane<F32>& reflectionPlane, Plane<F32>& refractionPlane) {
    F32 waterLevel = sgn.get<PhysicsComponent>()->getPosition().y;
    const Quaternion<F32>& orientation = sgn.get<PhysicsComponent>()->getOrientation();

    vec3<F32> reflectionNormal(orientation * WORLD_Y_AXIS);
    reflectionNormal.normalize();

    vec3<F32> refractionNormal(orientation * WORLD_Y_NEG_AXIS);
    refractionNormal.normalize();

    reflectionPlane.set(reflectionNormal, -waterLevel);
    reflectionPlane.active(false);

    refractionPlane.set(refractionNormal, -waterLevel);
    refractionPlane.active(false);
}

};