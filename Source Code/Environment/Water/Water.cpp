#include "Headers/Water.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Geometry/Material/Headers/Material.h"

#pragma message("ToDo: check water visibility - Ionut")

WaterPlane::WaterPlane() : SceneNode(TYPE_WATER), Reflector(TYPE_WATER_SURFACE,vec2<U16>(1024,1024)),
						   _plane(NULL),_texture(NULL), _shader(NULL),_planeTransform(NULL),
						   _node(NULL),_planeSGN(NULL),_waterLevel(0),_reflectionRendering(false){}

void WaterPlane::postLoad(SceneGraphNode* const sgn){
    assert(_texture && _shader && _plane);
    _node = sgn;

	_farPlane = 2.0f * ParamHandler::getInstance().getParam<F32>("runtime.zFar");
	_plane->setCorner(Quad3D::TOP_LEFT, vec3<F32>(   -_farPlane, 0, -_farPlane));
	_plane->setCorner(Quad3D::TOP_RIGHT, vec3<F32>(   _farPlane, 0, -_farPlane));
	_plane->setCorner(Quad3D::BOTTOM_LEFT, vec3<F32>(-_farPlane, 0,  _farPlane));
	_plane->setCorner(Quad3D::BOTTOM_RIGHT, vec3<F32>(_farPlane, 0,  _farPlane));
	_plane->getSceneNodeRenderState().setDrawState(false);
	_plane->setCustomShader(_shader);
	_plane->renderInstance()->preDraw(true);
	_planeSGN = _node->addNode(_plane);
    _planeSGN->setActive(false);
	_planeTransform = _planeSGN->getTransform();
	_plane->renderInstance()->transform(_planeTransform);
	///The water doesn't cast shadows, doesn't need ambient occlusion and doesn't have real "depth"
	getSceneNodeRenderState().addToDrawExclusionMask(DEPTH_STAGE);

	_shader->UniformTexture("texWaterReflection", 0);
	_shader->UniformTexture("texWaterNoiseNM", 1);
}

bool WaterPlane::computeBoundingBox(SceneGraphNode* const sgn){
	BoundingBox& bb = _node->getBoundingBox();
	if(bb.isComputed()) return true;
	_waterLevel = GET_ACTIVE_SCENE()->state()->getWaterLevel();
	F32 waterDepth = GET_ACTIVE_SCENE()->state()->getWaterDepth();
	bb.set(vec3<F32>(-_farPlane,_waterLevel - waterDepth, -_farPlane),vec3<F32>(_farPlane, _waterLevel, _farPlane));
	_planeSGN->getBoundingBox().Add(bb);
	PRINT_FN(Locale::get("WATER_CREATE_DETAILS_1"), bb.getMax().y)
	PRINT_FN(Locale::get("WATER_CREATE_DETAILS_2"), bb.getMin().y);
	bool state = SceneNode::computeBoundingBox(sgn);

	_shader->Uniform("water_bb_min",bb.getMin());
	_shader->Uniform("water_bb_max",bb.getMax());

	return state;
}

bool WaterPlane::unload(){
	bool state = false;
	state = SceneNode::unload();
	return state;
}

void WaterPlane::setParams(F32 shininess, F32 noiseTile, F32 noiseFactor, F32 transparency){

	_shader->Uniform("water_shininess",shininess );
	_shader->Uniform("noise_tile", noiseTile );
	_shader->Uniform("noise_factor", noiseFactor);
}

void WaterPlane::onDraw(const RenderStage& currentStage){
    _eyePos = GET_ACTIVE_SCENE()->renderState()->getCamera()->getEye();
	BoundingBox& bb = _node->getBoundingBox();
	_planeTransform->setPosition(vec3<F32>(_eyePos.x,bb.getMax().y,_eyePos.z));
}

void WaterPlane::postDraw(const RenderStage& currentStage){
}

void WaterPlane::prepareMaterial(SceneGraphNode* const sgn){
	//Prepare the main light (directional light only, sun) for now.
	if(!GFX_DEVICE.isCurrentRenderStage(DEPTH_STAGE)){
		//Find the most influental light for the terrain. Usually the Sun
		_lightCount = LightManager::getInstance().findLightsForSceneNode(sgn,LIGHT_TYPE_DIRECTIONAL);
		//Update lights for this node
		LightManager::getInstance().update();
		//Only 1 shadow map for terrains
	}
	CLAMP<U8>(_lightCount, 0, 1);
	if(GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE)){
		U8 offset = 9;
		for(U8 n = 0; n < _lightCount; n++, offset++){
			Light* l = LightManager::getInstance().getLightForCurrentNode(n);
			LightManager::getInstance().bindDepthMaps(l, n, offset);
		}
	}
	SET_STATE_BLOCK(getMaterial()->getRenderState(FINAL_STAGE));

	_reflectedTexture->Bind(0);
	_texture->Bind(1);
	_shader->bind();
	_shader->Uniform("underwater",isPointUnderWater(_eyePos));
    _shader->Uniform("material",getMaterial()->getMaterialMatrix());
	_shader->UniformTexture("texWaterReflection", 0);
	_shader->UniformTexture("texWaterNoiseNM", 1);
}

void WaterPlane::releaseMaterial(){
	_texture->Unbind(1);
	_reflectedTexture->Unbind(0);

	if(GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE)){
		U8 offset = (_lightCount - 1) + 9;
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
	if(!_createdFBO){
		return false;
	}
	// Make sure we are not drawing ourself
	if((currentStage == REFLECTION_STAGE || _reflectionRendering) && !_updateSelf){
		// unless this is desired
		return false;
	}
	// Else, process normal exclusion
	return SceneNode::getDrawState(currentStage);
}

/// Update water reflections
void WaterPlane::updateReflection(){
	// Early out check for render callback
	if(_renderCallback.empty()) return;
	_reflectionRendering = true;
	// If we are above water, process the plane's reflection. If we are below, we render the scene normally
	// Set the correct stage
	RenderStage prev = GFX_DEVICE.getRenderStage();
	if(!isPointUnderWater(_eyePos)){
		GFX_DEVICE.setRenderStage(REFLECTION_STAGE);
	}

	// bind the reflective texture
	_reflectedTexture->Begin();
		// render to the reflective texture
		_renderCallback();
	_reflectedTexture->End();

	GFX_DEVICE.setRenderStage(prev);
	_reflectionRendering = false;
}