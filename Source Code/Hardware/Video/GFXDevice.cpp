#include "Headers/GFXDevice.h"
#include "Headers/RenderStateBlock.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIText.h"
#include "GUI/Headers/GUIFlash.h"
#include "Core/Math/Headers/Plane.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/ShaderManager.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Rendering/RenderPass/Headers/RenderPass.h"

#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"

#include "Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

#ifdef FORCE_NV_OPTIMUS_HIGHPERFORMANCE
extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

ShaderProgram* GFXDevice::_activeShaderProgram = nullptr;
ShaderProgram* GFXDevice::_HIZConstructProgram = nullptr;

GFXDevice::GFXDevice() : _api(GL_API::getOrCreateInstance()),
                         _postFX(PostFX::getOrCreateInstance()),
                         _shaderManager(ShaderManager::getOrCreateInstance()) //<Defaulting to OpenGL if no api has been defined
{
   _stateExclusionMask = 0;
   _prevShaderId = 0;
   _prevTextureId = 0;
   _interpolationFactor = 1.0;
   _MSAASamples = 0;
   _FXAASamples = 0;
   _2DRendering = false;
   _loaderThread = nullptr;
   _matricesBuffer = nullptr;
   _state2DRendering = nullptr;
   _stateDepthOnlyRendering = nullptr;
   _defaultStateBlock = nullptr;
   _defaultStateNoDepth = nullptr;
   _currentStateBlock = nullptr;
   _previewDepthMapShader = nullptr;
   _stateBlockDirty = false;
   _drawDebugAxis = false;
   _enablePostProcessing = false;
   _enableAnaglyph = false;
   _enableHDR = false;
   _clippingPlanesDirty = true;
   _isDepthPrePass = false;
   _previewDepthBuffer = false;
   _renderer = nullptr;
   _rasterizationEnabled = true;
   _viewportUpdate = false;
   _viewportForced = false;
   _generalDetailLevel = DETAIL_HIGH;
   _shadowDetailLevel = DETAIL_HIGH;
   _renderStage = INVALID_STAGE;
   _worldMatrices.push(mat4<F32>(/*identity*/));
   _isUniformedScaled = true;
   _WDirty = _VDirty = _PDirty = true;
   _WVCachedMatrix.identity();
   _WVPCachedMatrix.identity();
   _viewport.push(vec4<I32>(0, 0, 0, 0));
   _cubeCamera = New FreeFlyCamera();
   _2DCamera = New FreeFlyCamera();
   _2DCamera->lockView(true);
   _2DCamera->lockFrustum(true);

   for (FrameBuffer*& renderTarget : _renderTarget)
       renderTarget = nullptr;

   RenderPass* diffusePass = New RenderPass("diffusePass");
   RenderPassManager::getOrCreateInstance().addRenderPass(diffusePass,1);
   //RenderPassManager::getInstance().addRenderPass(shadowPass,2);
}

GFXDevice::~GFXDevice()
{
    _2dRenderQueue.clear();
}

void GFXDevice::setApi(const RenderAPI& api){
    _api.setId(api);
    switch (api)	{
        default:
        case OpenGLES:
        case OpenGL:    _api = GL_API::getOrCreateInstance();	break;
        case Direct3D:	_api = DX_API::getOrCreateInstance();	break;

        case GFX_RENDER_API_PLACEHOLDER:
        case None:		{ ERROR_FN(Locale::get("ERROR_GFX_DEVICE_API")); setApi(OpenGL); return; }
    };
}

I8 GFXDevice::initHardware(const vec2<U16>& resolution, I32 argc, char **argv) {
    I8 hardwareState = _api.initHardware(resolution,argc,argv);

    if(hardwareState == NO_ERR){
        _matricesBuffer = newSB();
        _matricesBuffer->Create(true, false);
        _matricesBuffer->ReserveBuffer(3 * 16 + 4, sizeof(F32)); //View, Projection, ViewProjection and Viewport 3 x 16 + 4 float values
        _matricesBuffer->bind(Divide::SHADER_BUFFER_CAM_MATRICES);
        changeResolution(resolution);

        RenderStateBlockDescriptor defaultStateDescriptor;
        _defaultStateBlock = getOrCreateStateBlock(defaultStateDescriptor);
        RenderStateBlockDescriptor defaultStateDescriptorNoDepth;
        defaultStateDescriptorNoDepth.setZReadWrite(false, false);
        _defaultStateNoDepth = getOrCreateStateBlock(defaultStateDescriptorNoDepth);
        RenderStateBlockDescriptor state2DRenderingDesc;
        state2DRenderingDesc.setCullMode(CULL_MODE_NONE);
        state2DRenderingDesc.setZReadWrite(false, true);
        _state2DRendering = getOrCreateStateBlock(state2DRenderingDesc);
        RenderStateBlockDescriptor stateDepthOnlyRendering;
        stateDepthOnlyRendering.setColorWrites(false, false, false, false);
        stateDepthOnlyRendering.setZFunc(CMP_FUNC_ALWAYS);
        _stateDepthOnlyRendering = getOrCreateStateBlock(stateDepthOnlyRendering);

        SET_DEFAULT_STATE_BLOCK(true);
        //Screen FB should use MSAA if available, else fallback to normal color FB (no AA or FXAA)
        _renderTarget[RENDER_TARGET_SCREEN] = newFB(true);
        _renderTarget[RENDER_TARGET_DEPTH]  = newFB(false);

        SamplerDescriptor screenSampler;
        TextureDescriptor screenDescriptor(TEXTURE_2D_MS, RGBA16F, FLOAT_16);
        screenSampler.setFilters(TEXTURE_FILTER_NEAREST, TEXTURE_FILTER_NEAREST);
        screenSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
        screenSampler.toggleMipMaps(false);
        screenDescriptor.setSampler(screenSampler);

        SamplerDescriptor depthSamplerHiZ;
        depthSamplerHiZ.setFilters(TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST, TEXTURE_FILTER_NEAREST);
        depthSamplerHiZ.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
        depthSamplerHiZ.toggleMipMaps(true);
        depthSamplerHiZ._useRefCompare = true; //< Use compare function
        depthSamplerHiZ._cmpFunc = CMP_FUNC_GEQUAL; //< Use greater or equal

        SamplerDescriptor depthSampler;
        depthSampler.setFilters(TEXTURE_FILTER_NEAREST);
        depthSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
        depthSampler.toggleMipMaps(false);
        depthSampler._useRefCompare = true; //< Use compare function
        depthSampler._cmpFunc = CMP_FUNC_GEQUAL; //< Use greater or equal

        TextureDescriptor depthDescriptorHiZ(TEXTURE_2D_MS, DEPTH_COMPONENT32F, FLOAT_32);
        depthDescriptorHiZ.setSampler(depthSamplerHiZ);

        TextureDescriptor depthDescriptor(TEXTURE_2D_MS, DEPTH_COMPONENT32F, FLOAT_32);
        depthDescriptor.setSampler(depthSampler);

        _renderTarget[RENDER_TARGET_DEPTH]->AddAttachment(depthDescriptorHiZ, TextureDescriptor::Depth);
        _renderTarget[RENDER_TARGET_DEPTH]->toggleColorWrites(false);
        _renderTarget[RENDER_TARGET_DEPTH]->Create(resolution.width, resolution.height);

        _renderTarget[RENDER_TARGET_SCREEN]->AddAttachment(screenDescriptor, TextureDescriptor::Color0);
        _renderTarget[RENDER_TARGET_SCREEN]->AddAttachment(depthDescriptor,  TextureDescriptor::Depth);
        _renderTarget[RENDER_TARGET_SCREEN]->Create(resolution.width, resolution.height);

        if(_enableAnaglyph){
            _renderTarget[RENDER_TARGET_ANAGLYPH] = newFB(true);
            _renderTarget[RENDER_TARGET_ANAGLYPH]->AddAttachment(screenDescriptor, TextureDescriptor::Color0);
            _renderTarget[RENDER_TARGET_ANAGLYPH]->AddAttachment(depthDescriptor,  TextureDescriptor::Depth);
            _renderTarget[RENDER_TARGET_ANAGLYPH]->Create(resolution.width, resolution.height);
        }

        _postFX.init(resolution);
        add2DRenderFunction(DELEGATE_BIND(&GFXDevice::previewDepthBuffer, this), 0);
        _kernel->getCameraMgr().addCameraUpdateListener(DELEGATE_BIND(&ShaderManager::updateCamera, DELEGATE_REF(_shaderManager)));
        _kernel->getCameraMgr().addNewCamera("2DRenderCamera", _2DCamera);

        _HIZConstructProgram = CreateResource<ShaderProgram>(ResourceDescriptor("HiZConstruct"));
        _HIZConstructProgram->UniformTexture("LastMip", 0);
    }

    return hardwareState;
}

void GFXDevice::closeRenderingApi(){
    _shaderManager.destroyInstance();
    _api.closeRenderingApi();
    _loaderThread->join();
    SAFE_DELETE(_loaderThread);

    FOR_EACH(RenderStateMap::value_type& it, _stateBlockMap){
        SAFE_DELETE(it.second);
    }
    _stateBlockMap.clear();

    //Destroy all rendering Passes
    RenderPassManager::getInstance().destroyInstance();

    for (FrameBuffer*& renderTarget : _renderTarget)
        SAFE_DELETE(renderTarget);

    SAFE_DELETE(_matricesBuffer);
}

void GFXDevice::closeRenderer(){
    PRINT_FN(Locale::get("STOP_POST_FX"));
    _postFX.destroyInstance();
    _shaderManager.Destroy();
    PRINT_FN(Locale::get("CLOSING_RENDERER"));
    SAFE_DELETE(_renderer);
}

void GFXDevice::idle() {
    if (!_renderer) return;

    _postFX.idle();
    _shaderManager.idle();
    _api.idle();
}

void GFXDevice::drawPoints(U32 numPoints) {
    updateStates();

    if (_activeShaderProgram)
        _activeShaderProgram->uploadNodeMatrices();

    _api.drawPoints(numPoints); 
}

void GFXDevice::renderInstanceCmd(RenderInstance* const instance, const VertexBuffer::DeferredDrawCommand& cmd){
    //All geometry is stored in VB format
    assert(instance->object3D() != nullptr);
    Object3D* model = instance->object3D();

    if (model->getType() == Object3D::OBJECT_3D_PLACEHOLDER || model->getType() == Object3D::TEXT_3D){
        ERROR_FN(Locale::get("ERROR_GFX_INVALID_OBJECT_TYPE"), model->getName().c_str());
        //Text3D* text = dynamic_cast<Text3D*>(model);
        //drawText(text->getText(),text->getWidth(),text->getFont(),text->getHeight(),false,false);
        //3D text disabled for now - to add later - Ionut
        return;
    }

    if (instance->preDraw()){
        if (!model->onDraw(nullptr, getRenderStage()))
            return;
    }

    updateStates();

    Transform* transform = instance->transform();
    if (transform) {
        Transform* prevTransform = instance->prevTransform();
        if (_interpolationFactor < 0.99 && prevTransform) {
            pushWorldMatrix(transform->interpolate(prevTransform, _interpolationFactor), transform->isUniformScaled());
        }
        else{
            pushWorldMatrix(transform->getGlobalMatrix(), transform->isUniformScaled());
        }
    }

    VertexBuffer* modelVB = instance->buffer();
    if(!modelVB) modelVB = model->getGeometryVB();

    assert(modelVB != nullptr);
    if (cmd._cmd.count == 0){
        //Render our current vertex array object
        modelVB->Draw(false, model->getCurrentLOD());
    }else{
        modelVB->setRangeCount(cmd._cmd.count);
        modelVB->setFirstElement(cmd._cmd.firstIndex);
        modelVB->DrawRange();
    }

    if (transform) popWorldMatrix();
}

void GFXDevice::renderInstance(RenderInstance* const instance){
    renderInstanceCmd(instance, VertexBuffer::DeferredDrawCommand());
}

void GFXDevice::render(const DELEGATE_CBK& renderFunction, const SceneRenderState& sceneRenderState){
    //Call the specific render function that prepares the scene for presentation
    _renderer->render(renderFunction, sceneRenderState);
}

void  GFXDevice::generateCubeMap(FrameBuffer& cubeMap, const vec3<F32>& pos, const DELEGATE_CBK& callback, const vec2<F32>& zPlanes, const RenderStage& renderStage){
    static bool firstCall = true;
    if (firstCall){
        _kernel->getCameraMgr().addNewCamera("_gfxCubeCamera", _cubeCamera);
        firstCall = false;
    }

    //Only use cube map FB's
    if (cubeMap.GetAttachment(TextureDescriptor::Color0)._type != TEXTURE_CUBE_MAP) {
        ERROR_FN(Locale::get("ERROR_GFX_DEVICE_INVALID_FB_CUBEMAP"));
        return;
    }

    assert(!callback.empty());

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

    //set a 90 degree vertical FoV perspective projection
    _cubeCamera->setProjection(1.0f, 90.0f, zPlanes);
    _kernel->getCameraMgr().pushActiveCamera(_cubeCamera, false);

    //And set the current render stage to
    RenderStage prevRenderStage = setRenderStage(renderStage);
    //Bind our FB
    cubeMap.Begin(FrameBuffer::defaultPolicy());
   
    //For each of the environment's faces (TOP,DOWN,NORTH,SOUTH,EAST,WEST)
    for(U8 i = 0; i < 6; i++){
        // true to the current cubemap face
        cubeMap.DrawToFace(TextureDescriptor::Color0, i);
        ///Set our Rendering API to render the desired face
        _cubeCamera->lookAt(pos, TabCenter[i], TabUp[i]);
        _cubeCamera->renderLookAt();
        //draw our scene
        render(callback, GET_ACTIVE_SCENE()->renderState());
    }
    //Unbind this fb
    cubeMap.End();
    //Return to our previous rendering stage
    setRenderStage(prevRenderStage);
    _kernel->getCameraMgr().popActiveCamera();
}

RenderStateBlock* GFXDevice::getOrCreateStateBlock(RenderStateBlockDescriptor& descriptor){
   size_t hashValue = descriptor.getHash();

   if (_stateBlockMap.find(hashValue) != _stateBlockMap.end())
      return _stateBlockMap[hashValue];

   RenderStateBlock* result = New RenderStateBlock(descriptor);
   _stateBlockMap.insert(std::make_pair(hashValue,result));

   return result;
}

RenderStateBlock* GFXDevice::setStateBlock(const RenderStateBlock& block, bool forceUpdate) {
   
   RenderStateBlock* prev = _newStateBlock;
   if (!_currentStateBlock || !block.Compare(*_currentStateBlock)) {
       _deviceStateDirty = _stateBlockDirty = true;
       _newStateBlock = const_cast<RenderStateBlock*>(&block);
       if(forceUpdate)  updateStates();//<there is no need to force a internal update of stateblocks if nothing changed
   } else {
       _stateBlockDirty = false;
       _newStateBlock = _currentStateBlock;
   }

   return prev;
}

void GFXDevice::updateStates(bool force) {
    RenderStateBlock* old = nullptr;
    //Verify render states
    if(force){
        _stateBlockDirty = true;
    }else{
        old = _currentStateBlock;
    }

    if (_newStateBlock && _stateBlockDirty){
        activateStateBlock(*_newStateBlock, old);
        _stateBlockDirty = false;
    }

    _currentStateBlock = _newStateBlock;
    
    updateClipPlanes();
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

    // Set the viewport to be the entire window
    setViewport(vec4<I32>(0, 0, w, h));
    changeResolutionInternal(w,h);

    //Update post-processing render targets and buffers
    _postFX.updateResolution(w, h);
    //Refresh shader programs
    _shaderManager.refresh();

    // 2D rendering. Identity view matrix, ortho projection
    _2DCamera->setProjection(vec4<F32>(0, w, 0, h), vec2<F32>(-1, 1));
}

void GFXDevice::enableFog(F32 density, const vec3<F32>& color){
    ParamHandler& par = ParamHandler::getInstance();
    par.setParam("rendering.sceneState.fogColor.r", color.r);
    par.setParam("rendering.sceneState.fogColor.g", color.g);
    par.setParam("rendering.sceneState.fogColor.b", color.b);
    par.setParam("rendering.sceneState.fogDensity",density);
    _shaderManager.refresh();
}

 void GFXDevice::popWorldMatrix(){
     _worldMatrices.pop();
     _isUniformedScaled = _WDirty = true;
     _shaderManager.setMatricesDirty();
}

void GFXDevice::pushWorldMatrix(const mat4<F32>& worldMatrix, const bool isUniformedScaled){
    _worldMatrices.push(worldMatrix);
    _isUniformedScaled = isUniformedScaled;
    _WDirty = true;
    _shaderManager.setMatricesDirty();
}
       
void GFXDevice::getMatrix(const EXTENDED_MATRIX& mode, mat3<GLfloat>& mat){
     assert(mode == NORMAL_MATRIX /*|| mode == ... */);
     cleanMatrices();
            
     // Normal Matrix
     mat.set(_isUniformedScaled ? _WVCachedMatrix : _WVCachedMatrix.inverseTranspose());
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
    if(!_WDirty)
        return;

    assert(!_worldMatrices.empty());

    if (_VDirty) 	        _api.getMatrix(VIEW_MATRIX, _viewCacheMatrix);
    if (_VDirty || _PDirty) _api.getMatrix(VIEW_PROJECTION_MATRIX, _VPCachedMatrix);

    // we transpose the matrices when we use them in the shader
    _WVCachedMatrix.set(_worldMatrices.top() * _viewCacheMatrix);
    _WVPCachedMatrix.set(_worldMatrices.top() * _VPCachedMatrix);

    _VDirty = _PDirty = _WDirty = false;
}

void GFXDevice::toggle2D(bool state) {
    static RenderStateBlock* previousStateBlock = nullptr;
    if (state == _2DRendering) return;
#ifdef _DEBUG
    assert((state && !_2DRendering) || (!state && _2DRendering));
#endif
    _2DRendering = state;

    if (state){ //2D
        previousStateBlock = SET_STATE_BLOCK(*_state2DRendering);
        _kernel->getCameraMgr().pushActiveCamera(_2DCamera);
        _2DCamera->renderLookAt();
    }else{ //3D
        _kernel->getCameraMgr().popActiveCamera();
        SET_STATE_BLOCK(*previousStateBlock);
    }
}

void GFXDevice::previewDepthBuffer(){
    if (!_previewDepthBuffer)
        return;

    if(!_previewDepthMapShader){
        ParamHandler& par = ParamHandler::getInstance();
        ResourceDescriptor shadowPreviewShader("fbPreview.LinearDepth");
        _previewDepthMapShader = CreateResource<ShaderProgram>(shadowPreviewShader);
        assert(_previewDepthMapShader != nullptr);
        _previewDepthMapShader->UniformTexture("tex", 0);
        _previewDepthMapShader->Uniform("useScenePlanes", true);
        _previewDepthMapShader->Uniform("dvd_sceneZPlanes", vec2<F32>(par.getParam<F32>("rendering.zNear"), par.getParam<F32>("rendering.zFar")));
    }

    if(_previewDepthMapShader->getState() != RES_LOADED)
        return;

    _previewDepthMapShader->bind();
    _renderTarget[RENDER_TARGET_DEPTH]->Bind(0, TextureDescriptor::Depth);
    
    renderInViewport(vec4<I32>(Application::getInstance().getResolution().width-256,0,256,256), DELEGATE_BIND(&GFXDevice::drawPoints, this, 1));
}

void GFXDevice::updateClipPlanes() {
    if (!_clippingPlanesDirty)
        return;

    _api.updateClipPlanes(); 
    _clippingPlanesDirty = false;
    _shaderManager.updateClipPlanes();
}

void GFXDevice::postProcessingEnabled(const bool state) {
    if (_enablePostProcessing != state){
        _enablePostProcessing = state; 
        if(state) _postFX.idle();
    }
}

void GFXDevice::updateViewMatrix(const F32* viewMatrixData)  {
    const size_t mat4Size = 16 * sizeof(F32);

    _matricesBuffer->ChangeSubData(mat4Size, 2 * mat4Size, viewMatrixData);
     _VDirty = true;
}

void GFXDevice::updateProjMatrix(const F32* projMatrixData)  { 
    const size_t mat4Size = 16 * sizeof(F32);

    _matricesBuffer->ChangeSubData(0, 3 * mat4Size, projMatrixData);
    _PDirty = true; 
}

void GFXDevice::restoreViewport(){
    if (!_viewportUpdate)  return;
    if (!_viewportForced) _viewport.pop(); //push / pop only if new viewport (not-forced)

    updateViewportInternal(_viewport.top());
}

vec4<I32> GFXDevice::setViewport(const vec4<I32>& viewport, bool force){
    _viewportUpdate = !viewport.compare(_viewport.top());

    if (_viewportUpdate) {

        _viewportForced = force;
        if (!_viewportForced) _viewport.push(viewport); //push / pop only if new viewport (not-forced)
        else                  _viewport.top() = viewport;

        updateViewportInternal(viewport);
    }

    return viewport;
}

void GFXDevice::updateViewportInternal(const vec4<I32>& viewport){
    static vec4<F32> viewportF;
    const size_t vec4Size = 4 * sizeof(F32);
    const size_t mat4Size = 16 * sizeof(F32);
    viewportF.set(viewport.x, viewport.y, viewport.z, viewport.w);
    changeViewport(viewport);
    _matricesBuffer->ChangeSubData(3 * mat4Size, vec4Size, &viewportF[0]);
}

bool GFXDevice::loadInContext(const CurrentContext& context, const DELEGATE_CBK& callback) {
    if (callback.empty())
        return false;

    if (context == GFX_LOADING_CONTEXT){
        while (!_loadQueue.push(callback));
        if (!_loaderThread)
            _loaderThread = New boost::thread(&GFXDevice::loadInContextInternal, this);
    }else{
        callback();
    }
    return true;
}


void GFXDevice::ConstructHIZ() {
    static FrameBuffer::FrameBufferTarget hizTarget;
    hizTarget._clearBuffersOnBind = false;
    hizTarget._changeViewport = false;
    _HIZConstructProgram->bind();
    // disable color buffer as we will render only a depth image
    // we have to disable depth testing but allow depth writes
    setStateBlock(*_stateDepthOnlyRendering);
    
    _renderTarget[RENDER_TARGET_DEPTH]->Begin(hizTarget);
    _renderTarget[RENDER_TARGET_DEPTH]->Bind(0, TextureDescriptor::Depth);
    vec2<U16> resolution = _renderTarget[RENDER_TARGET_DEPTH]->getResolution();
    // calculate the number of mipmap levels for NPOT texture
    I32 numLevels = 1 + (I32)floorf(log2f(fmaxf((F32)resolution.width, (F32)resolution.height)));
    I32 currentWidth = resolution.width;
    I32 currentHeight = resolution.height;
    for (int i = 1; i<numLevels; i++) {
        _HIZConstructProgram->Uniform("LastMipSize", vec2<I32>(currentWidth, currentHeight));
        // calculate next viewport size
        currentWidth  /= 2;
        currentHeight /= 2;
        // ensure that the viewport size is always at least 1x1
        currentWidth = currentWidth > 0 ? currentWidth : 1;
        currentHeight = currentHeight > 0 ? currentHeight : 1;
        setViewport(vec4<I32>(0, 0, currentWidth, currentHeight));
        // bind next level for rendering but first restrict fetches only to previous level
        _renderTarget[RENDER_TARGET_DEPTH]->SetMipLevel(i - 1, TextureDescriptor::Depth);
        // dummy draw command as the full screen quad is generated completely by a geometry shader
        drawPoints(1);
    }
    // reset mipmap level range for the depth image
    _renderTarget[RENDER_TARGET_DEPTH]->ResetMipLevel(TextureDescriptor::Depth);
    _renderTarget[RENDER_TARGET_DEPTH]->End();
    setViewport(vec4<I32>(0, 0, resolution.width, resolution.height));
    
    SET_DEFAULT_STATE_BLOCK(true);
}