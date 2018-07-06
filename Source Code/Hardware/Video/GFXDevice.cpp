#include "Headers/GFXDevice.h"
#include "Headers/RenderStateBlock.h"

#include "GUI/Headers/GUI.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Rendering/Headers/Frustum.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Headers/Renderer.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/RenderPass/Headers/RenderPass.h"

#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"



GFXDevice::GFXDevice() : _api(GL_API::getInstance()) ///<Defaulting to OpenGL if no api has been defined
{
   _prevShaderId = 0;
   _prevTextureId = 0;
   _currentStateBlock = NULL;
   _newStateBlock = NULL;
   _stateBlockDirty = false;
   _renderer = NULL;
   RenderPass* diffusePass = New RenderPass("diffusePass");
   RenderPassManager::getInstance().addRenderPass(diffusePass,1);
   //RenderPassManager::getInstance().addRenderPass(shadowPass,2);
}

void GFXDevice::setApi(RenderAPI api){

	switch(api)	{

	case OpenGL:
	default:
		_api = GL_API::getInstance();
		break;
	case Direct3D:
		_api = DX_API::getInstance();
		break;
	case Software:
	case None:
	case GFX_RENDER_API_PLACEHOLDER: ///< Placeholder
		///Ionut: OpenGL 4.0 and DX 11 in another life maybe :)
		ERROR_FN(Locale::get("ERROR_GFX_DEVICE_API"));
		break;
	};
	_api.setId(api);
}

void GFXDevice::closeRenderingApi(){
	_api.closeRenderingApi();
	for_each(RenderStateMap::value_type& it, _stateBlockMap){
		SAFE_DELETE(it.second);
	}
	_stateBlockMap.clear();
	///Destroy all rendering Passes
	RenderPassManager::getInstance().DestroyInstance();
}

void GFXDevice::closeRenderer(){
	PRINT_FN(Locale::get("CLOSING_RENDERER"));
	SAFE_DELETE(_renderer);
}

void GFXDevice::renderModel(Object3D* const model){
	///All geometry is stored in VBO format
	assert(model != NULL);
	if(_stateBlockDirty){
		updateStates();
	}
	_api.renderModel(model);

}

void GFXDevice::renderElements(PrimitiveType t, GFXDataFormat f, U32 count, const void* first_element){
	if(_stateBlockDirty){
		updateStates();
	}
	_api.renderElements(t,f,count,first_element);
}

void GFXDevice::drawBox3D(const vec3<F32>& min,const vec3<F32>& max, const mat4<F32>& globalOffset){
	if(_stateBlockDirty){
		updateStates();
	}
	_api.drawBox3D(min,max,globalOffset);
}

void GFXDevice::drawLines(const vectorImpl<vec3<F32> >& pointsA,const vectorImpl<vec3<F32> >& pointsB,const vectorImpl<vec4<F32> >& colors, const mat4<F32>& globalOffset){
	if(_stateBlockDirty){
		updateStates();
	}
	_api.drawLines(pointsA,pointsB,colors,globalOffset);
}

void GFXDevice::renderGUIElement(GUIElement* const element){
	if(_stateBlockDirty){
		updateStates();
	}
	if(!element) return; ///< Console not created, for example
	switch(element->getGuiType()){
		case GUI_TEXT:
			drawTextToScreen(element);
			break;
		case GUI_BUTTON:
			drawButton(element);
			break;
		case GUI_FLASH:
			drawFlash(element);
			break;
		default:
			break;
	};

}

void GFXDevice::render(boost::function0<void> renderFunction,SceneRenderState* const sceneRenderState){
	///Call the specific renderfunction that prepares the scene for presentation
	_renderer->render(renderFunction,sceneRenderState);
}

bool GFXDevice::isCurrentRenderStage(U16 renderStageMask){
	assert((renderStageMask & ~(INVALID_STAGE-1)) == 0);
	if(renderStageMask & _renderStage) {return true;}
	return false;
}

void GFXDevice::setRenderer(Renderer* const renderer) {
	SAFE_UPDATE(_renderer,renderer);
	RenderPassManager::getInstance().updateMaterials(true);
}

bool GFXDevice::getDeferredRendering(){
	assert(_renderer != NULL);
	return (_renderer->getType() != RENDERER_FORWARD);
}

void  GFXDevice::generateCubeMap(FrameBufferObject& cubeMap, 
								 Camera* const activeCamera,
								 const vec3<F32>& pos, 
								 boost::function0<void> callback){
	///Don't need to override cubemap rendering callback
	if(callback.empty()){
		SceneGraph* sg = GET_ACTIVE_SCENE()->getSceneGraph();
		///Default case is that everything is reflected
		callback = SCENE_GRAPH_UPDATE(sg);
	}
	///Only use cube map FBO's
	if(cubeMap.getType() != FBO_CUBE_COLOR){
		ERROR_FN(Locale::get("ERROR_GFX_DEVICE_INVALID_FBO_CUBEMAP"));
		return;
	}
	///Get some global vars
	ParamHandler& par = ParamHandler::getInstance();
	///Save our current camera settings
	activeCamera->SaveCamera();
	///And save all camera transform matrices
	lockModelView();
	lockProjection();
	///set a 90 degree vertical FoV perspective projection
	setPerspectiveProjection(90.0,1,Frustum::getInstance().getZPlanes());
	///Save our old rendering stage
	RenderStage prev = getRenderStage();
	///And set the current render stage to 
	setRenderStage(ENVIRONMENT_MAPPING_STAGE);
	///For each of the environment's faces (TOP,DOWN,NORTH,SOUTH,EAST,WEST)
	for(U8 i = 0; i < 6; i++){
		///Set the correct camera orientation and position for current face
		activeCamera->RenderLookAtToCubeMap( pos, i );
		///Bind our FBO's current face
		cubeMap.Begin(i);
			///draw our scene
			render(callback, GET_ACTIVE_SCENE()->renderState());
		///Unbind this face
		cubeMap.End(i);
	}
	///Return to our previous rendering stage
	setRenderStage(prev);
	///Restore transfom matrices
	releaseProjection();
	releaseModelView();
	///And restore camera
	activeCamera->RestoreCamera();
}

RenderStateBlock* GFXDevice::createStateBlock(const RenderStateBlockDescriptor& descriptor){

   U32 hashValue = descriptor.getHash();
   if (_stateBlockMap[hashValue])
      return _stateBlockMap[hashValue];

   RenderStateBlock* result = newRenderStateBlock(descriptor);
   _stateBlockMap.insert(std::make_pair(hashValue,result));

   return result;
}

void GFXDevice::setStateBlock(RenderStateBlock* block) {
   assert(block != NULL);

   if (block != _currentStateBlock) {
      _deviceStateDirty = true;
      _stateBlockDirty = true;
      _newStateBlock = block;
   } else {
      _stateBlockDirty = false;
     _newStateBlock = _currentStateBlock;
   }
}

void GFXDevice::setStateBlockByDesc( const RenderStateBlockDescriptor &desc ){
   RenderStateBlock *block = createStateBlock( desc );
   _stateBlockByDescription = true;
   SET_STATE_BLOCK( block );
}

void GFXDevice::updateStates(bool force) {
	///Verify render states
	if(force){
		if ( _newStateBlock ){
			updateStateInternal(_newStateBlock, true);
		}
		_currentStateBlock = _newStateBlock;
	}
	if (_stateBlockDirty && !force) {
      updateStateInternal(_newStateBlock, force);
      _currentStateBlock = _newStateBlock;
      _stateBlockDirty = false;
   }
	LightManager::getInstance().update();
}

bool GFXDevice::excludeFromStateChange(const SceneNodeType& currentType){
	U16 exclusionMask = TYPE_LIGHT | TYPE_TRIGGER | TYPE_PARTICLE_EMITTER | TYPE_SKY;
	return (exclusionMask & currentType) == currentType ? true : false;
}