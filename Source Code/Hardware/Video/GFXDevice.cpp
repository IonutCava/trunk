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

#ifdef FORCE_NV_OPTIMUS_HIGHPERFORMANCE
extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

GFXDevice::GFXDevice() : _api(GL_API::getOrCreateInstance()) //<Defaulting to OpenGL if no api has been defined
{
   _prevShaderId = 0;
   _prevTextureId = 0;
   _interpolationFactor = 1.0;
   _currentStateBlock = nullptr;
   _newStateBlock = nullptr;
   _previousStateBlock = nullptr;
   _previewDepthMapShader = nullptr;
   _renderQuad = nullptr;
   _stateBlockDirty = false;
   _drawDebugAxis = false;
   _enablePostProcessing = false;
   _enableAnaglyph = false;
   _enableHDR = false;
   _clippingPlanesDirty = true;
   _isDepthPrePass = false;
   _previewDepthBuffer = false;
   _renderer = nullptr;
   _renderStage = INVALID_STAGE;
   _worldMatrices.push(mat4<F32>(/*identity*/));
   _isUniformedScaled = true;
   _WDirty = _VDirty = _PDirty = true;
   _WVCachedMatrix.identity();
   _WVPCachedMatrix.identity();

   for (FrameBuffer*& renderTarget : _renderTarget)
       renderTarget = nullptr;

   Frustum::createInstance();
   RenderPass* diffusePass = New RenderPass("diffusePass");
   RenderPassManager::getOrCreateInstance().addRenderPass(diffusePass,1);
   //RenderPassManager::getInstance().addRenderPass(shadowPass,2);
}

GFXDevice::~GFXDevice()
{
}

I8 GFXDevice::initHardware(const vec2<U16>& resolution, I32 argc, char **argv) {
    
    PostFX::createInstance();
    ShaderManager::createInstance();

    I8 hardwareState = _api.initHardware(resolution,argc,argv);

    if(hardwareState == NO_ERR){
        //Screen FB should use MSAA if available, else fallback to normal color FB (no AA or FXAA)
        _renderTarget[RENDER_TARGET_SCREEN] = newFB(FB_2D_COLOR_MS);
        _renderTarget[RENDER_TARGET_DEPTH]  = newFB(FB_2D_DEPTH);

        SamplerDescriptor screenSampler;
        TextureDescriptor screenDescriptor(TEXTURE_2D,
                                           RGBA,
                                           _enableHDR ? RGBA16F : RGBA8,
                                           _enableHDR ? FLOAT_16 : UNSIGNED_BYTE);

        screenSampler.setFilters(TEXTURE_FILTER_NEAREST, TEXTURE_FILTER_NEAREST);
        screenSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
        screenSampler.toggleMipMaps(false);
        screenDescriptor.setSampler(screenSampler);

        SamplerDescriptor depthSampler;
        depthSampler.setFilters(TEXTURE_FILTER_NEAREST, TEXTURE_FILTER_NEAREST);
        depthSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
        depthSampler.toggleMipMaps(false);
        depthSampler._useRefCompare = true; //< Use compare function
        depthSampler._cmpFunc = CMP_FUNC_LEQUAL; //< Use less or equal

        TextureDescriptor depthDescriptor(TEXTURE_2D, DEPTH_COMPONENT, DEPTH_COMPONENT, UNSIGNED_INT);
        depthDescriptor.setSampler(depthSampler);

        _renderTarget[RENDER_TARGET_DEPTH]->AddAttachment(depthDescriptor, TextureDescriptor::Depth);
        _renderTarget[RENDER_TARGET_DEPTH]->toggleColorWrites(false);

        _renderTarget[RENDER_TARGET_SCREEN]->AddAttachment(screenDescriptor, TextureDescriptor::Color0);
        _renderTarget[RENDER_TARGET_SCREEN]->toggleDepthBuffer(true);
        if(_enableAnaglyph){
            _renderTarget[RENDER_TARGET_ANAGLYPH] = newFB(FB_2D_COLOR_MS);
            _renderTarget[RENDER_TARGET_ANAGLYPH]->AddAttachment(screenDescriptor, TextureDescriptor::Color0);
            _renderTarget[RENDER_TARGET_ANAGLYPH]->toggleDepthBuffer(true);
        }

        for (FrameBuffer* renderTarget : _renderTarget){
            if (renderTarget)
                renderTarget->Create(resolution.width, resolution.height);
        }
    }

    return hardwareState;
}

void GFXDevice::setApi(const RenderAPI& api){
    switch(api)	{
        default:
        case OpenGL:    _api = GL_API::getOrCreateInstance();	break;
        case Direct3D:	_api = DX_API::getOrCreateInstance();	break;

        case GFX_RENDER_API_PLACEHOLDER: ///< Placeholder - OpenGL 4.0 and DX 11 in another life maybe :) - Ionut
        case None:		{ ERROR_FN(Locale::get("ERROR_GFX_DEVICE_API")); setApi(OpenGL); return; }
    };

    _api.setId(api);
}

void GFXDevice::closeRenderingApi(){
    ShaderManager::getInstance().destroyInstance();
    _api.closeRenderingApi();
    FOR_EACH(RenderStateMap::value_type& it, _stateBlockMap){
        SAFE_DELETE(it.second);
    }
    _stateBlockMap.clear();
    Frustum::destroyInstance();
    //Destroy all rendering Passes
    RenderPassManager::getInstance().destroyInstance();

    for (FrameBuffer* renderTarget : _renderTarget)
        SAFE_DELETE(renderTarget);
}

void GFXDevice::closeRenderer(){
    PRINT_FN(Locale::get("STOP_POST_FX"));
    PostFX::getInstance().destroyInstance();
    ShaderManager::getInstance().Destroy();
    PRINT_FN(Locale::get("CLOSING_RENDERER"));
    SAFE_DELETE(_renderer);
}

void GFXDevice::idle() {
    PostFX::getInstance().idle();
    ShaderManager::getInstance().idle();
    _api.idle();
}

void GFXDevice::renderInstance(RenderInstance* const instance){
    //All geometry is stored in VB format
    assert(instance->object3D() != nullptr);
    Object3D* model = instance->object3D();
 
    if (instance->preDraw()){
        if(!model->onDraw(getRenderStage()))
            return;
    }

    if(instance->draw2D()){
        //toggle2D(true);
    }

    if(_stateBlockDirty)
        updateStates();

    if(model->getType() == Object3D::OBJECT_3D_PLACEHOLDER){
        ERROR_FN(Locale::get("ERROR_GFX_INVALID_OBJECT_TYPE"),model->getName().c_str());
        return;
    }
     
    if(model->getType() == Object3D::TEXT_3D) {
        //Text3D* text = dynamic_cast<Text3D*>(model);
        //drawText(text->getText(),text->getWidth(),text->getFont(),text->getHeight(),false,false);
        //3D text disabled for now - to add lader - Ionut
        return;
    }

    Transform* transform = instance->transform();
    Transform* prevTransform = instance->prevTransform();
    if(transform) {
        if(_interpolationFactor < 0.99 && prevTransform) {
            pushWorldMatrix(transform->interpolate(instance->prevTransform(), _interpolationFactor), transform->isUniformScaled());
        }else{
            pushWorldMatrix(transform->getGlobalMatrix(), transform->isUniformScaled());
        }
    }

    VertexBuffer* VB = model->getGeometryVB();
    assert(VB != nullptr);
    //Send our transformation matrices (projection, world, view, inv model view, etc)
    VB->currentShader()->uploadNodeMatrices();
    //Render our current vertex array object
    VB->Draw(model->getCurrentLOD());

    if(transform) popWorldMatrix();
}

void GFXDevice::renderBuffer(VertexBuffer* const vb, Transform* const vbTransform){
    assert(vb != nullptr);

    if(_stateBlockDirty)
        updateStates();
     
    if(vbTransform){
        pushWorldMatrix(vbTransform->getGlobalMatrix(), vbTransform->isUniformScaled());
    }
         
    vb->currentShader()->uploadNodeMatrices();
    //Render our current vertex array
    vb->DrawRange();

    if(vbTransform){
        popWorldMatrix();
    }
}

void GFXDevice::renderGUIElement(U64 renderInterval, GUIElement* const element,ShaderProgram* const guiShader){
    if(!element)
        return; //< Console not created, for example

    if(_stateBlockDirty)
        updateStates();

    switch(element->getGuiType()){
        case GUI_TEXT:{
            GUIText* text = dynamic_cast<GUIText*>(element);
            drawText(*text, text->getPosition());
            text->_lastDrawTimer = _kernel->getCurrentTime();
            }break;
        case GUI_FLASH:
            dynamic_cast<GUIFlash* >(element)->playMovie();
            break;
        default:
            break;
    };
}

void GFXDevice::render(const DELEGATE_CBK& renderFunction, const SceneRenderState& sceneRenderState){
    //Call the specific render function that prepares the scene for presentation
    _renderer->render(renderFunction, sceneRenderState);
}

bool GFXDevice::isCurrentRenderStage(U8 renderStageMask){
    assert((renderStageMask & ~(INVALID_STAGE-1)) == 0);
    return bitCompare(renderStageMask, _renderStage);
}

void GFXDevice::setRenderer(Renderer* const renderer) {
    assert(renderer != nullptr);
    SAFE_UPDATE(_renderer,renderer);
}

void  GFXDevice::generateCubeMap(FrameBuffer& cubeMap,
                                 const vec3<F32>& pos,
                                 const DELEGATE_CBK& callback,
                                 const RenderStage& renderStage){
    //Only use cube map FB's
    if(cubeMap.getType() != FB_CUBE_COLOR && cubeMap.getType() != FB_CUBE_DEPTH){
        if(cubeMap.getType() != FB_CUBE_COLOR) {
            ERROR_FN(Locale::get("ERROR_GFX_DEVICE_INVALID_FB_CUBEMAP"));
        }else{
            ERROR_FN(Locale::get("ERROR_GFX_DEVICE_INVALID_FB_CUBEMAP_SHADOW"));
        }
        return;
    }

    static vec3<F32> TabUp[6] = {
        WORLD_Y_NEG_AXIS,
        WORLD_Y_NEG_AXIS,
        WORLD_Z_AXIS,
        WORLD_Z_NEG_AXIS,
        WORLD_Y_NEG_AXIS,
        WORLD_Y_NEG_AXIS
    };

    //Get the center and up vectors for each cube face
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
    setPerspectiveProjection(90.0f,1.0f,Frustum::getInstance().getZPlanes());
    //And set the current render stage to
    setRenderStage(renderStage);
    //Bind our FB
    cubeMap.Begin(FrameBuffer::defaultPolicy());

    assert(!callback.empty());

    //For each of the environment's faces (TOP,DOWN,NORTH,SOUTH,EAST,WEST)
    for(U8 i = 0; i < 6; i++){
        // true to the current cubemap face
        cubeMap.DrawToFace(TextureDescriptor::Color0, i);
        ///Set our Rendering API to render the desired face
        GFX_DEVICE.lookAt(pos,TabCenter[i],TabUp[i]);
        GET_ACTIVE_SCENE()->renderState().getCamera().updateListeners();
        ///Extract the view frustum associated with this face
        Frustum::getInstance().Extract(pos);
            //draw our scene
            render(callback, GET_ACTIVE_SCENE()->renderState());
    }
    //Unbind this fb
    cubeMap.End();
    //Return to our previous rendering stage
    setPreviousRenderStage();
    //Restore transform matrices
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
   assert(block != nullptr);

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
    U16 exclusionMask = TYPE_LIGHT | TYPE_TRIGGER | TYPE_PARTICLE_EMITTER | TYPE_SKY | TYPE_VEGETATION_GRASS | TYPE_VEGETATION_TREES;
    return (exclusionMask & currentType) == currentType ? true : false;
}

void GFXDevice::changeResolution(U16 w, U16 h) {
    if(_renderTarget[RENDER_TARGET_SCREEN] != nullptr) {
        if (vec2<U16>(w,h) == _renderTarget[RENDER_TARGET_SCREEN]->getResolution() || !(w > 1 && h > 1)) 
            return;
        
        for (FrameBuffer* renderTarget : _renderTarget){
            if (renderTarget)
                renderTarget->Create(w, h);
        }
    }

    _api.changeResolution(w,h);
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

 void GFXDevice::popWorldMatrix(){
     _worldMatrices.pop();
     _isUniformedScaled = _WDirty = true;

     ShaderManager::getInstance().setMatricesDirty();
}

void GFXDevice::pushWorldMatrix(const mat4<F32>& worldMatrix, const bool isUniformedScaled){
    _worldMatrices.push(worldMatrix);
    _isUniformedScaled = isUniformedScaled;
    _WDirty = true;

    ShaderManager::getInstance().setMatricesDirty();
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
     if(mode != WORLD_MATRIX)
          cleanMatrices();
      
      switch(mode){
          case WORLD_MATRIX:  mat.set(_worldMatrices.top()); break;
          case WV_MATRIX:     mat.set(_WVCachedMatrix);      break;
          case WVP_MATRIX:    mat.set(_WVPCachedMatrix);     break;
          case WV_INV_MATRIX: _WVCachedMatrix.inverse(mat);  break;
      }
}

void GFXDevice::cleanMatrices(){
    assert(!_worldMatrices.empty());
    if(_WDirty){
        if (_VDirty) 	       {
            _api.getMatrix(VIEW_MATRIX, _viewCacheMatrix);
        }
        if (_VDirty || _PDirty) {
            _api.getMatrix(VIEW_PROJECTION_MATRIX, _VPCachedMatrix);
        }
        // we transpose the matrices when we use them in the shader
        _WVCachedMatrix.set(_worldMatrices.top() * _viewCacheMatrix);
        _WVPCachedMatrix.set(_worldMatrices.top() * _VPCachedMatrix);

        _VDirty = _PDirty = _WDirty = false;
    }
}

void GFXDevice::previewDepthBuffer(){
    _previewDepthBuffer = true;
}

void GFXDevice::previewDepthBufferInternal(){
    if(!_previewDepthBuffer)
        return;

    if(!_previewDepthMapShader){
        ResourceDescriptor shadowPreviewShader("fbPreview.LinearDepth");
        _previewDepthMapShader = CreateResource<ShaderProgram>(shadowPreviewShader);
        assert(_previewDepthMapShader != nullptr);
        ResourceDescriptor mrt("DepthBufferPreviewQuad");
        mrt.setFlag(true); //No default Material;
        _renderQuad = CreateResource<Quad3D>(mrt);
        _renderQuad->renderInstance()->draw2D(true);
        _renderQuad->renderInstance()->preDraw(true);
        _renderQuad->setCustomShader(_previewDepthMapShader);
        _renderQuad->setDimensions(vec4<F32>(0,0,Application::getInstance().getResolution().width,Application::getInstance().getResolution().height));
    }

    if(_previewDepthMapShader->getState() != RES_LOADED)
        return;

    const I32 viewportSide = 256;
    _renderTarget[RENDER_TARGET_DEPTH]->Bind(0, TextureDescriptor::Depth);
    _previewDepthMapShader->bind();
    _previewDepthMapShader->UniformTexture("tex",0);
    toggle2D(true);
    renderInViewport(vec4<I32>(Application::getInstance().getResolution().width-viewportSide,0,viewportSide,viewportSide),
                                boost::bind(&GFXDevice::renderInstance,
                                            DELEGATE_REF(GFX_DEVICE),
                                            _renderQuad->renderInstance()));
    toggle2D(false);
    _renderTarget[RENDER_TARGET_DEPTH]->Unbind(0);

    _previewDepthBuffer = false;
}

void GFXDevice::flush(){
    previewDepthBufferInternal();
    _api.flush();
}