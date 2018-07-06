#include "stdafx.h"

#include "Headers/GFXDevice.h"

#include "GUI\Headers\GUI.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Scenes/Headers/SceneState.h"
#include "Utility/Headers/TextLabel.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace Divide {

void GFXDevice::renderDebugViews(GFX::CommandBuffer& bufferInOut) {
    static DebugView* HiZPtr;
    static size_t labelStyleHash = TextLabelStyle(Font::DROID_SERIF_BOLD, vec4<U8>(255), 96).getHash();

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
            HiZ->_texture = renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachment(RTAttachmentType::Depth, 0).texture();
            HiZ->_name = "Hierarchical-Z";
            HiZ->_shaderData.set("lodLevel", PushConstantType::FLOAT, to_F32(HiZ->_texture->getMaxMipLevel() - 1));

            DebugView_ptr DepthPreview = std::make_shared<DebugView>();
            DepthPreview->_shader = _previewDepthMapShader;
            DepthPreview->_texture = renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachment(RTAttachmentType::Depth, 0).texture();
            DepthPreview->_shaderData.set("Depth Buffer", PushConstantType::FLOAT, 0.0f);

            DebugView_ptr NormalPreview = std::make_shared<DebugView>();
            NormalPreview->_shader = _renderTargetDraw;
            NormalPreview->_texture = renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachment(RTAttachmentType::Colour, to_U8(ScreenTargets::NORMALS)).texture();
            NormalPreview->_name = "Normals";
            NormalPreview->_shaderData.set("linearSpace", PushConstantType::UINT, 0u);
            NormalPreview->_shaderData.set("unpack2Channel", PushConstantType::UINT, 1u);

            DebugView_ptr VelocityPreview = std::make_shared<DebugView>();
            VelocityPreview->_shader = _renderTargetDraw;
            VelocityPreview->_texture = renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachment(RTAttachmentType::Colour, to_U8(ScreenTargets::VELOCITY)).texture();
            VelocityPreview->_name = "Velocity Map";
            VelocityPreview->_shaderData.set("linearSpace", PushConstantType::UINT, 0u);
            VelocityPreview->_shaderData.set("unpack2Channel", PushConstantType::UINT, 0u);

            DebugView_ptr AlphaAccumulation = std::make_shared<DebugView>();
            AlphaAccumulation->_shader = _renderTargetDraw;
            AlphaAccumulation->_texture = renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::OIT)).getAttachment(RTAttachmentType::Colour, to_U8(ScreenTargets::ALBEDO)).texture();
            AlphaAccumulation->_name = "Alpha Accumulation";
            AlphaAccumulation->_shaderData.set("linearSpace", PushConstantType::UINT, 1u);
            AlphaAccumulation->_shaderData.set("unpack2Channel", PushConstantType::UINT, 0u);

            addDebugView(HiZ);
            addDebugView(DepthPreview);
            addDebugView(NormalPreview);
            addDebugView(VelocityPreview);
            addDebugView(AlphaAccumulation);
            HiZPtr = HiZ.get();
        }

        RenderTarget& screenRT = renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN));

        if (HiZPtr) {
            //HiZ preview
            I32 LoDLevel = 0;
            if (Config::USE_HIZ_CULLING && Config::USE_Z_PRE_PASS) {
                LoDLevel = to_I32(std::ceil(Time::ElapsedMilliseconds() / 750.0f)) %
                    (screenRT.getAttachment(RTAttachmentType::Depth, 0).texture()->getMaxMipLevel() - 1);
            }
            HiZPtr->_shaderData.set("lodLevel", PushConstantType::FLOAT, to_F32(LoDLevel));
        }

        constexpr I32 maxViewportColumnCount = 10;
        I32 viewCount = to_I32(_debugViews.size());
        I32 columnCount = std::min(viewCount, maxViewportColumnCount);
        I32 rowCount = viewCount / maxViewportColumnCount;
        if (viewCount % maxViewportColumnCount > 0) {
            rowCount++;
        }

        U16 screenWidth = std::max(screenRT.getWidth(), to_U16(1280));
        U16 screenHeight = std::max(screenRT.getHeight(), to_U16(720));
        F32 aspectRatio = to_F32(screenWidth) / screenHeight;

        I32 viewportWidth = screenWidth / maxViewportColumnCount;
        I32 viewportHeight = to_I32(viewportWidth / aspectRatio);
        vec4<I32> viewport(screenWidth - viewportWidth, 0, viewportWidth, viewportHeight);

        PipelineDescriptor pipelineDesc;
        pipelineDesc._stateHash = _defaultStateBlockHash;

        GenericDrawCommand triangleCmd;
        triangleCmd.primitiveType(PrimitiveType::TRIANGLES);
        triangleCmd.drawCount(1);

        vectorImplFast <std::pair<stringImpl, vec4<I32>>> labelStack;

        vec4<I32> crtViewport = getCurrentViewport();
        GFX::SetViewportCommand setViewport;
        GFX::SendPushConstantsCommand pushConstants;
        GFX::BindPipelineCommand bindPipeline;
        GFX::BindDescriptorSetsCommand bindDescriptorSets;
        GFX::DrawCommand drawCommand;
        drawCommand._drawCommands.push_back(triangleCmd);

        I32 viewIndex = 0;
        for (U8 i = 0; i < rowCount; ++i) {
            for (U8 j = 0; j < columnCount; ++j) {
                DebugView& view = *_debugViews[viewIndex];
                pipelineDesc._shaderProgram = view._shader;

                bindPipeline._pipeline = &newPipeline(pipelineDesc);
                GFX::BindPipeline(bufferInOut, bindPipeline);

                pushConstants._constants = view._shaderData;
                GFX::SendPushConstants(bufferInOut, pushConstants);

                setViewport._viewport.set(viewport);
                GFX::SetViewPort(bufferInOut, setViewport);

                bindDescriptorSets._set._textureData.clear();
                bindDescriptorSets._set._textureData.addTexture(view._texture->getData(),
                                                                view._textureBindSlot);
                GFX::BindDescriptorSets(bufferInOut, bindDescriptorSets);

                GFX::AddDrawCommands(bufferInOut, drawCommand);

                if (!view._name.empty()) {
                    labelStack.emplace_back(view._name, viewport);
                }

                viewport.x -= viewportWidth;
                if (viewIndex++ == viewCount) {
                    return;
                }
            }
            viewport.y += viewportHeight;
        }

        TextElement text(labelStyleHash, vec2<F32>(10.0f));
        for (const std::pair<stringImpl, vec4<I32>>& entry : labelStack) {
            // Draw labels at the end to reduce number of state changes
            setViewport._viewport.set(entry.second);
            GFX::SetViewPort(bufferInOut, setViewport);

            text._position.y = entry.second.sizeY - 10.0f;
            text.text(entry.first);
            drawText(text, bufferInOut);
        }
    }
}


void GFXDevice::addDebugView(const std::shared_ptr<DebugView>& view) {
    _debugViews.push_back(view);
    if (_debugViews.back()->_sortIndex == -1) {
        _debugViews.back()->_sortIndex = to_I16(_debugViews.size());
    }
    std::sort(std::begin(_debugViews),
              std::end(_debugViews),
              [](const std::shared_ptr<DebugView>& a, const std::shared_ptr<DebugView>& b)-> bool {
                  return a->_sortIndex < b->_sortIndex;
               });
}

void GFXDevice::drawDebugFrustum(GFX::CommandBuffer& bufferInOut) {
    if (_debugFrustum != nullptr) {
        vectorImpl<vec3<F32>> corners;
        _debugFrustum->getCornersViewSpace(corners);

        vectorImpl<Line> lines;
        for (U8 i = 0; i < 4; ++i) {
            // Draw Near Plane
            lines.emplace_back(corners[i], corners[(i + 1) % 4], DefaultColours::RED_U8);
            // Draw Far Plane
            lines.emplace_back(corners[i + 4], corners[(i + 4 + 1) % 4], DefaultColours::RED_U8);
            // Connect Near Plane with Far Plane
            lines.emplace_back(corners[i], corners[(i + 4) % 8], DefaultColours::GREEN_U8);
        }

        _debugFrustumPrimitive->fromLines(lines);
        bufferInOut.add(_debugFrustumPrimitive->toCommandBuffer());
    }
}

/// Render all of our immediate mode primitives. This isn't very optimised and
/// most are recreated per frame!
void GFXDevice::debugDraw(const SceneRenderState& sceneRenderState, const Camera& activeCamera, GFX::CommandBuffer& bufferInOut) {
    if (Config::Build::IS_DEBUG_BUILD) {
        drawDebugFrustum(bufferInOut);

        // Debug axis form the axis arrow gizmo in the corner of the screen
        // This is togglable, so check if it's actually requested
        if (BitCompare(to_U32(sceneRenderState.gizmoState()),
                       to_base(SceneRenderState::GizmoState::SCENE_GIZMO))) {

            // Apply the inverse view matrix so that it cancels out in the shader
            // Submit the draw command, rendering it in a tiny viewport in the lower
            // right corner
            U16 windowWidth = renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getWidth();
            _axisGizmo->fromLines(_axisLines, vec4<I32>(windowWidth - 120, 8, 128, 128));
        
            // We need to transform the gizmo so that it always remains axis aligned
            // Create a world matrix using a look at function with the eye position
            // backed up from the camera's view direction
            _axisGizmo->worldMatrix(mat4<F32>(-activeCamera.getForwardDir() * 2,
                                               VECTOR3_ZERO,
                                               activeCamera.getUpDir()) * getMatrix(MATRIX::VIEW_INV));
            bufferInOut.add(_axisGizmo->toCommandBuffer());
        }
    }
}

};