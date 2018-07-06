#include "Headers/GFXDevice.h"

#include "Core/Headers/Application.h"
#include "Scenes/Headers/SceneState.h"
#include "Managers/Headers/CameraManager.h"
#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {
/// Show the contents of the depth buffer in a small rectangle in the bottom right of the screen
void GFXDevice::previewDepthBuffer() {
    // As this is touched once per frame, we'll only enable it in debug builds
#   ifdef _DEBUG
        // Early out if we didn't request the preview
        if (!_previewDepthBuffer) {
            return;
        }
        // Lazy-load preview shader
        if (!_previewDepthMapShader) {
            // The LinearDepth variant converts the depth values to linear values between the 2 scene z-planes
            _previewDepthMapShader = CreateResource<ShaderProgram>(ResourceDescriptor("fbPreview.LinearDepth"));
            assert(_previewDepthMapShader != nullptr);
            _previewDepthMapShader->Uniform("useScenePlanes", true);
        }

        if (_previewDepthMapShader->getState() != RES_LOADED) {
            return;
        }

        _renderTarget[RENDER_TARGET_DEPTH]->Bind(ShaderProgram::TEXTURE_UNIT0, TextureDescriptor::Depth);

		renderInViewport(vec4<I32>(Application::getInstance().getResolution().width - 256, 0, 256, 256), 
			             DELEGATE_BIND((void(GFXDevice::*)(U32, size_t, ShaderProgram* const))&GFXDevice::drawPoints, this, 1, _defaultStateNoDepthHash, _previewDepthMapShader));
#   endif
}

/// Render all of our immediate mode primitives. This isn't very optimised and most are recreated per frame!
void GFXDevice::debugDraw(const SceneRenderState& sceneRenderState) {
    // We need a shader that emulates the fixed pipeline in order to continue
    if (!_imShader->bind()) {
        return;
    }
    // Debug axis form the axis arrow gizmo in the corner of the screen
    drawDebugAxis(sceneRenderState);
    // Loop over all available primitives
    for (IMPrimitive* priv : _imInterfaces) {
        // A primitive may be paused if drawing it isn't desired at the current point in time
        if (priv->paused()) {
            continue;
        }
        // If the current primitive isn't in use, and can be recycled, increase it's "zombie counter" 
        if (!priv->inUse() && priv->_canZombify) {
            // The zombie counter represents the number of frames since the primitive was last used
            priv->zombieCounter(priv->zombieCounter() + 1);
            continue;
        }
        // Inform the primitive that we're using the imShader
        // A primitive can be rendered with any shader program available, so make sure we actually use the right one for this stage
        priv->drawShader(_imShader);
        // Set the primitive's render state block
        setStateBlock(priv->stateHash());
        // Call any "onDraw" function the primitive may have attached
        priv->setupStates();
        // If we are drawing lines, set the required width
        if (priv->_hasLines) {
            setLineWidth(priv->_lineWidth);
        }
        // Check if any texture is present
        bool texture = (priv->_texture != nullptr);
        // And bind it to the first diffuse texture slot
        if (texture) {
            priv->_texture->Bind(ShaderProgram::TEXTURE_UNIT0);
        }
        // Inform the shader if we have (or don't have) a texture
        _imShader->Uniform("useTexture", texture);
        // Upload the primitive's world matrix to the shader
        _imShader->Uniform("dvd_WorldMatrix", priv->worldMatrix());
        // Submit the render call. We do not support instancing yet!
        priv->render(priv->forceWireframe(), 1);
        // Reset line width if needed
        if (priv->_hasLines) {
            restoreLineWidth();
        }
        // Call any "postDraw" function the primitive may have attached
        priv->resetStates();
        // If this primitive is recyclable, clear it's inUse flag. It should be recreated next frame
        if (priv->_canZombify) {
            priv->inUse(false);
        }
    }
}

/// Draw the axis arrow gizmo
void GFXDevice::drawDebugAxis(const SceneRenderState& sceneRenderState) {
    // This is togglable, so check if it's actually requested 
    if (!drawDebugAxis()) {
        return;
    }
    // We need to transform the gizmo so that it always remains axis aligned
    const Camera& cam = sceneRenderState.getCameraConst();
    // Create a world matrix using a look at function with the eye position backed up from the camera's view direction
    mat4<F32> offset(- cam.getViewDir() * 2, VECTOR3_ZERO, cam.getUpDir());
    // Apply the inverse view matrix so that it cancels out in the shader
    // Submit the draw command, rendering it in a tiny viewport in the lower right corner
    drawLines(_axisLines, offset * _gpuBlock._ViewMatrix.getInverse(), vec4<I32>(_renderTarget[RENDER_TARGET_SCREEN]->getWidth() - 128, 0, 128, 128), true, true);
}

};