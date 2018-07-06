#include "Headers/GFXDevice.h"

#include "Core/Headers/Application.h"
#include "Scenes/Headers/SceneState.h"
#include "Managers/Headers/CameraManager.h"
#include "Core/Resources/Headers/ResourceCache.h"

void GFXDevice::previewDepthBuffer(){
    if (!_previewDepthBuffer)
        return;

    if(!_previewDepthMapShader){
        ResourceDescriptor shadowPreviewShader("fbPreview.LinearDepth");
        _previewDepthMapShader = CreateResource<ShaderProgram>(shadowPreviewShader);
        assert(_previewDepthMapShader != nullptr);
        _previewDepthMapShader->UniformTexture("tex", 0);
        _previewDepthMapShader->Uniform("useScenePlanes", true);
    }

    if(_previewDepthMapShader->getState() != RES_LOADED)
        return;

    _previewDepthMapShader->bind();
    _renderTarget[RENDER_TARGET_DEPTH]->Bind(0, TextureDescriptor::Depth);
    
    renderInViewport(vec4<I32>(Application::getInstance().getResolution().width-256,0,256,256), DELEGATE_BIND(&GFXDevice::drawPoints, this, 1, _defaultStateNoDepthHash));
}

void GFXDevice::debugDraw(const SceneRenderState& sceneRenderState) {
    drawDebugAxis(sceneRenderState);

    _imShader->bind();

    for(IMPrimitive* priv : _imInterfaces){
        if(priv->paused())
            continue;

        if(!priv->inUse() && priv->_canZombify) {
            priv->zombieCounter(priv->zombieCounter() + 1);
            continue;
        }
        
        setStateBlock(priv->stateHash());

        priv->setupStates();
        
        if(priv->_hasLines)
            setLineWidth(priv->_lineWidth);
        
        bool texture = (priv->_texture != nullptr);

        if(texture)
            priv->_texture->Bind(0);
        
        _imShader->Uniform("useTexture", texture);
        _imShader->Uniform("dvd_WorldMatrix", priv->worldMatrix());

        priv->render(1, priv->forceWireframe());

        if(priv->_hasLines)
            restoreLineWidth();
      
        priv->resetStates();

        if(priv->_canZombify)
            priv->inUse(false);
    }
    
    _imShader->unbind();
}

void GFXDevice::drawDebugAxis(const SceneRenderState& sceneRenderState) {
    if(!drawDebugAxis()) return;

    if(_axisLines.empty()){
        //Red X-axis
        _axisLines.push_back(Line(VECTOR3_ZERO, WORLD_X_AXIS, vec4<U8>(255, 0, 0, 255)));
        //Green Y-axis
        _axisLines.push_back(Line(VECTOR3_ZERO, WORLD_Y_AXIS, vec4<U8>(0, 255, 0, 255))); 
        //Blue Z-axis
        _axisLines.push_back(Line(VECTOR3_ZERO, WORLD_Z_AXIS, vec4<U8>(0, 0, 255, 255)));
    }

    const Camera& cam = sceneRenderState.getCameraConst();

    mat4<F32> offset(- cam.getViewDir() * 2, VECTOR3_ZERO, cam.getUpDir());

    drawLines(_axisLines, offset * _viewMatrix.getInverse(), vec4<I32>(_renderTarget[RENDER_TARGET_SCREEN]->getWidth() - 128, 0, 128, 128), true, true);
}