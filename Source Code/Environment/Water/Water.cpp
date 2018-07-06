#include "Headers/Water.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Geometry/Material/Headers/Material.h"

#pragma message("ToDo: check water visibility - Ionut")

WaterPlane::WaterPlane() : SceneNode(TYPE_WATER), Reflector(TYPE_WATER_SURFACE,vec2<U16>(1024,1024)),
                           _plane(NULL),_texture(NULL), _shader(NULL),_planeTransform(NULL),
                           _node(NULL),_planeSGN(NULL),_waterLevel(0),_waterDepth(0),_clippingPlaneID(-1),_reflectionRendering(false){}

void WaterPlane::postLoad(SceneGraphNode* const sgn){
    assert(_texture && _shader && _plane);
    _node = sgn;

    _farPlane = 2.0f * ParamHandler::getInstance().getParam<F32>("runtime.zFar");
    _plane->setCorner(Quad3D::TOP_LEFT,     vec3<F32>(-_farPlane, 0, -_farPlane));
    _plane->setCorner(Quad3D::TOP_RIGHT,    vec3<F32>( _farPlane, 0, -_farPlane));
    _plane->setCorner(Quad3D::BOTTOM_LEFT,  vec3<F32>(-_farPlane, 0,  _farPlane));
    _plane->setCorner(Quad3D::BOTTOM_RIGHT, vec3<F32>( _farPlane, 0,  _farPlane));
    _plane->getSceneNodeRenderState().setDrawState(false);
    _plane->setCustomShader(_shader);
    _plane->renderInstance()->preDraw(true);
    _planeSGN = _node->addNode(_plane);
    _planeSGN->setActive(false);
    _planeTransform = _planeSGN->getTransform();
    _plane->renderInstance()->transform(_planeTransform);
    //The water doesn't cast shadows, doesn't need ambient occlusion and doesn't have real "depth"
    getSceneNodeRenderState().addToDrawExclusionMask(SHADOW_STAGE);

    _shader->UniformTexture("texWaterNoiseNM", 0);
    _shader->UniformTexture("texWaterReflection", 1);
    _shader->UniformTexture("texWaterRefraction", 2);
}

bool WaterPlane::computeBoundingBox(SceneGraphNode* const sgn){
    BoundingBox& bb = _node->getBoundingBox();

    if(bb.isComputed())
        return true;

    _waterLevel = GET_ACTIVE_SCENE()->state().getWaterLevel();
    _waterDepth = GET_ACTIVE_SCENE()->state().getWaterDepth();
    _planeTransform->setPositionY(_waterLevel);
    _planeDirty = true;
    bb.set(vec3<F32>(-_farPlane,_waterLevel - _waterDepth, -_farPlane),
           vec3<F32>(_farPlane, _waterLevel, _farPlane));
    _planeSGN->getBoundingBox().Add(bb);
    PRINT_FN(Locale::get("WATER_CREATE_DETAILS_1"), bb.getMax().y)
    PRINT_FN(Locale::get("WATER_CREATE_DETAILS_2"), bb.getMin().y);

    return SceneNode::computeBoundingBox(sgn);
}

bool WaterPlane::unload(){
    bool state = false;
    state = SceneNode::unload();
    return state;
}

void WaterPlane::setParams(F32 shininess, const vec2<F32>& noiseTile, const vec2<F32>& noiseFactor, F32 transparency){
    _shader->Uniform("_waterShininess",   shininess   );
    _shader->Uniform("_noiseFactor",      noiseFactor );
    _shader->Uniform("_noiseTile",        noiseTile   );
    _shader->Uniform("_transparencyBias", transparency);
}

void WaterPlane::onDraw(const RenderStage& currentStage){
    const vec3<F32>& newEye = GET_ACTIVE_SCENE()->renderState().getCamera().getEye();

    if(newEye != _eyePos){
        _eyeDiff.set(_eyePos.xz() - newEye.xz());
        _eyePos.set(newEye);
        _planeTransform->translateX(_eyeDiff.x);
        _planeTransform->translateZ(_eyeDiff.y);
        BoundingBox& bb = _node->getBoundingBox();
        bb.Translate(vec3<F32>(-_eyeDiff.x,0,-_eyeDiff.y));
        _shader->Uniform("water_bb_min", bb.getMin());
        _shader->Uniform("water_bb_diff",bb.getMax() - bb.getMin());
        _shader->Uniform("underwater",isPointUnderWater(_eyePos));
    }
}

void WaterPlane::postDraw(const RenderStage& currentStage){
}

void WaterPlane::prepareMaterial(SceneGraphNode* const sgn){
    //Prepare the main light (directional light only, sun) for now.
    //Find the most influental light for the terrain. Usually the Sun
    _lightCount = LightManager::getInstance().findLightsForSceneNode(sgn,LIGHT_TYPE_DIRECTIONAL);
    //Update lights for this node
    LightManager::getInstance().update();
    //Only 1 shadow map for terrains

    if(GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE)){
        U8 offset = Config::MAX_TEXTURE_STORAGE;
        CLAMP<U8>(_lightCount, 0, 1);
        for(U8 n = 0; n < _lightCount; n++, offset++){
            Light* l = LightManager::getInstance().getLightForCurrentNode(n);
            LightManager::getInstance().bindDepthMaps(l, n, offset);
        }
    }

    SET_STATE_BLOCK(getMaterial()->getRenderState(FINAL_STAGE));

    _texture->Bind(0);
    _reflectedTexture->Bind(1);
    //_refractedTexture->Bind(2);
    _shader->bind();
    _shader->Uniform("material",getMaterial()->getMaterialMatrix());
    _shader->Uniform("dvd_isReflection", GFX_DEVICE.isCurrentRenderStage(REFLECTION_STAGE));
}

void WaterPlane::releaseMaterial(){
    //_refractedTexture->Unbind(2);
    _reflectedTexture->Unbind(1);
    _texture->Unbind(0);
    if(GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE)){
        U8 offset = (_lightCount - 1) + Config::MAX_TEXTURE_STORAGE;
        for(I32 n = _lightCount - 1; n >= 0; n--,offset--){
            Light* l = LightManager::getInstance().getLightForCurrentNode(n);
            LightManager::getInstance().unbindDepthMaps(l, offset);
        }
    }
}

void WaterPlane::render(SceneGraphNode* const sgn){
    GFX_DEVICE.renderInstance(_plane->renderInstance());
}

bool WaterPlane::getDrawState(const RenderStage& currentStage)  const {
    // Wait for the Reflector to update
    if(!_createdFBO) return false;

    // Make sure we are not drawing ourself unless this is desired
    if((currentStage == REFLECTION_STAGE || _reflectionRendering) && !_updateSelf)	return false;

    // Else, process normal exclusion
    return SceneNode::getDrawState(currentStage);
}

/// update water refraction
void WaterPlane::updateRefraction(){
    // Early out check for render callback
    //if(_renderCallback.empty()) return;
    //_refractionRendering = true;
    // bind the refractive texture
    //_refractionTexture->Begin();
        // render to the reflective texture
        //_renderCallback();
    //_refractionTexture->End();
    //_refractionRendering = false;
}

/// Update water reflections
void WaterPlane::updateReflection(){
    // Early out check for render callback
    if(_renderCallback.empty()) return;
    //ToDo:: this will cause problems later with multiple reflectors. Fix it! -Ionut
    _reflectionRendering = true;
    // If we are above water, process the plane's reflection. If we are below, we render the scene normally
    bool underwater = isPointUnderWater(_eyePos);

    if(!underwater){
        GFX_DEVICE.setRenderStage(REFLECTION_STAGE);
        GFX_DEVICE.enableClipPlane(_clippingPlaneID);
    }

    _reflectedTexture->Begin(FrameBufferObject::defaultPolicy());
        _renderCallback(); //< render to the reflective texture
    _reflectedTexture->End();

    if(!underwater){
        GFX_DEVICE.disableClipPlane(_clippingPlaneID);
        GFX_DEVICE.setPreviousRenderStage();
    }
    _reflectionRendering = false;
}

void WaterPlane::updatePlaneEquation(){
    _absNormal = _planeTransform->getOrientation() * WORLD_Y_AXIS;
    _absNormal.normalize();
    _reflectionPlane.set(_absNormal,_waterLevel);
    _reflectionPlane.active(false);
    if(_clippingPlaneID == -1){
        _clippingPlaneID = GFX_DEVICE.addClipPlane(_reflectionPlane);
    }else{
        GFX_DEVICE.setClipPlane(_clippingPlaneID,_reflectionPlane);
    }
}