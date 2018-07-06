#include "Headers/GFXDevice.h"
#include "Headers/RenderStateBlock.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIText.h"
#include "GUI/Headers/GUIFlash.h"
#include "Core/Math/Headers/Plane.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Rendering/Headers/Frustum.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/ShaderManager.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/RenderPass/Headers/RenderPass.h"

#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"

GFXDevice::GFXDevice() : _api(GL_API::getOrCreateInstance()) ///<Defaulting to OpenGL if no api has been defined
{
   _prevShaderId = 0;
   _prevTextureId = 0;
   _currentStateBlock = NULL;
   _newStateBlock = NULL;
   _previousStateBlock = NULL;
   _stateBlockDirty = false;
   _drawDebugAxis = false;
   _clippingPlanesDirty = true;
   _renderer = NULL;
   _renderStage = INVALID_STAGE;
   _worldMatrices.push(mat4<F32>(/*identity*/));
   _isUniformedScaled = true;
   _WDirty = _VDirty = _PDirty = true;
   _WVCachedMatrix.identity();
   _WVPCachedMatrix.identity();

   Frustum::createInstance();
   RenderPass* diffusePass = New RenderPass("diffusePass");
   RenderPassManager::getOrCreateInstance().addRenderPass(diffusePass,1);
   //RenderPassManager::getInstance().addRenderPass(shadowPass,2);
}

void GFXDevice::setApi(const RenderAPI& api){
    switch(api)	{
        default:
        case OpenGL:    _api = GL_API::getOrCreateInstance();	break;
        case Direct3D:	_api = DX_API::getOrCreateInstance();	break;

        case GFX_RENDER_API_PLACEHOLDER: ///< Placeholder - OpenGL 4.0 and DX 11 in another life maybe :) - Ionut
        case Software:
        case None:		{ ERROR_FN(Locale::get("ERROR_GFX_DEVICE_API")); setApi(OpenGL); return; }
    };

    _api.setId(api);
}

void GFXDevice::closeRenderingApi(){
    _api.closeRenderingApi();
    for_each(RenderStateMap::value_type& it, _stateBlockMap){
        SAFE_DELETE(it.second);
    }
    _stateBlockMap.clear();
    Frustum::destroyInstance();
    ///Destroy all rendering Passes
    RenderPassManager::getInstance().destroyInstance();
}

void GFXDevice::closeRenderer(){
    PRINT_FN(Locale::get("CLOSING_RENDERER"));
    SAFE_DELETE(_renderer);
}

void GFXDevice::renderInstance(RenderInstance* const instance){
    //All geometry is stored in VBO format
    assert(instance->object3D() != NULL);
	Object3D* model = instance->object3D();
 
    if(instance->preDraw())
        model->onDraw(getRenderStage());

    if(instance->draw2D()){
        //toggle2D(true);
    }

    if(_stateBlockDirty)
        updateStates();

	if(model->getType() == Object3D::OBJECT_3D_PLACEHOLDER){
        ERROR_FN(Locale::get("ERROR_GFX_INVALID_OBJECT_TYPE"),model->getName().c_str());
        return;
    }
	 
	if(model->getType() == Object3D::TEXT_3D){
        Text3D* text = dynamic_cast<Text3D*>(model);
        drawText(text->getText(),text->getWidth(),text->getFont(),text->getHeight());
        return;
    }

	Transform* transform = instance->transform();
	VertexBufferObject* VBO = model->getGeometryVBO();
	assert(VBO != NULL);

	if(transform) 
		pushWorldMatrix(transform->getGlobalMatrix(), transform->isUniformScaled());

    //Send our transformation matrixes (projection, world, view, inv model view, etc)
    VBO->currentShader()->uploadNodeMatrices();
    //Render our current vertex array object
    VBO->Draw(model->getCurrentLOD());

	if(transform) 
		popWorldMatrix();
}

void GFXDevice::renderBuffer(VertexBufferObject* const vbo, Transform* const vboTransform){
	assert(vbo != NULL);

    if(_stateBlockDirty)
        updateStates();
	 
	if(vboTransform){
         pushWorldMatrix(vboTransform->getGlobalMatrix(), vboTransform->isUniformScaled());
    }
         
	vbo->currentShader()->uploadNodeMatrices();
    //Render our current vertex array object
    vbo->DrawRange();

	if(vboTransform){
        popWorldMatrix();
    }
}

void GFXDevice::renderGUIElement(GUIElement* const element,ShaderProgram* const guiShader){
    if(!element)
        return; //< Console not created, for example

    if(_stateBlockDirty)
        updateStates();

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

void GFXDevice::render(boost::function0<void> renderFunction, const SceneRenderState& sceneRenderState){
    //Call the specific renderfunction that prepares the scene for presentation
    _renderer->render(renderFunction,sceneRenderState);
}

bool GFXDevice::isCurrentRenderStage(U16 renderStageMask){
    assert((renderStageMask & ~(INVALID_STAGE-1)) == 0);
    return bitCompare(renderStageMask,_renderStage);
}

void GFXDevice::setRenderer(Renderer* const renderer) {
    assert(renderer != NULL);
    SAFE_UPDATE(_renderer,renderer);
}

void  GFXDevice::generateCubeMap(FrameBufferObject& cubeMap,
                                 const vec3<F32>& pos,
                                 boost::function0<void> callback,
                                 const RenderStage& renderStage){
    //Don't need to override cubemap rendering callback
    if(callback.empty()){
        //Default case is that everything is reflected
        callback = SCENE_GRAPH_UPDATE(GET_ACTIVE_SCENEGRAPH());
    }
    //Only use cube map FBO's
    if(cubeMap.getType() != FBO_CUBE_COLOR && cubeMap.getType() != FBO_CUBE_DEPTH){
        if(cubeMap.getType() != FBO_CUBE_COLOR) {
            ERROR_FN(Locale::get("ERROR_GFX_DEVICE_INVALID_FBO_CUBEMAP"));
        }else{
            ERROR_FN(Locale::get("ERROR_GFX_DEVICE_INVALID_FBO_CUBEMAP_SHADOW"));
        }
        return;
    }

    static vec3<F32> TabUp[6] = {
        vec3<F32>(0.0f,	-1.0f,	0.0f),
        vec3<F32>(0.0f,	-1.0f,	0.0f),
        vec3<F32>(0.0f,	 0.0f,	1.0f),
        vec3<F32>(0.0f,  0.0f, -1.0f),
        vec3<F32>(0.0f,	-1.0f,	0.0f),
        vec3<F32>(0.0f,	-1.0f,	0.0f)
    };

    ///Get the center and up vectors for each cube face
    vec3<F32> TabCenter[6] = {
        vec3<F32>(pos.x+1.0f,	pos.y,		pos.z),
        vec3<F32>(pos.x-1.0f,	pos.y,		pos.z),
        vec3<F32>(pos.x,		pos.y+1.0f,	pos.z),
        vec3<F32>(pos.x,		pos.y-1.0f,	pos.z),
        vec3<F32>(pos.x,		pos.y,		pos.z+1.0f),
        vec3<F32>(pos.x,		pos.y,		pos.z-1.0f)
    };

    //And save all camera transform matrices
    lockMatrices(PROJECTION_MATRIX,true,true);
    //set a 90 degree vertical FoV perspective projection
    setPerspectiveProjection(90.0,1,Frustum::getInstance().getZPlanes());
    //And set the current render stage to
    setRenderStage(renderStage);
    //For each of the environment's faces (TOP,DOWN,NORTH,SOUTH,EAST,WEST)
    for(U8 i = 0; i < 6; i++){
        ///Set our Rendering API to render the desired face
        GFX_DEVICE.lookAt(pos,TabCenter[i],TabUp[i]);
        GET_ACTIVE_SCENE()->renderState().getCamera().updateListeners();
        ///Extract the view frustum associated with this face
        Frustum::getInstance().Extract(pos);
        //Bind our FBO's current face
        cubeMap.Begin(i);
            //draw our scene
            render(callback, GET_ACTIVE_SCENE()->renderState());
        //Unbind this face
        cubeMap.End(i);
    }
    //Return to our previous rendering stage
    setPreviousRenderStage();
    //Restore transfom matrices
    releaseMatrices();
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

void GFXDevice::updateStates(bool force) {
    //Verify render states
    if(force){
        if ( _newStateBlock )
            updateStateInternal(_newStateBlock, true);

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
    ParamHandler::getInstance().setParam("runtime.verticalFOV", Util::xfov_to_yfov((F32)newFoV,ratio));
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

 void GFXDevice::popWorldMatrix(bool force){
	 _worldMatrices.pop();
	 _isUniformedScaled = _WDirty = true;
}

void GFXDevice::pushWorldMatrix(const mat4<F32>& worldMatrix, bool isUniformedScaled){
    _worldMatrices.push(worldMatrix);
	_isUniformedScaled = isUniformedScaled;
    _WDirty = true;
}
       
void GFXDevice::getMatrix(const EXTENDED_MATRIX& mode, mat3<GLfloat>& mat){
     assert(mode == NORMAL_MATRIX /*|| mode == ... */);
     cleanMatrices();
            
	 // Normal Matrix
     if(!_isUniformedScaled){
		 mat4<F32> temp;
		 _WVCachedMatrix.inverse(temp);
		 mat.set(temp.transpose());
	 }else{
		 mat.set(_WVCachedMatrix);
	 }
}

void GFXDevice::getMatrix(const EXTENDED_MATRIX& mode, mat4<F32>& mat) {
     assert(mode != NORMAL_MATRIX /*|| mode != ... */);

	 //refresh cache
	 if(mode != WORLD_MATRIX && mode != BIAS_MATRIX)
		 cleanMatrices();

      switch(mode){
          case WORLD_MATRIX:  mat.set(_worldMatrices.top()); break;
		  case WV_MATRIX:     mat.set(_WVCachedMatrix);      break;
		  case WVP_MATRIX:    mat.set(_WVPCachedMatrix);     break;
          case BIAS_MATRIX:   mat.bias();                    break;
	      case WV_INV_MATRIX: _WVCachedMatrix.inverse(mat);  break;
      }
}

void GFXDevice::cleanMatrices(){
	 if(_WDirty){
		if(_VDirty) 	       _api.getMatrix(VIEW_MATRIX, _viewCacheMatrix);
		if(_VDirty || _PDirty) _api.getMatrix(VIEW_PROJECTION_MATRIX, _VPCachedMatrix);
	 
		 _WVCachedMatrix.set(_viewCacheMatrix * _worldMatrices.top());
		 _WVPCachedMatrix.set(_VPCachedMatrix * _worldMatrices.top());
		 _VDirty = _PDirty = _WDirty = false;
	 }
}