#include "Headers/GFXDevice.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Scenes/Headers/SceneState.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace Divide {
/// Show the contents of the depth buffer in a small rectangle in the bottom
/// right of the screen
void GFXDevice::previewDepthBuffer() {
// As this is touched once per frame, we'll only enable it in debug builds
    if (Config::Build::IS_DEBUG_BUILD) {
        // Early out if we didn't request the preview
        if (!ParamHandler::instance().getParam<bool>(
            _ID("rendering.previewDepthBuffer"), false)) {
            return;
        }
        // Lazy-load preview shader
        if (!_previewDepthMapShader) {
            // The LinearDepth variant converts the depth values to linear values
            // between the 2 scene z-planes
            ResourceDescriptor fbPreview("fbPreview.LinearDepth.ScenePlanes");
            fbPreview.setPropertyList("USE_SCENE_ZPLANES");
            _previewDepthMapShader = CreateResource<ShaderProgram>(parent().resourceCache(), fbPreview);
            assert(_previewDepthMapShader != nullptr);
        }

        if (_previewDepthMapShader->getState() != ResourceState::RES_LOADED) {
            return;
        }

        GenericDrawCommand triangleCmd;
        triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
        triangleCmd.drawCount(1);
        triangleCmd.stateHash(_defaultStateNoDepthHash);

        RenderTarget& screenRT = renderTarget(RenderTargetID(RenderTargetUsage::SCREEN));
        U16 screenWidth = std::max(screenRT.getWidth(), to_const_ushort(768));
        screenRT.bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Depth, 0);

        triangleCmd.shaderProgram(_previewDepthMapShader);
        {
            //HiZ preview
            I32 LoDLevel = 0;
            if (Config::USE_HIZ_CULLING && Config::USE_Z_PRE_PASS) {
                LoDLevel = to_int(std::ceil(Time::ElapsedMilliseconds() / 750.0f)) %
                           (screenRT.getAttachment(RTAttachment::Type::Depth, 0).asTexture()->getMaxMipLevel() - 1);
            }

            _previewDepthMapShader->Uniform("lodLevel", to_float(LoDLevel));
            GFX::ScopedViewport viewport(*this, screenWidth - 256, 0, 256, 256);
            draw(triangleCmd);
        }
        {
            //Depth preview
            _previewDepthMapShader->Uniform("lodLevel", 0.0f);
            GFX::ScopedViewport viewport(*this, screenWidth - 512, 0, 256, 256);
            draw(triangleCmd);
        }

        triangleCmd.shaderProgram(_renderTargetDraw);
        {        
            //Normals preview
            screenRT.bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Colour, to_const_ubyte(ScreenTargets::NORMALS));

            GFX::ScopedViewport viewport(*this, screenWidth - 768, 0, 256, 256);
            _renderTargetDraw->Uniform("linearSpace", false);
            _renderTargetDraw->Uniform("unpack2Channel", true);
            draw(triangleCmd);
        }
        {
            //Velocity preview
            screenRT.bind(to_const_ubyte(ShaderProgram::TextureUsage::UNIT0), RTAttachment::Type::Colour, to_const_ubyte(ScreenTargets::VELOCITY));

            GFX::ScopedViewport viewport(*this, screenWidth - 1024, 0, 256, 256);
            _renderTargetDraw->Uniform("linearSpace", false);
            _renderTargetDraw->Uniform("unpack2Channel", false);
            draw(triangleCmd);
        }
    }
}

void GFXDevice::drawDebugFrustum(RenderSubPassCmds& subPassesInOut) {
    if (_debugFrustum != nullptr) {
        static const vec4<U8> redColour(Util::ToByteColour(DefaultColours::RED()));
        static const vec4<U8> greenColour(Util::ToByteColour(DefaultColours::GREEN()));

        vectorImpl<vec3<F32>> corners;
        _debugFrustum->getCornersViewSpace(corners);

        vectorImpl<Line> lines;
        for (U8 i = 0; i < 4; ++i) {
            // Draw Near Plane
            lines.emplace_back(corners[i], corners[(i + 1) % 4], redColour);
            // Draw Far Plane
            lines.emplace_back(corners[i + 4], corners[(i + 4 + 1) % 4], redColour);
            // Connect Near Plane with Far Plane
            lines.emplace_back(corners[i], corners[(i + 4) % 8], greenColour);
        }

        _debugFrustumPrimitive->fromLines(lines);
        subPassesInOut.back()._commands.push_back(_debugFrustumPrimitive->toDrawCommand());
    }
}

/// Render all of our immediate mode primitives. This isn't very optimised and
/// most are recreated per frame!
void GFXDevice::debugDraw(const SceneRenderState& sceneRenderState, const Camera& activeCamera, RenderSubPassCmds& subPassesInOut) {
    if (Config::Build::IS_DEBUG_BUILD) {
        drawDebugFrustum(subPassesInOut);

        // Debug axis form the axis arrow gizmo in the corner of the screen
        // This is togglable, so check if it's actually requested
        if (BitCompare(to_uint(sceneRenderState.gizmoState()),
                       to_const_uint(SceneRenderState::GizmoState::SCENE_GIZMO))) {

            // Apply the inverse view matrix so that it cancels out in the shader
            // Submit the draw command, rendering it in a tiny viewport in the lower
            // right corner
            U16 windowWidth = renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getWidth();
            _axisGizmo->fromLines(_axisLines, vec4<I32>(windowWidth - 120, 8, 128, 128));
        
            // We need to transform the gizmo so that it always remains axis aligned
            // Create a world matrix using a look at function with the eye position
            // backed up from the camera's view direction
            _axisGizmo->worldMatrix(
                    mat4<F32>(-activeCamera.getForwardDir() * 2, VECTOR3_ZERO, activeCamera.getUpDir()) *
                    getMatrix(MATRIX::VIEW_INV));
            _axisGizmo->paused(false);
        
            subPassesInOut.back()._commands.push_back(_axisGizmo->toDrawCommand());
        } else {
            _axisGizmo->paused(true);
        }
    }
}

};