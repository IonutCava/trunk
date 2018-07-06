#include "Headers/GFXDevice.h"

#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Scenes/Headers/SceneState.h"
#include "Managers/Headers/CameraManager.h"
#include "Core/Resources/Headers/ResourceCache.h"

namespace Divide {
/// Show the contents of the depth buffer in a small rectangle in the bottom
/// right of the screen
void GFXDevice::previewDepthBuffer() {
// As this is touched once per frame, we'll only enable it in debug builds
#ifdef _DEBUG
    // Early out if we didn't request the preview
    if (!ParamHandler::getInstance().getParam<bool>(
            "rendering.previewDepthBuffer", false)) {
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
    }

    if (_previewDepthMapShader->getState() != ResourceState::RES_LOADED) {
        return;
    }

    _renderTarget[to_uint(RenderTarget::DEPTH)]->Bind(
        static_cast<U8>(ShaderProgram::TextureUsage::UNIT0),
        TextureDescriptor::AttachmentType::Depth);

    GFX::ScopedViewport viewport(Application::getInstance().getResolution().width - 256, 0,256, 256);
    drawPoints(1, _defaultStateNoDepthHash, _previewDepthMapShader);
#endif
}

/// Render all of our immediate mode primitives. This isn't very optimised and
/// most are recreated per frame!
void GFXDevice::debugDraw(const SceneRenderState& sceneRenderState) {
    uploadGlobalBufferData();
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
        prim->drawShader(_imShader);
        // Set the primitive's render state block
        setStateBlock(prim->stateHash());
        // Call any "onDraw" function the primitive may have attached
        prim->setupStates();
        // Check if any texture is present
        bool texture = (prim->_texture != nullptr);
        // And bind it to the first diffuse texture slot
        if (texture) {
            prim->_texture->Bind(to_const_uint(ShaderProgram::TextureUsage::UNIT0));
        }
        // Inform the shader if we have (or don't have) a texture
        _imShader->Uniform("useTexture", texture);
        // Upload the primitive's world matrix to the shader
        _imShader->Uniform("dvd_WorldMatrix", prim->worldMatrix());
        // Submit the render call. We do not support instancing yet!
        prim->render(prim->forceWireframe(), 1);
        // Call any "postDraw" function the primitive may have attached
        prim->resetStates();
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
    // Create a world matrix using a look at function with the eye position
    // backed up from the camera's view direction
    mat4<F32> offset(-cam.getViewDir() * 2, VECTOR3_ZERO, cam.getUpDir());
    // Apply the inverse view matrix so that it cancels out in the shader
    // Submit the draw command, rendering it in a tiny viewport in the lower
    // right corner
    U16 windowWidth = _renderTarget[to_uint(RenderTarget::SCREEN)]->getWidth();

    IMPrimitive* primitive = getOrCreatePrimitive();
    primitive->name("GFXDeviceAxisGizmo");
    RenderStateBlock primitiveDescriptor(getRenderStateBlock(getDefaultStateBlock(true)));
    primitiveDescriptor.setLineWidth(3.0f);
    primitive->stateHash(primitiveDescriptor.getHash());
    drawLines(*primitive,
              _axisLines,
              offset * _gpuBlock._ViewMatrix.getInverse(),
              vec4<I32>(windowWidth - 120, 8, 128, 128),
              true);
}
};