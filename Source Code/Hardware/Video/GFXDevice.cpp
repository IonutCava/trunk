#include "Headers/GFXDevice.h"
#include "Headers/RenderStateBlock.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIConsole.h"
#include "Core/Headers/Application.h"

#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/ShaderManager.h"

#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"

#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Importer/Headers/DVDConverter.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"


void GFXDevice::setApi(RENDER_API api){

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
}

void GFXDevice::renderModel(Object3D* const model){
	///All geometry is stored in VBO format
	if(!model) return;
	if(model->getGeometryVBO()->getHWIndices().empty()){
		model->onDraw(); ///<something wrong! Re-run pre-draw tests!
	}
	if(_stateBlockDirty){
		updateStates();
	}
	_api.renderModel(model);

}

void GFXDevice::renderElements(PRIMITIVE_TYPE t, VERTEX_DATA_FORMAT f, U32 count, const void* first_element){
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

void GFXDevice::drawLines(const std::vector<vec3<F32> >& pointsA,const std::vector<vec3<F32> >& pointsB,const std::vector<vec4<F32> >& colors, const mat4<F32>& globalOffset){
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
		case GUI_CONSOLE:///Console is singleton so no parameter needed
			drawConsole();
		default:
			break;
	};

}

void GFXDevice::setRenderStage(RENDER_STAGE stage){
	_renderStage = stage;
}

bool _drawBBoxes = false;
bool _drawSkeleton = false;
void GFXDevice::processRenderQueue(){

	///Sort the render queue by the specified key
	///This only affects the final rendering stage
	RenderQueue::getInstance().sort();

	SceneNode* sn = NULL;
	SceneGraphNode* sgn = NULL;
	Transform* t = NULL;
	///Shadows are applied only in the final stage
	if(getRenderStage() == FINAL_STAGE  || getRenderStage() == DEFERRED_STAGE){ 
		///Tell the lightManager to bind all available depth maps
		LightManager::getInstance().bindDepthMaps();
	}

	_renderBinCount = RenderQueue::getInstance().getRenderQueueStackSize();
	///Draw the entire queue;
	///Limited to 65536 (2^16) items per queue pass!
	if(GET_ACTIVE_SCENE()->drawObjects()){
		for(U16 i = 0; i < _renderBinCount; i++){
			//Get the current scene node
			sgn = RenderQueue::getInstance().getItem(i)._node;
			///And validate it
			assert(sgn);
			///Get it's transform
			t = sgn->getTransform();
			///And it's attached SceneNode
			sn = sgn->getNode<SceneNode>();
			///Validate the SceneNode
			assert(sn);
			///Call any pre-draw operations on the SceneNode (refresh VBO, update materials, etc)
			sn->onDraw();
			///Try to see if we need a shader transform or a fixed pipeline one
			ShaderProgram* s = NULL;
			Material* m = sn->getMaterial();
			if(m){
				s = m->getShaderProgram();
			}
			///Check if we should draw the node. (only after onDraw as it may contain exclusion mask changes before draw)
			if(sn->getDrawState(getRenderStage())){
				///Transform the Object (Rot, Trans, Scale)
				if(!excludeFromStateChange(sn->getType())){ ///< only if the node is not in the exclusion mask
					setObjectState(t,false,s);
				}
				///setup materials and render the node
				///As nodes are sorted, this should be very fast
				///We need to apply different materials for each stage
				switch(getRenderStage()){
					case FINAL_STAGE:
					case DEFERRED_STAGE:
					case REFLECTION_STAGE:
					case ENVIRONMENT_MAPPING_STAGE:
						sn->prepareMaterial(sgn);
							break;
					case SHADOW_STAGE:
						sn->prepareShadowMaterial(sgn);
							break;
				}
				///Call taage exclusion mask should do the rest
				sn->render(sgn); 
				///Unbind current material properties
				///ToDo: Optimize this!!!! -Ionut
				switch(getRenderStage()){
					case FINAL_STAGE:
					case DEFERRED_STAGE:
					case REFLECTION_STAGE:
					case ENVIRONMENT_MAPPING_STAGE:
						sn->releaseMaterial();
						break;
					case SHADOW_STAGE:
						sn->releaseShadowMaterial();
						break;
				}
				///Drop all applied transformations so that they do not affect the next node
				if(!excludeFromStateChange(sn->getType())){
					releaseObjectState(t,s);
				}
			}
			/// Perform any post draw operations regardless of the draw state
			sn->postDraw();
		}
	}
	///Unbind all shaders after every render pass
	ShaderManager::getInstance().unbind();

	if(getRenderStage() == FINAL_STAGE || getRenderStage() == DEFERRED_STAGE){
		///Unbind all depth maps 
		LightManager::getInstance().unbindDepthMaps();

        ///Dump render states
		///Draw all BBoxes
		_drawBBoxes = GET_ACTIVE_SCENE()->drawBBox();
		_drawSkeleton = GET_ACTIVE_SCENE()->drawSkeletons();
		for(U16 i = 0; i < _renderBinCount; i++){
			///Get the current scene node
			sgn = RenderQueue::getInstance().getItem(i)._node;
			///draw bounding box if needed and only in the final stage to prevent Shadow/PostFX artifcats
			BoundingBox& bb = sgn->getBoundingBox();
			///Draw the bounding box if it's always on or if the scene demands it
			///Terrain handles bounding boxes diferrently
			if(bb.getVisibility() || _drawBBoxes){
				sgn->getNode<SceneNode>()->drawBoundingBox(sgn);
			}
			if(_drawSkeleton){
				if(sgn->getNode<SceneNode>()->getType() == TYPE_OBJECT3D){
					Object3D* obj = sgn->getNode<Object3D>();
					assert(obj != NULL);
					/// For SubMesh objects
					if(obj->getType() == SUBMESH){
						dynamic_cast<SubMesh*>(obj)->renderSkeleton(sgn);
					}
				}
			}
		}
	}
	RenderQueue::getInstance().refresh();
}

void GFXDevice::renderInViewport(const vec4<F32>& rect, boost::function0<void> callback){
	_api.renderInViewport(rect,callback);
}

void  GFXDevice::generateCubeMap(FrameBufferObject& cubeMap, 
								 Camera* const activeCamera,
								 const vec3<F32>& pos, 
								 boost::function0<void> callback){
	///Don't need to override cubemap rendering callback
	if(callback.empty()){
		SceneGraph* sg = GET_ACTIVE_SCENE()->getSceneGraph();
		///Default case is that everything is reflected
		callback = boost::bind(boost::bind(&SceneGraph::render, sg));
	}
	///Only use cube map FBO's
	if(cubeMap.getType() != FBO_CUBE_COLOR){
		ERROR_FN(Locale::get("ERROR_GFX_DEVICE_INVALID_FBO_CUBEMAP"));
		return;
	}
	///Get some global vars
	ParamHandler& par = ParamHandler::getInstance();
	F32 zNear  = par.getParam<F32>("zNear");
	F32 zFar   = par.getParam<F32>("zFar");
	///Save our current camera settings
	activeCamera->SaveCamera();
	///And save all camera transform matrices
	lockModelView();
	lockProjection();
	///set a 90 degree vertical FoV perspective projection
	setPerspectiveProjection(90.0,1,vec2<F32>(zNear,zFar));
	///Save our old rendering stage
	RENDER_STAGE prev = getRenderStage();
	///And set the current render stage to 
	setRenderStage(ENVIRONMENT_MAPPING_STAGE);
	///For each of the environment's faces (TOP,DOWN,NORTH,SOUTH,EAST,WEST)
	for(U8 i = 0; i < 6; i++){
		///Set the correct camera orientation and position for current face
		activeCamera->RenderLookAtToCubeMap( pos, i );
		///Bind our FBO's current face
		cubeMap.Begin(i);
			///draw our scene
			callback();
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
   ///Verify lights
   LightManager::getInstance().update();
}

bool GFXDevice::excludeFromStateChange(SCENE_NODE_TYPE currentType){
	U8 exclusionMask = TYPE_LIGHT | TYPE_TRIGGER | TYPE_PARTICLE_EMITTER;
	return (exclusionMask & currentType) == currentType ? true : false;
}