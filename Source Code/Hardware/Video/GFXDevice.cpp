#include "Headers/GFXDevice.h"
#include "Headers/RenderStateBlock.h"

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIText.h"
#include "GUI/Headers/GUIFlash.h"
#include "Core/Math/Headers/Plane.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Utility/Headers/ImageTools.h"
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

GFXDevice::GFXDevice() : _api(GL_API::getOrCreateInstance()),
                         _postFX(PostFX::getOrCreateInstance()),
                         _shaderManager(ShaderManager::getOrCreateInstance()), //<Defaulting to OpenGL if no api has been defined
                         _loaderThread(nullptr), _renderer(nullptr), _maxTextureSlots(0),
                         _interpolationFactor(1.0), _renderStage(INVALID_STAGE)
{
   _MSAASamples = _FXAASamples = 0;
   _previousLineWidth = _currentLineWidth = 1.0;
   _prevTextureId = _stateExclusionMask = _prevShaderId = 0;
   _gfxDataBuffer = _nodeBuffer = nullptr;
   _isDepthPrePass = _previewDepthBuffer = _viewportUpdate = false;
   _2DRendering = _drawDebugAxis = _enablePostProcessing = _enableAnaglyph = _enableHDR = false;
   _previewDepthMapShader = _imShader = _activeShaderProgram = nullptr;
   _HIZConstructProgram = _depthRangesConstructProgram = nullptr;
   _rasterizationEnabled = true;
   _generalDetailLevel = _shadowDetailLevel = DETAIL_HIGH;
   _state2DRenderingHash = _stateDepthOnlyRenderingHash = _defaultStateBlockHash = 0;
   _defaultStateNoDepthHash = _currentStateBlockHash = 0;

   _clippingPlanes.resize(Config::MAX_CLIP_PLANES, Plane<F32>(0,0,0,0));

   _cubeCamera = New FreeFlyCamera();
   _2DCamera = New FreeFlyCamera();
   _2DCamera->lockView(true);
   _2DCamera->lockFrustum(true);

   for (Framebuffer*& renderTarget : _renderTarget)
       renderTarget = nullptr;

   RenderPassManager::getOrCreateInstance().addRenderPass(New RenderPass("diffusePass"), 1);
   //RenderPassManager::getInstance().addRenderPass(shadowPass,2);
}

GFXDevice::~GFXDevice()
{
}

void GFXDevice::setBufferData(const GenericDrawCommand& cmd) {
    DIVIDE_ASSERT(cmd._shaderProgram && cmd._shaderProgram->getId() != 0,
                  "GFXDevice error: Draw shader state is not valid for the current draw operation!");

    cmd._shaderProgram->bind();
    cmd._shaderProgram->Uniform("lodLevel", (I32)cmd._lodIndex);
    
    assert(cmd._drawID < (I32)_matricesData.size());

    _nodeBuffer->BindRange(Divide::SHADER_BUFFER_NODE_INFO, std::max(cmd._drawID, 0), 1);
        
    setStateBlock(cmd._stateHash);
}

void GFXDevice::drawPoints(U32 numPoints, size_t stateHash) {
    if(numPoints == 0)
        return;

    _defaultDrawCmd.setDrawID(-1);
    _defaultDrawCmd.setStateHash(stateHash);
    _defaultDrawCmd.setShaderProgram(activeShaderProgram());

    setBufferData(_defaultDrawCmd);
    drawPoints(numPoints); 
}

void GFXDevice::drawGUIElement(GUIElement* guiElement){
    assert(guiElement);
    if (!guiElement->isVisible()) return;

    setStateBlock(guiElement->getStateBlockHash());

    switch (guiElement->getType()){
        case GUI_TEXT:{
            const GUIText& text = dynamic_cast<GUIText&>(*guiElement);
            drawText(text, text.getPosition());
        }break;
        case GUI_FLASH:
            dynamic_cast<GUIFlash* >(guiElement)->playMovie();
            break;
        default:
            break;
    };
    guiElement->lastDrawTimer(GETUSTIME());
}

void GFXDevice::submitRenderCommand(VertexDataInterface* const buffer, const GenericDrawCommand& cmd){
    assert(buffer != nullptr);
    if(cmd._cmd.instanceCount == 0)
        return;

    setBufferData(cmd);
    buffer->Draw(cmd);
}

void GFXDevice::submitRenderCommand(VertexDataInterface* const buffer, const vectorImpl<GenericDrawCommand>& cmds){
    STUBBED("Batch by state hash and submit multiple draw calls! - Ionut");    
    FOR_EACH(const GenericDrawCommand& cmd, cmds){
        submitRenderCommand(buffer, cmd);
    }
}

void  GFXDevice::generateCubeMap(Framebuffer& cubeMap, const vec3<F32>& pos, const DELEGATE_CBK& callback, const vec2<F32>& zPlanes, const RenderStage& renderStage){
    //Only use cube map FB's
    if (cubeMap.GetAttachment(TextureDescriptor::Color0)->getTextureType() != TEXTURE_CUBE_MAP) {
        ERROR_FN(Locale::get("ERROR_GFX_DEVICE_INVALID_FB_CUBEMAP"));
        return;
    }

    assert(!callback.empty());

    static vec3<F32> TabUp[6] = {
        WORLD_Y_NEG_AXIS,  WORLD_Y_NEG_AXIS,
        WORLD_Z_AXIS,      WORLD_Z_NEG_AXIS,
        WORLD_Y_NEG_AXIS,  WORLD_Y_NEG_AXIS
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
    cubeMap.Begin(Framebuffer::defaultPolicy());
   
    //For each of the environment's faces (TOP,DOWN,NORTH,SOUTH,EAST,WEST)
    for(U8 i = 0; i < 6; i++){
        // true to the current cubemap face
        cubeMap.DrawToFace(TextureDescriptor::Color0, i);
        ///Set our Rendering API to render the desired face
        _cubeCamera->lookAt(pos, TabCenter[i], TabUp[i]);
        _cubeCamera->renderLookAt();
        //draw our scene
        getRenderer()->render(callback, GET_ACTIVE_SCENE()->renderState());
    }
    //Unbind this fb
    cubeMap.End();
    //Return to our previous rendering stage
    setRenderStage(prevRenderStage);
    _kernel->getCameraMgr().popActiveCamera();
}

size_t GFXDevice::getOrCreateStateBlock(const RenderStateBlockDescriptor& descriptor){
   size_t hashValue = descriptor.getHash();

   if (_stateBlockMap.find(hashValue) == _stateBlockMap.end())
       _stateBlockMap.insert(std::make_pair(hashValue, New RenderStateBlock(descriptor)));

   return hashValue;
}

size_t GFXDevice::setStateBlock(size_t stateBlockHash) {
   stateBlockHash = stateBlockHash == 0 ? _defaultStateBlockHash : stateBlockHash;
   size_t prevStateHash = _newStateBlockHash;
   if (_currentStateBlockHash == 0 || stateBlockHash != _currentStateBlockHash) {
        _newStateBlockHash = stateBlockHash;
        if (_newStateBlockHash)
            activateStateBlock(*_stateBlockMap[_newStateBlockHash], _stateBlockMap[_currentStateBlockHash]);
        _currentStateBlockHash = _newStateBlockHash;
   } else {
       _newStateBlockHash = _currentStateBlockHash;
   }

   return prevStateHash;
}

const RenderStateBlockDescriptor& GFXDevice::getStateBlockDescriptor(size_t renderStateBlockHash) const {
    RenderStateMap ::const_iterator it = _stateBlockMap.find(renderStateBlockHash);
    assert(it != _stateBlockMap.end());
    return it->second->getDescriptor(); 
}

void GFXDevice::changeResolution(U16 w, U16 h) {
    if(_renderTarget[RENDER_TARGET_SCREEN] != nullptr) {
        if (vec2<U16>(w,h) == _renderTarget[RENDER_TARGET_SCREEN]->getResolution() || !(w > 1 && h > 1)) 
            return;
        
        for (Framebuffer* renderTarget : _renderTarget){
            if (renderTarget)
                renderTarget->Create(w, h);
        }
    }

    // Set the viewport to be the entire window
    forceViewportInternal(vec4<I32>(0, 0, w, h));
    assert(_viewport.size() == 1);
    changeResolutionInternal(w,h);
    //Inform the Kernel
    Kernel::updateResolutionCallback(w,h);
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
    par.setParam("rendering.sceneState.fogDensity", density);
    _shaderManager.refreshSceneData();
}

void GFXDevice::getMatrix(const MATRIX_MODE& mode, mat4<F32>& mat) {
    if (mode == VIEW_PROJECTION_MATRIX)          mat.set(_gpuBlock._ViewProjectionMatrix);
    else if (mode == VIEW_MATRIX)                mat.set(_gpuBlock._ViewMatrix);
    else if (mode == PROJECTION_MATRIX)          mat.set(_gpuBlock._ProjectionMatrix);
    else if (mode == TEXTURE_MATRIX)             mat.set(_textureMatrix);
    else if (mode == VIEW_INV_MATRIX)            _gpuBlock._ViewMatrix.inverse(mat);
    else if (mode == PROJECTION_INV_MATRIX)      _gpuBlock._ProjectionMatrix.inverse(mat);
    else if(mode == VIEW_PROJECTION_INV_MATRIX)  _gpuBlock._ViewProjectionMatrix.inverse(mat);
    else { DIVIDE_ASSERT(mode == -1, "GFXDevice error: attempted to query an invalid matrix target!"); }
}

void GFXDevice::updateClipPlanes(){
    for(U8 i = 0 ; i < Config::MAX_CLIP_PLANES; ++i)
        _gpuBlock._clipPlanes[i] = _clippingPlanes[i].getEquation();

    _gfxDataBuffer->UpdateData(0, sizeof(GPUBlock), &_gpuBlock, true);
}

void GFXDevice::updateViewportInternal(const vec4<I32>& viewport){
    changeViewport(viewport);
    _gpuBlock._ViewPort.set(viewport.x, viewport.y, viewport.z, viewport.w);

    _gfxDataBuffer->UpdateData(0, sizeof(GPUBlock), &_gpuBlock);
}

F32* GFXDevice::lookAt(const mat4<F32>& viewMatrix, const vec3<F32>& eyePos) {
    _gpuBlock._ViewMatrix.set(viewMatrix);
    _gpuBlock._cameraPosition.set(eyePos);
    _gpuBlock._ViewProjectionMatrix.set(_gpuBlock._ViewMatrix * _gpuBlock._ProjectionMatrix);
    _gfxDataBuffer->UpdateData(0, sizeof(GPUBlock), &_gpuBlock);

    return _gpuBlock._ViewMatrix.mat;
}

//Setting ortho projection:
F32* GFXDevice::setProjection(const vec4<F32>& rect, const vec2<F32>& planes) {
    _gpuBlock._ProjectionMatrix.ortho(rect.x, rect.y, rect.z, rect.w, planes.x, planes.y);
    _gpuBlock._ZPlanesCombined.x = planes.x;
    _gpuBlock._ZPlanesCombined.y = planes.y;
    _gpuBlock._ViewProjectionMatrix.set(_gpuBlock._ViewMatrix * _gpuBlock._ProjectionMatrix);
    _gfxDataBuffer->UpdateData(0, sizeof(GPUBlock), &_gpuBlock);

    return _gpuBlock._ProjectionMatrix.mat;
}

//Setting perspective projection:
F32* GFXDevice::setProjection(F32 FoV, F32 aspectRatio, const vec2<F32>& planes) {
    _gpuBlock._ProjectionMatrix.perspective(RADIANS(FoV), aspectRatio, planes.x, planes.y);
    _gpuBlock._ZPlanesCombined.x = planes.x;
    _gpuBlock._ZPlanesCombined.y = planes.y;
    _gpuBlock._ViewProjectionMatrix.set(_gpuBlock._ViewMatrix * _gpuBlock._ProjectionMatrix);
    _gfxDataBuffer->UpdateData(0, sizeof(GPUBlock), &_gpuBlock);

    return _gpuBlock._ProjectionMatrix.mat;
}

namespace {
    ///Used for anaglyph rendering
    struct CameraFrustum {
        D32 leftfrustum;
        D32 rightfrustum;
        D32 bottomfrustum;
        D32 topfrustum;
        F32 modeltranslation;
    } _leftCam, _rightCam;
    F32  _anaglyphIOD = -0.01f;
};

//Setting anaglyph frustum for specified eye
void GFXDevice::setAnaglyphFrustum(F32 camIOD, const vec2<F32>& zPlanes, F32 aspectRatio, F32 verticalFoV, bool rightFrustum) {

    if(!FLOAT_COMPARE(_anaglyphIOD,camIOD)){
        static const D32 DTR = 0.0174532925;
        static const D32 screenZ = 10.0;

        //sets top of frustum based on FoV-Y and near clipping plane
        F32 top = zPlanes.x*tan(DTR * verticalFoV * 0.5f);
        F32 right = aspectRatio*top;
        //sets right of frustum based on aspect ratio
        F32 frustumshift = (camIOD/2)*zPlanes.x/screenZ;

        _leftCam.topfrustum = top;
        _leftCam.bottomfrustum = -top;
        _leftCam.leftfrustum = -right + frustumshift;
        _leftCam.rightfrustum = right + frustumshift;
        _leftCam.modeltranslation = camIOD/2;

        _rightCam.topfrustum = top;
        _rightCam.bottomfrustum = -top;
        _rightCam.leftfrustum = -right - frustumshift;
        _rightCam.rightfrustum = right - frustumshift;
        _rightCam.modeltranslation = -camIOD/2;

        _anaglyphIOD = camIOD;
    }

    CameraFrustum& tempCamera = rightFrustum ? _rightCam : _leftCam;

    _gpuBlock._ProjectionMatrix.frustum(tempCamera.leftfrustum,   tempCamera.rightfrustum,
                                        tempCamera.bottomfrustum, tempCamera.topfrustum,
                                        zPlanes.x, zPlanes.y);

    //translate to cancel parallax
    _gpuBlock._ProjectionMatrix.translate(tempCamera.modeltranslation, 0.0, 0.0);

    _gpuBlock._ZPlanesCombined.x = zPlanes.x;
    _gpuBlock._ZPlanesCombined.y = zPlanes.y;
    _gpuBlock._ViewProjectionMatrix.set(_gpuBlock._ViewMatrix * _gpuBlock._ProjectionMatrix);

    _gfxDataBuffer->UpdateData(0, sizeof(GPUBlock), &_gpuBlock);
}

void GFXDevice::toggle2D(bool state) {
    static size_t previousStateBlockHash = 0;
    if (state == _2DRendering) return;

    assert((state && !_2DRendering) || (!state && _2DRendering));

    _2DRendering = state;

    if (state){ //2D
        previousStateBlockHash = setStateBlock(_state2DRenderingHash);
        _kernel->getCameraMgr().pushActiveCamera(_2DCamera);
        _2DCamera->renderLookAt();
    }else{ //3D
        _kernel->getCameraMgr().popActiveCamera();
        setStateBlock(previousStateBlockHash);
    }
}

void GFXDevice::postProcessingEnabled(const bool state) {
    if (_enablePostProcessing != state){
        _enablePostProcessing = state; 
        if(state) _postFX.idle();
    }
}

void GFXDevice::restoreViewport(){
    if (!_viewportUpdate)
        return;

    _viewport.pop();
    updateViewportInternal(_viewport.top());
    _viewportUpdate = false;
}

void GFXDevice::setViewport(const vec4<I32>& viewport){
    _viewportUpdate = !viewport.compare(_viewport.top());
    
    if (_viewportUpdate) {

        _viewport.push(viewport);

        updateViewportInternal(viewport);
   }
}

void GFXDevice::forceViewportInternal(const vec4<I32>& viewport){
    while(!_viewport.empty())
        _viewport.pop();

    _viewport.push(viewport);
    updateViewportInternal(viewport);
    _viewportUpdate = false;
}

void GFXDevice::processVisibleNodes(const vectorImpl<SceneGraphNode* >& visibleNodes){
    if(visibleNodes.empty()){
        //_nodeBuffer->SetData(NULL);
        return;
    }

    // Generate and upload all lighting data
    
    //LightManager::getInstance().buildLightGrid(_viewMatrix, _projectionMatrix, _gpuBlock._ZPlanesCombined.xy());
    LightManager::getInstance().updateAndUploadLightData(_gpuBlock._ViewMatrix);

    // Generate and upload per node data
    _sgnToDrawIDMap.clear();
    _matricesData.clear();

    _matricesData.reserve(visibleNodes.size() + 1);

    NodeData temp; 
    _matricesData.push_back(temp);

    for(U16 i = 0; i < visibleNodes.size(); ++i){
        SceneGraphNode* crtNode = visibleNodes[i];
        Transform* transform = crtNode->getTransform();
        if (transform) {
            temp._matrix[0].set(crtNode->getWorldMatrix(_interpolationFactor));
            temp._matrix[1].set(temp._matrix[0] * _gpuBlock._ViewMatrix);
            if(!transform->isUniformScaled())
                temp._matrix[1].inverseTranspose();
        }else{
            temp._matrix[0].identity();
            temp._matrix[1].identity();
        }
    
        temp._matrix[1].element(3,2,true) = (F32)LightManager::getInstance().getLights().size();
        temp._matrix[1].element(3,3,true) = (F32)(crtNode->getComponent<AnimationComponent>() ? crtNode->getComponent<AnimationComponent>()->boneCount() : 0);
        
        temp._matrix[2].set(crtNode->getMaterialColorMatrix());
        temp._matrix[3].set(crtNode->getMaterialPropertyMatrix());

        _matricesData.push_back(temp);

        _sgnToDrawIDMap[crtNode->getGUID()] = (I32)_matricesData.size() - 1;
    }

    _nodeBuffer->UpdateData(0,  _matricesData.size() * sizeof(NodeData), &_matricesData.front(), true);
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
    static Framebuffer::FramebufferTarget hizTarget;
    hizTarget._clearBuffersOnBind = false;
    hizTarget._changeViewport = false;
    _HIZConstructProgram->bind();
    // disable color buffer as we will render only a depth image
    // we have to disable depth testing but allow depth writes
    _renderTarget[RENDER_TARGET_DEPTH]->Begin(hizTarget);
    _renderTarget[RENDER_TARGET_DEPTH]->Bind(0, TextureDescriptor::Depth);
    vec2<U16> resolution = _renderTarget[RENDER_TARGET_DEPTH]->getResolution();
    // calculate the number of mipmap levels for NPOT texture
    U32 numLevels = 1 + (U32)floorf(log2f(fmaxf((F32)resolution.width, (F32)resolution.height)));
    I32 currentWidth = resolution.width;
    I32 currentHeight = resolution.height;
    for (U32 i = 1; i < numLevels; i++) {
        _HIZConstructProgram->Uniform("LastMipSize", vec2<I32>(currentWidth, currentHeight));
        // calculate next viewport size
        currentWidth  /= 2;
        currentHeight /= 2;
        // ensure that the viewport size is always at least 1x1
        currentWidth = currentWidth > 0 ? currentWidth : 1;
        currentHeight = currentHeight > 0 ? currentHeight : 1;
        setViewport(vec4<I32>(0, 0, currentWidth, currentHeight));
        // bind next level for rendering but first restrict fetches only to previous level
        _renderTarget[RENDER_TARGET_DEPTH]->SetMipLevel(i - 1, i - 1, i, TextureDescriptor::Depth);
        // dummy draw command as the full screen quad is generated completely by a geometry shader
        drawPoints(1, _stateDepthOnlyRenderingHash);
        restoreViewport();
    }
    
    // reset mipmap level range for the depth image
    _renderTarget[RENDER_TARGET_DEPTH]->ResetMipLevel(TextureDescriptor::Depth);
    _renderTarget[RENDER_TARGET_DEPTH]->End();

}

void GFXDevice::DownSampleDepthBuffer(vectorImpl<vec2<F32>> &depthRanges){

    _renderTarget[RENDER_TARGET_DEPTH_RANGES]->Begin(Framebuffer::defaultPolicy());
    _renderTarget[RENDER_TARGET_DEPTH]->Bind(0, TextureDescriptor::Depth);
    _depthRangesConstructProgram->bind();
    _depthRangesConstructProgram->Uniform("dvd_ProjectionMatrixInverse", getMatrix(PROJECTION_INV_MATRIX));
    _depthRangesConstructProgram->Uniform("dvd_screenDimension", _renderTarget[RENDER_TARGET_DEPTH]->getResolution());
    drawPoints(1, _defaultStateNoDepthHash);
    depthRanges.resize(_renderTarget[RENDER_TARGET_DEPTH_RANGES]->getWidth() * _renderTarget[RENDER_TARGET_DEPTH_RANGES]->getHeight());
    _renderTarget[RENDER_TARGET_DEPTH_RANGES]->ReadData(RG, FLOAT_32, &depthRanges[0]);
    _renderTarget[RENDER_TARGET_DEPTH_RANGES]->End();
}

void GFXDevice::Screenshot(char* filename){
    const vec2<U16>& resolution = _renderTarget[RENDER_TARGET_SCREEN]->getResolution();
    // allocate memory for the pixels
    U8 *imageData = New U8[resolution.width * resolution.height * 4];
    // read the pixels from the frame 
    _renderTarget[RENDER_TARGET_SCREEN]->ReadData(RGBA, UNSIGNED_BYTE, imageData);
    //save to file
    ImageTools::SaveSeries(filename, vec2<U16>(resolution.width, resolution.height), 32, imageData);
    SAFE_DELETE_ARRAY(imageData);
}

IMPrimitive* GFXDevice::getOrCreatePrimitive(bool allowPrimitiveRecycle) {
    IMPrimitive* tempPriv = nullptr;
    //Find a zombified primitive
    vectorImpl<IMPrimitive* >::iterator it = std::find_if(_imInterfaces.begin(),_imInterfaces.end(), 
                                                          [](IMPrimitive* const priv){return (priv && !priv->inUse());});
    if(allowPrimitiveRecycle && it != _imInterfaces.end()){//If we have one ...
        tempPriv = *it;
        //... resurrect it
        tempPriv->clear();
    }else{//If we do not have a valid zombie, add a new element
        tempPriv = newIMP();
        _imInterfaces.push_back(tempPriv);
    }

    tempPriv->_canZombify = allowPrimitiveRecycle;

    return tempPriv;
}

void GFXDevice::drawBox3D(const vec3<F32>& min, const vec3<F32>& max, const mat4<F32>& globalOffset){
    IMPrimitive* priv = getOrCreatePrimitive();

    priv->_hasLines = true;
    priv->_lineWidth = 4.0f;

    priv->stateHash(_defaultStateBlockHash);
    priv->worldMatrix(globalOffset);
 
    priv->beginBatch();

    vec4<U8> color(0,0,255,255);
    priv->attribute4ub("inColorData", color);

    priv->begin(LINE_LOOP);
        priv->vertex( vec3<F32>(min.x, min.y, min.z) );
        priv->vertex( vec3<F32>(max.x, min.y, min.z) );
        priv->vertex( vec3<F32>(max.x, min.y, max.z) );
        priv->vertex( vec3<F32>(min.x, min.y, max.z) );
    priv->end();

    priv->begin(LINE_LOOP);
        priv->vertex( vec3<F32>(min.x, max.y, min.z) );
        priv->vertex( vec3<F32>(max.x, max.y, min.z) );
        priv->vertex( vec3<F32>(max.x, max.y, max.z) );
        priv->vertex( vec3<F32>(min.x, max.y, max.z) );
    priv->end();

    priv->begin(LINES);
        priv->vertex( vec3<F32>(min.x, min.y, min.z) );
        priv->vertex( vec3<F32>(min.x, max.y, min.z) );
        priv->vertex( vec3<F32>(max.x, min.y, min.z) );
        priv->vertex( vec3<F32>(max.x, max.y, min.z) );
        priv->vertex( vec3<F32>(max.x, min.y, max.z) );
        priv->vertex( vec3<F32>(max.x, max.y, max.z) );
        priv->vertex( vec3<F32>(min.x, min.y, max.z) );
        priv->vertex( vec3<F32>(min.x, max.y, max.z) );
    priv->end();

    priv->endBatch();
}

void GFXDevice::drawLines(const vectorImpl<Line >& lines,
                          const mat4<F32>& globalOffset,
                          const vec4<I32>& viewport,
                          const bool inViewport,
                          const bool disableDepth){

    if(lines.empty()) return;
    
    IMPrimitive* priv = getOrCreatePrimitive();

    priv->_hasLines = true;
    priv->_lineWidth = 5.0f;
    priv->worldMatrix(globalOffset);
    priv->stateHash(getDefaultStateBlock(disableDepth));
   
    if(inViewport)
        priv->setRenderStates(DELEGATE_BIND(&GFXDevice::setViewport, DELEGATE_REF(GFX_DEVICE), viewport),
                              DELEGATE_BIND(&GFXDevice::restoreViewport, DELEGATE_REF(GFX_DEVICE)));            
    priv->beginBatch();

    priv->attribute4ub("inColorData", lines[0]._color);

    priv->begin(LINES);

    for(size_t i = 0; i < lines.size(); i++){
        priv->attribute4ub("inColorData", lines[i]._color);
        priv->vertex( lines[i]._startPoint );
        priv->vertex( lines[i]._endPoint );
    }

    priv->end();
    priv->endBatch();
}

