#include "Headers/GFXDevice.h"
#include "Headers/RenderStateBlock.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIText.h"
#include "GUI/Headers/GUIFlash.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Rendering/Headers/Frustum.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/ShaderManager.h"
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
   _previousStateBlock = NULL;
   _stateBlockDirty = false;
   _drawDebugAxis = false;
   _renderer = NULL;
   _renderStage = INVALID_STAGE;
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

void GFXDevice::renderInstance(RenderInstance* const instance){
	///All geometry is stored in VBO format
	assert(instance->object3D() != NULL);

	if(instance->preDraw()){
		instance->object3D()->onDraw(getRenderStage());
	}
	
	if(instance->draw2D()){
		//toggle2D(true);
	}
	if(_stateBlockDirty) updateStates();

	_api.renderInstance(instance);
}

void GFXDevice::renderBuffer(VertexBufferObject* const vbo,Transform* const vboTransform){
	if(_stateBlockDirty) updateStates();

	_api.renderBuffer(vbo,vboTransform);
}

void GFXDevice::drawBox3D(const vec3<F32>& min,const vec3<F32>& max, const mat4<F32>& globalOffset){
	_api.drawBox3D(min,max,globalOffset);
}

void GFXDevice::drawLines(const vectorImpl<vec3<F32> >& pointsA,
				          const vectorImpl<vec3<F32> >& pointsB,
						  const vectorImpl<vec4<U8> >& colors, 
						  const mat4<F32>& globalOffset,
						  const bool orthoMode,
						  const bool disableDepth){
	_api.drawLines(pointsA,pointsB,colors,globalOffset,orthoMode,disableDepth);
}

void GFXDevice::renderGUIElement(GUIElement* const element,ShaderProgram* const guiShader){
	if(!element) return; //< Console not created, for example

	if(_stateBlockDirty) updateStates();

	switch(element->getGuiType()){
        case GUI_TEXT:{
            GUIText* text = dynamic_cast<GUIText*>(element);
            guiShader->Attribute("inColorData",text->_color);
            drawText(text->_text,text->_width,text->getPosition(),text->_font,text->_height);
            }break;
		case GUI_FLASH:
			dynamic_cast<GUIFlash* >(element)->playMovie();
			break;
		default:
			break;
	};
}

void GFXDevice::render(boost::function0<void> renderFunction,SceneRenderState* const sceneRenderState){
	//Call the specific renderfunction that prepares the scene for presentation
	_renderer->render(renderFunction,sceneRenderState);
}

bool GFXDevice::isCurrentRenderStage(U16 renderStageMask){
	assert((renderStageMask & ~(INVALID_STAGE-1)) == 0);
	return bitCompare(renderStageMask,_renderStage);
}

void GFXDevice::setRenderer(Renderer* const renderer) {
	SAFE_UPDATE(_renderer,renderer);
}

bool GFXDevice::getDeferredRendering(){
	assert(_renderer != NULL);
	return (_renderer->getType() != RENDERER_FORWARD);
}

void  GFXDevice::generateCubeMap(FrameBufferObject& cubeMap,
								 Camera* const activeCamera,
								 const vec3<F32>& pos,
								 boost::function0<void> callback){
	//Don't need to override cubemap rendering callback
	if(callback.empty()){
		SceneGraph* sg = GET_ACTIVE_SCENE()->getSceneGraph();
		//Default case is that everything is reflected
		callback = SCENE_GRAPH_UPDATE(sg);
	}
	//Only use cube map FBO's
	if(cubeMap.getType() != FBO_CUBE_COLOR){
		ERROR_FN(Locale::get("ERROR_GFX_DEVICE_INVALID_FBO_CUBEMAP"));
		return;
	}
	//Save our current camera settings
	activeCamera->SaveCamera();
	//And save all camera transform matrices
    lockMatrices(PROJECTION_MATRIX,true,true);
	//set a 90 degree vertical FoV perspective projection
	setPerspectiveProjection(90.0,1,Frustum::getInstance().getZPlanes());
	//Save our old rendering stage
	RenderStage prev = getRenderStage();
	//And set the current render stage to
	setRenderStage(ENVIRONMENT_MAPPING_STAGE);
	//For each of the environment's faces (TOP,DOWN,NORTH,SOUTH,EAST,WEST)
	for(U8 i = 0; i < 6; i++){
		//Set the correct camera orientation and position for current face
		activeCamera->RenderLookAtToCubeMap( pos, i );
		//Bind our FBO's current face
		cubeMap.Begin(i);
			//draw our scene
			render(callback, GET_ACTIVE_SCENE()->renderState());
		//Unbind this face
		cubeMap.End(i);
	}
	//Return to our previous rendering stage
	setRenderStage(prev);
	//Restore transfom matrices
	releaseMatrices();
	//And restore camera
	activeCamera->RestoreCamera();
}

RenderStateBlock* GFXDevice::createStateBlock(const RenderStateBlockDescriptor& descriptor){
   I64 hashValue = descriptor.getGUID();
   if (_stateBlockMap[hashValue])
      return _stateBlockMap[hashValue];

   RenderStateBlock* result = newRenderStateBlock(descriptor);
   _stateBlockMap.insert(std::make_pair(hashValue,result));

   return result;
}

RenderStateBlock* GFXDevice::setStateBlock(RenderStateBlock* block, bool forceUpdate) {
   assert(block != NULL);

   if (block != _currentStateBlock) {
      _deviceStateDirty = true;
      _stateBlockDirty = true;
	  _previousStateBlock = _newStateBlock;
      _newStateBlock = block;
      if(forceUpdate)  updateStates();//<there is no need to force a internal update of stateblocks if nothing changed
   } else {
      _stateBlockDirty = false;
     _newStateBlock = _currentStateBlock;
   }
   return _previousStateBlock;
}

RenderStateBlock* GFXDevice::setStateBlockByDesc( const RenderStateBlockDescriptor &desc ){
   RenderStateBlock *block = createStateBlock( desc );
   _stateBlockByDescription = true;
   return SET_STATE_BLOCK( block );
}

void GFXDevice::setOrthoProjection(const vec4<F32>& rect, const vec2<F32>& planes) {
    _api.setOrthoProjection(rect,planes);
}

void GFXDevice::setPerspectiveProjection(F32 FoV,F32 aspectRatio, const vec2<F32>& planes){
    _api.setPerspectiveProjection(FoV,aspectRatio,planes);
}

void GFXDevice::updateStates(bool force) {
	//Verify render states
	if(force){
		if ( _newStateBlock ){
			updateStateInternal(_newStateBlock, true);
		}
		_currentStateBlock = _newStateBlock;
	}
	if (_stateBlockDirty && !force) {
      updateStateInternal(_newStateBlock);
      _currentStateBlock = _newStateBlock;
    }
    _stateBlockDirty = false;

   LightManager::getInstance().update();
}

bool GFXDevice::excludeFromStateChange(const SceneNodeType& currentType){
	U16 exclusionMask = TYPE_LIGHT | TYPE_TRIGGER | TYPE_PARTICLE_EMITTER | TYPE_SKY;
	return (exclusionMask & currentType) == currentType ? true : false;
}

void GFXDevice::setHorizontalFoV(I32 newFoV){
	F32 ratio  = ParamHandler::getInstance().getParam<F32>("runtime.aspectRatio");
	ParamHandler::getInstance().setParam("runtime.verticalFOV", xfov_to_yfov((F32)newFoV,ratio));
	changeResolution(Application::getInstance().getResolution().width,Application::getInstance().getResolution().height);
}

void GFXDevice::enableFog(FogMode mode, F32 density, const vec3<F32>& color, F32 startDist, F32 endDist){
	ParamHandler& par = ParamHandler::getInstance();
	par.setParam("rendering.sceneState.fogColor.r", color.r);
	par.setParam("rendering.sceneState.fogColor.g", color.g);
	par.setParam("rendering.sceneState.fogColor.b", color.b);
	par.setParam("rendering.sceneState.fogDensity",density);
	par.setParam("rendering.sceneState.fogStart",startDist);
	par.setParam("rendering.sceneState.fogEnd",endDist);
	par.setParam("rendering.sceneState.fogMode",mode);
	ShaderManager::getInstance().refresh();
}