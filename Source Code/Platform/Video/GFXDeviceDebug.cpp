#include "Headers/GFXDevice.h"

#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Scenes/Headers/SceneState.h"
#include "Managers/Headers/CameraManager.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/Framebuffer/Headers/Framebuffer.h"

namespace Divide {
/// Show the contents of the depth buffer in a small rectangle in the bottom
/// right of the screen
void GFXDevice::previewDepthBuffer() {
// As this is touched once per frame, we'll only enable it in debug builds
#ifdef _DEBUG
    // Early out if we didn't request the preview
    if (!ParamHandler::getInstance().getParam<bool>(
        _ID("rendering.previewDepthBuffer"), false)) {
        return;
    }
    // Lazy-load preview shader
    if (!_previewDepthMapShader) {
        // The LinearDepth variant converts the depth values to linear values
        // between the 2 scene z-planes
        ResourceDescriptor fbPreview("fbPreview.LinearDepth.ScenePlanes");
        fbPreview.setPropertyList("USE_SCENE_ZPLANES");
        _previewDepthMapShader = CreateResource<ShaderProgram>(fbPreview);
        assert(_previewDepthMapShader != nullptr);

        ResourceDescriptor previewNormalsShader("fbPreview");
        previewNormalsShader.setThreadedLoading(false);
        _previewNormalsShader = CreateResource<ShaderProgram>(previewNormalsShader);
        assert(_previewNormalsShader != nullptr);
    }

    if (_previewDepthMapShader->getState() != ResourceState::RES_LOADED) {
        return;
    }

    { //Depth preview
        Texture* depthTex = _renderTarget[to_uint(RenderTarget::SCREEN)]->getAttachment(TextureDescriptor::AttachmentType::Depth);
        _previewDepthMapShader->Uniform("lodLevel", to_float(to_uint((Time::ElapsedMilliseconds() / 750.0)) % (depthTex->getMaxMipLevel() - 1)));
        depthTex->Bind(to_ubyte(ShaderProgram::TextureUsage::UNIT0));
        GFX::ScopedViewport viewport(_renderTarget[to_uint(RenderTarget::SCREEN)]->getResolution().width - 256, 0,256, 256);
        drawTriangle(_defaultStateNoDepthHash, _previewDepthMapShader);
    }
    { //Normals preview
        Texture* normalsPreview = _renderTarget[to_uint(RenderTarget::SCREEN)]->getAttachment(TextureDescriptor::AttachmentType::Color1);
        normalsPreview->Bind(to_ubyte(ShaderProgram::TextureUsage::UNIT0));
        GFX::ScopedViewport viewport(_renderTarget[to_uint(RenderTarget::SCREEN)]->getResolution().width - 512, 0, 256, 256);
        drawTriangle(_defaultStateNoDepthHash, _previewNormalsShader);
    }
#endif
}

/// Render all of our immediate mode primitives. This isn't very optimised and
/// most are recreated per frame!
void GFXDevice::debugDraw(const SceneRenderState& sceneRenderState) {
    uploadGPUBlock();
    // We need a shader that emulates the fixed pipeline in order to continue
    if (!_imShader->bind()) {
        return;
    }
    // Debug axis form the axis arrow gizmo in the corner of the screen
    drawDebugAxis(sceneRenderState);
    // Loop over all available primitives
    for (IMPrimitive* prim : _imInterfaces) {
        // A primitive may be paused if drawing it isn't desired at the current
        // point in time
        if (prim->paused()) {
            continue;
        }
        // Inform the primitive that we're using the imShader
        // A primitive can be rendered with any shader program available, so
        // make sure we actually use the right one for this stage
        if (prim->drawShader() == nullptr) {
            prim->drawShader(_imShader);
        }
        // Set the primitive's render state block
        setStateBlock(prim->stateHash());
        // Call any "onDraw" function the primitive may have attached
        prim->setupStates();
        // Check if any texture is present
        bool texture = (prim->_texture != nullptr);
        // And bind it to the first diffuse texture slot
        if (texture) {
            prim->_texture->Bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0));
        }
        // Inform the shader if we have (or don't have) a texture
        _imShader->Uniform("useTexture", texture);
        // Upload the primitive's world matrix to the shader
        _imShader->Uniform("dvd_WorldMatrix", prim->worldMatrix());
        // Submit the render call. We do not support instancing yet!
        prim->render(prim->forceWireframe(), 1);
        registerDrawCall();
        // Call any "postDraw" function the primitive may have attached
        prim->resetStates();
        if (prim->_canZombify) {
            prim->paused(true);
        }
    }
}

/// Draw the axis arrow gizmo
void GFXDevice::drawDebugAxis(const SceneRenderState& sceneRenderState) {
    // This is togglable, so check if it's actually requested
    if (!drawDebugAxis()) {
        return;
    }
  
    // Apply the inverse view matrix so that it cancels out in the shader
    // Submit the draw command, rendering it in a tiny viewport in the lower
    // right corner
    U16 windowWidth = _renderTarget[to_uint(RenderTarget::SCREEN)]->getWidth();
    drawLines(*_axisGizmo,
              _axisLines,
              vec4<I32>(windowWidth - 120, 8, 128, 128),
              true);
}
};