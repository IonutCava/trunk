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

void GFXDevice::renderDebugViews() {
    static DebugView* HiZPtr;

    // As this is touched once per frame, we'll only enable it in debug builds
    if (Config::Build::IS_DEBUG_BUILD) {
        // Early out if we didn't request the preview
        if (!ParamHandler::instance().getParam<bool>(_ID("rendering.previewDebugViews"), false)) {
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

            DebugView_ptr HiZ = std::make_shared<DebugView>();
            HiZ->_shader = _previewDepthMapShader;
            HiZ->_texture = renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachment(RTAttachment::Type::Depth, 0).asTexture();
            HiZ->_shaderData._floatValues.push_back(std::make_pair("lodLevel", to_F32(HiZ->_texture->getMaxMipLevel() - 1)));

            DebugView_ptr DepthPreview = std::make_shared<DebugView>();
            DepthPreview->_shader = _previewDepthMapShader;
            DepthPreview->_texture = renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachment(RTAttachment::Type::Depth, 0).asTexture();
            DepthPreview->_shaderData._floatValues.push_back(std::make_pair("lodLevel", 0.0f));

            DebugView_ptr NormalPreview = std::make_shared<DebugView>();
            NormalPreview->_shader = _renderTargetDraw;
            NormalPreview->_texture = renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachment(RTAttachment::Type::Colour, to_const_U8(ScreenTargets::NORMALS)).asTexture();
            NormalPreview->_shaderData._boolValues.push_back(std::make_pair("linearSpace", false));
            NormalPreview->_shaderData._boolValues.push_back(std::make_pair("unpack2Channel", true));

            DebugView_ptr VelocityPreview = std::make_shared<DebugView>();
            VelocityPreview->_shader = _renderTargetDraw;
            VelocityPreview->_texture = renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachment(RTAttachment::Type::Colour, to_const_U8(ScreenTargets::VELOCITY)).asTexture();
            VelocityPreview->_shaderData._boolValues.push_back(std::make_pair("linearSpace", false));
            VelocityPreview->_shaderData._boolValues.push_back(std::make_pair("unpack2Channel", false));


            addDebugView(HiZ);
            addDebugView(DepthPreview);
            addDebugView(NormalPreview);
            addDebugView(VelocityPreview);

            HiZPtr = HiZ.get();
        }

        if (HiZPtr) {
            //HiZ preview
            I32 LoDLevel = 0;
            if (Config::USE_HIZ_CULLING && Config::USE_Z_PRE_PASS) {
                LoDLevel = to_I32(std::ceil(Time::ElapsedMilliseconds() / 750.0f)) %
                    (renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachment(RTAttachment::Type::Depth, 0).asTexture()->getMaxMipLevel() - 1);
            }
            HiZPtr->_shaderData._floatValues[0].second = to_F32(LoDLevel);
        }
    

        constexpr I32 maxViewportColumnCount = 10;
        I32 viewCount = to_I32(_debugViews.size());
        I32 columnCount = std::min(viewCount, maxViewportColumnCount);
        I32 rowCount = viewCount / maxViewportColumnCount;
        if (viewCount % maxViewportColumnCount > 0) {
            rowCount++;
        }

        RenderTarget& screenRT = renderTarget(RenderTargetID(RenderTargetUsage::SCREEN));
        U16 screenWidth = std::max(screenRT.getWidth(), to_const_U16(1280));
        U16 screenHeight = std::max(screenRT.getHeight(), to_const_U16(720));
        F32 aspectRatio = to_F32(screenWidth) / screenHeight;

        I32 viewportWidth = screenWidth / maxViewportColumnCount;
        I32 viewportHeight = to_I32(viewportWidth / aspectRatio);
        vec4<I32> viewport(screenWidth - viewportWidth, 0, viewportWidth, viewportHeight);

        GenericDrawCommand triangleCmd;
        triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
        triangleCmd.drawCount(1);
        triangleCmd.stateHash(_defaultStateNoDepthHash);

        I32 viewIndex = 0;
        for (U8 i = 0; i < rowCount; ++i) {
            for (U8 j = 0; j < columnCount; ++j) {
                DebugView& view = *_debugViews[viewIndex];
                triangleCmd.shaderProgram(view._shader);
                view._texture->bind(view._textureBindSlot);

                DebugView::ShaderData& shaderData = view._shaderData;
                for (const std::pair<stringImpl, I32>& data : shaderData._intValues) {
                    view._shader->Uniform(data.first.c_str(), data.second);
                }
                for (const std::pair<stringImpl, F32>& data : shaderData._floatValues) {
                    view._shader->Uniform(data.first.c_str(), data.second);
                }
                for (const std::pair<stringImpl, bool>& data : shaderData._boolValues) {
                    view._shader->Uniform(data.first.c_str(), data.second);
                }
                {
                    GFX::ScopedViewport sView(*this, viewport);
                    draw(triangleCmd);
                }

                viewport.x -= viewportWidth;
                viewIndex++;
            }
            viewport.y -= viewportHeight;
        }
    }
}


void GFXDevice::addDebugView(const std::shared_ptr<DebugView>& view) {
    _debugViews.push_back(view);
    std::sort(std::begin(_debugViews),
              std::end(_debugViews),
              [](const std::shared_ptr<DebugView>& a, const std::shared_ptr<DebugView>& b)-> bool {
                  return a->_shader->getGUID() < b->_shader->getGUID();
               });
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
        if (BitCompare(to_U32(sceneRenderState.gizmoState()),
                       to_const_U32(SceneRenderState::GizmoState::SCENE_GIZMO))) {

            // Apply the inverse view matrix so that it cancels out in the shader
            // Submit the draw command, rendering it in a tiny viewport in the lower
            // right corner
            U16 windowWidth = renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getWidth();
            _axisGizmo->fromLines(_axisLines, vec4<I32>(windowWidth - 120, 8, 128, 128));
        
            // We need to transform the gizmo so that it always remains axis aligned
            // Create a world matrix using a look at function with the eye position
            // backed up from the camera's view direction
            _axisGizmo->worldMatrix(mat4<F32>(-activeCamera.getForwardDir() * 2,
                                               VECTOR3_ZERO,
                                               activeCamera.getUpDir()) * getMatrix(MATRIX::VIEW_INV));
            _axisGizmo->paused(false);
        
            subPassesInOut.back()._commands.push_back(_axisGizmo->toDrawCommand());
        } else {
            _axisGizmo->paused(true);
        }
    }
}

};