#include "Headers/Water.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Geometry/Material/Headers/Material.h"


WaterPlane::WaterPlane() : SceneNode(TYPE_WATER), Reflector(TYPE_WATER_SURFACE,vec2<U16>(1024,1024)),
                           _plane(nullptr),  _planeTransform(nullptr), _node(nullptr), _planeSGN(nullptr), _waterLevel(0), _waterDepth(0), _refractionPlaneID(-1), 
                           _reflectionPlaneID(-1), _reflectionRendering(false), _refractionRendering(false), _refractionTexture(nullptr), _dirty(true), _cameraUnderWater(false),
                           _cameraMgr(Application::getInstance().getKernel()->getCameraMgr())
{
    //Set water plane to be single-sided
    P32 quadMask;
    quadMask.i = 0;
    quadMask.b.b0 = true;
    ResourceDescriptor waterPlane("waterPlane");
    waterPlane.setFlag(true); //No default material
    waterPlane.setBoolMask(quadMask);
    _plane = CreateResource<Quad3D>(waterPlane);
    
}

void WaterPlane::postLoad(SceneGraphNode* const sgn){
    _node = sgn;

    ShaderProgram* shader = getMaterial()->getShaderProgram(FINAL_STAGE);

    _farPlane = 2.0f * GET_ACTIVE_SCENE()->state().getRenderState().getCameraConst().getZPlanes().y;
    _plane->setCorner(Quad3D::TOP_LEFT,     vec3<F32>(-_farPlane * 1.5f, 0, -_farPlane * 1.5f));
    _plane->setCorner(Quad3D::TOP_RIGHT,    vec3<F32>( _farPlane * 1.5f, 0, -_farPlane * 1.5f));
    _plane->setCorner(Quad3D::BOTTOM_LEFT,  vec3<F32>(-_farPlane * 1.5f, 0,  _farPlane * 1.5f));
    _plane->setCorner(Quad3D::BOTTOM_RIGHT, vec3<F32>( _farPlane * 1.5f, 0,  _farPlane * 1.5f));
    _plane->setNormal(Quad3D::CORNER_ALL, WORLD_Y_AXIS);
    _plane->getSceneNodeRenderState().setDrawState(false);
    _plane->setCustomShader(shader);
    _plane->renderInstance()->preDraw(true);
    _planeSGN = _node->addNode(_plane);
    _planeSGN->setActive(false);
    _planeTransform = _planeSGN->getTransform();
    _plane->renderInstance()->transform(_planeTransform);
    //The water doesn't cast shadows, doesn't need ambient occlusion and doesn't have real "depth"
    getSceneNodeRenderState().addToDrawExclusionMask(SHADOW_STAGE);

    shader->UniformTexture("texWaterNoiseNM", 0);
    shader->UniformTexture("texWaterReflection", 1);
    shader->UniformTexture("texWaterRefraction", 2);
    shader->UniformTexture("texWaterNoiseDUDV", 3);
    _dirty = true;

    PRINT_FN(Locale::get("REFRACTION_INIT_FB"), _resolution.x, _resolution.y);
    SamplerDescriptor refractionSampler;
    refractionSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    refractionSampler.toggleMipMaps(false);

    TextureDescriptor refractionDescriptor(TEXTURE_2D, RGBA8, UNSIGNED_BYTE); //Less precision for reflections
    refractionDescriptor.setSampler(refractionSampler);

    _refractionTexture = GFX_DEVICE.newFB();
    _refractionTexture->AddAttachment(refractionDescriptor, TextureDescriptor::Color0);
    _refractionTexture->toggleDepthBuffer(true);
    _refractionTexture->Create(_resolution.x, _resolution.y);

    SceneNode::postLoad(sgn);
}

bool WaterPlane::computeBoundingBox(SceneGraphNode* const sgn){
    BoundingBox& bb = _node->getBoundingBox();

    if(bb.isComputed())
        return true;

    _waterLevel = GET_ACTIVE_SCENE()->state().getWaterLevel();
    _waterDepth = GET_ACTIVE_SCENE()->state().getWaterDepth();
    _planeTransform->setPositionY(_waterLevel);
    _planeDirty = true;
    bb.set(vec3<F32>(-_farPlane,_waterLevel - _waterDepth, -_farPlane), vec3<F32>(_farPlane, _waterLevel, _farPlane));
    _planeSGN->getBoundingBox().Add(bb);
    PRINT_FN(Locale::get("WATER_CREATE_DETAILS_1"), bb.getMax().y)
    PRINT_FN(Locale::get("WATER_CREATE_DETAILS_2"), bb.getMin().y);
    _dirty = true;
    return SceneNode::computeBoundingBox(sgn);
}

bool WaterPlane::unload(){
    bool state = false;
    state = SceneNode::unload();
    return state;
}

void WaterPlane::setParams(F32 shininess, const vec2<F32>& noiseTile, const vec2<F32>& noiseFactor, F32 transparency){
    ShaderProgram* shader = getMaterial()->getShaderProgram(FINAL_STAGE);
    shader->Uniform("_waterShininess",   shininess   );
    shader->Uniform("_noiseFactor",      noiseFactor );
    shader->Uniform("_noiseTile",        noiseTile   );
    shader->Uniform("_transparencyBias", transparency);
}

void WaterPlane::sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState){
    _eyePos = sceneState.getRenderState().getCamera().getEye();
    _cameraUnderWater = isPointUnderWater(_eyePos);
}

bool WaterPlane::onDraw(SceneGraphNode* const sgn, const RenderStage& currentStage){
    if(_dirty){
        const vec3<F32>& newEye = GET_ACTIVE_SCENE()->renderState().getCamera().getEye();
        _eyeDiff.set(_eyePos.xz() - newEye.xz());
        _eyePos.set(newEye);
        _planeTransform->translateX(_eyeDiff.x);
        _planeTransform->translateZ(_eyeDiff.y);
        BoundingBox& bb = _node->getBoundingBox();
        _node->getBoundingSphere().fromBoundingBox(bb);
        bb.Translate(vec3<F32>(-_eyeDiff.x,0,-_eyeDiff.y));
        _dirty = false;
    }
    SET_STATE_BLOCK(getMaterial()->getRenderStateBlock(FINAL_STAGE));
    return true;
}

void WaterPlane::postDraw(SceneGraphNode* const sgn, const RenderStage& currentStage){
}

bool WaterPlane::prepareMaterial(SceneGraphNode* const sgn){

    Material* waterMat = getMaterial();
    _drawShader = waterMat->getShaderProgram();
    waterMat->getTexture(Material::TEXTURE_UNIT0)->Bind(0);
    _reflectedTexture->Bind(1);
    if (!_cameraUnderWater){
        _refractionTexture->Bind(2);
    }

    _drawShader->Uniform("underwater", _cameraUnderWater);
    _drawShader->Uniform("material", getMaterial()->getMaterialMatrix());

    _plane->setCustomShader(_drawShader);
    return _drawShader->bind();
}

bool WaterPlane::prepareDepthMaterial(SceneGraphNode* const sgn){ 
    _drawShader = getMaterial()->getShaderProgram(Z_PRE_PASS_STAGE);
    _plane->setCustomShader(_drawShader);
    return _drawShader->bind();
}

void WaterPlane::render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState){
    GFX_DEVICE.renderInstance(_plane->renderInstance());
}

bool WaterPlane::getDrawState(const RenderStage& currentStage)  const {
    // Wait for the Reflector to update
    if(!_createdFB) return false;

    // Make sure we are not drawing ourself unless this is desired
    if((currentStage == REFLECTION_STAGE || _reflectionRendering || _refractionRendering) && !_updateSelf)	return false;

    // Else, process normal exclusion
    return SceneNode::getDrawState(currentStage);
}

/// update water refraction
void WaterPlane::updateRefraction(){
    if (_cameraUnderWater)
        return;

    // Early out check for render callback
    if(_renderCallback.empty()) return;

    _refractionRendering = true;
    // If we are above water, process the plane's reflection. If we are below, we render the scene normally
    RenderStage prevRenderStage = GFX_DEVICE.setRenderStage(FINAL_STAGE);
    GFX_DEVICE.enableClipPlane(_refractionPlaneID);
    _cameraMgr.getActiveCamera()->renderLookAt();
    // bind the refractive texture
    _refractionTexture->Begin(FrameBuffer::defaultPolicy());
        // render to the reflective texture
        _refractionCallback();
    _refractionTexture->End();

    GFX_DEVICE.disableClipPlane(_refractionPlaneID);
    GFX_DEVICE.setRenderStage(prevRenderStage);
    
    _refractionRendering = false;
}

/// Update water reflections
void WaterPlane::updateReflection(){
    // Early out check for render callback
    if (!_renderCallback.empty()){
        //ToDo: this will cause problems later with multiple reflectors. Fix it! -Ionut
        _reflectionRendering = true;
    
        RenderStage prevRenderStage = GFX_DEVICE.setRenderStage(_cameraUnderWater ? FINAL_STAGE : REFLECTION_STAGE);
        GFX_DEVICE.enableClipPlane(_reflectionPlaneID);

        _cameraUnderWater ? _cameraMgr.getActiveCamera()->renderLookAt() : _cameraMgr.getActiveCamera()->renderLookAtReflected(getReflectionPlane());

        _reflectedTexture->Begin(FrameBuffer::defaultPolicy());
            _renderCallback(); //< render to the reflective texture
        _reflectedTexture->End();

        GFX_DEVICE.disableClipPlane(_reflectionPlaneID);
        GFX_DEVICE.setRenderStage(prevRenderStage);
    
        _reflectionRendering = false;
    }
    updateRefraction();
}

void WaterPlane::updatePlaneEquation(){
    const Quaternion<F32>& orientation = _planeTransform->getOrientation();
    vec3<F32> reflectionNormal(orientation * WORLD_Y_AXIS);
    reflectionNormal.normalize();
    vec3<F32> refractionNormal(orientation * WORLD_Y_NEG_AXIS);
    refractionNormal.normalize();
    _reflectionPlane.set(reflectionNormal, -_waterLevel);
    _reflectionPlane.active(false);
    _refractionPlane.set(refractionNormal, -_waterLevel);
    _refractionPlane.active(false);
    if(_reflectionPlaneID == -1){
        _reflectionPlaneID = GFX_DEVICE.addClipPlane(_reflectionPlane, CLIP_PLANE_0);
        _refractionPlaneID = GFX_DEVICE.addClipPlane(_refractionPlane, CLIP_PLANE_1);
    }else{
        GFX_DEVICE.setClipPlane(_reflectionPlaneID, _reflectionPlane);
        GFX_DEVICE.setClipPlane(_refractionPlaneID, _refractionPlane);
    }
    
    _dirty = true;
}

bool WaterPlane::previewReflection(){

    if (_previewReflection) {
        F32 height = _resolution.y * 0.333f;
        _previewReflectionShader->bind();
        _refractionTexture->Bind();
        GFX_DEVICE.renderInViewport(vec4<I32>(_resolution.x  * 0.333f, Application::getInstance().getResolution().y - height, _resolution.x  * 0.666f, height), DELEGATE_BIND(&GFXDevice::drawPoints, DELEGATE_REF(GFX_DEVICE), 1));
    }

    return Reflector::previewReflection();
}