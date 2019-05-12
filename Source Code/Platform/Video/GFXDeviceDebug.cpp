#include "stdafx.h"

#include "Headers/GFXDevice.h"

#include "GUI\Headers\GUI.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Headers/Configuration.h"
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

void GFXDevice::renderDebugViews(const Rect<I32>& targetViewport, GFX::CommandBuffer& bufferInOut) {
    static DebugView* HiZPtr = nullptr;
    static size_t labelStyleHash = TextLabelStyle(Font::DROID_SERIF_BOLD, UColour(128), 96).getHash();

    // As this is touched once per frame, we'll only enable it in debug builds
    if (Config::ENABLE_GPU_VALIDATION) {
        // Early out if we didn't request the preview
        if (!ParamHandler::instance().getParam<bool>(_ID("rendering.previewDebugViews"), false)) {
            return;
        }

        // Lazy-load preview shader
        if (!_previewDepthMapShader) {
            // The LinearDepth variant converts the depth values to linear values between the 2 scene z-planes
            ResourceDescriptor fbPreview("fbPreview.LinearDepth");
            _previewDepthMapShader = CreateResource<ShaderProgram>(parent().resourceCache(), fbPreview);
            assert(_previewDepthMapShader != nullptr);

            DebugView_ptr HiZ = std::make_shared<DebugView>();
            HiZ->_shader = _previewDepthMapShader;
            HiZ->_texture = renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::HI_Z)).getAttachment(RTAttachmentType::Depth, 0).texture();
            HiZ->_name = "Hierarchical-Z";
            HiZ->_shaderData.set("lodLevel", GFX::PushConstantType::FLOAT, to_F32(HiZ->_texture->getMaxMipLevel() - 1));
            HiZ->_shaderData.set("zPlanes", GFX::PushConstantType::VEC2, vec2<F32>(_context.config().runtime.zNear, _context.config().runtime.zFar));

            DebugView_ptr DepthPreview = std::make_shared<DebugView>();
            DepthPreview->_shader = _previewDepthMapShader;
            DepthPreview->_texture = renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachment(RTAttachmentType::Depth, 0).texture();
            DepthPreview->_name = "Depth Buffer";
            DepthPreview->_shaderData.set("lodLevel", GFX::PushConstantType::FLOAT, 0.0f);
            DepthPreview->_shaderData.set("zPlanes", GFX::PushConstantType::VEC2, vec2<F32>(_context.config().runtime.zNear, _context.config().runtime.zFar));

            DebugView_ptr NormalPreview = std::make_shared<DebugView>();
            NormalPreview->_shader = _renderTargetDraw;
            NormalPreview->_texture = renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachment(RTAttachmentType::Colour, to_U8(ScreenTargets::NORMALS_AND_VELOCITY)).texture();
            NormalPreview->_name = "Normals";
            NormalPreview->_shaderData.set("lodLevel", GFX::PushConstantType::FLOAT, 0.0f);
            NormalPreview->_shaderData.set("unpack1Channel", GFX::PushConstantType::UINT, 0u);
            NormalPreview->_shaderData.set("unpack2Channel", GFX::PushConstantType::UINT, 1u);
            NormalPreview->_shaderData.set("startOnBlue", GFX::PushConstantType::UINT, 0u);

            DebugView_ptr VelocityPreview = std::make_shared<DebugView>();
            VelocityPreview->_shader = _renderTargetDraw;
            VelocityPreview->_texture = renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getAttachment(RTAttachmentType::Colour, to_U8(ScreenTargets::NORMALS_AND_VELOCITY)).texture();
            VelocityPreview->_name = "Velocity Map";
            VelocityPreview->_shaderData.set("lodLevel", GFX::PushConstantType::FLOAT, 0.0f);
            VelocityPreview->_shaderData.set("unpack1Channel", GFX::PushConstantType::UINT, 0u);
            VelocityPreview->_shaderData.set("unpack2Channel", GFX::PushConstantType::UINT, 0u);
            VelocityPreview->_shaderData.set("startOnBlue", GFX::PushConstantType::UINT, 1u);
            

            DebugView_ptr AlphaAccumulationHigh = std::make_shared<DebugView>();
            AlphaAccumulationHigh->_shader = _renderTargetDraw;
            AlphaAccumulationHigh->_texture = renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::OIT)).getAttachment(RTAttachmentType::Colour, to_U8(ScreenTargets::ALBEDO)).texture();
            AlphaAccumulationHigh->_name = "Alpha Accumulation High";
            AlphaAccumulationHigh->_shaderData.set("lodLevel", GFX::PushConstantType::FLOAT, 0.0f);
            AlphaAccumulationHigh->_shaderData.set("unpack2Channel", GFX::PushConstantType::UINT, 0u);
            AlphaAccumulationHigh->_shaderData.set("unpack1Channel", GFX::PushConstantType::UINT, 0u);
            AlphaAccumulationHigh->_shaderData.set("startOnBlue", GFX::PushConstantType::UINT, 0u);

            DebugView_ptr AlphaRevealageHigh = std::make_shared<DebugView>();
            AlphaRevealageHigh->_shader = _renderTargetDraw;
            AlphaRevealageHigh->_texture = renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::OIT)).getAttachment(RTAttachmentType::Colour, to_U8(ScreenTargets::REVEALAGE)).texture();
            AlphaRevealageHigh->_name = "Alpha Revealage High";
            AlphaRevealageHigh->_shaderData.set("lodLevel", GFX::PushConstantType::FLOAT, 0.0f);
            AlphaRevealageHigh->_shaderData.set("unpack1Channel", GFX::PushConstantType::UINT, 1u);
            AlphaRevealageHigh->_shaderData.set("unpack2Channel", GFX::PushConstantType::UINT, 0u);
            AlphaRevealageHigh->_shaderData.set("startOnBlue", GFX::PushConstantType::UINT, 0u);

            //DebugView_ptr AlphaAccumulationLow = std::make_shared<DebugView>();
            //AlphaAccumulationLow->_shader = _renderTargetDraw;
            //AlphaAccumulationLow->_texture = renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::OIT_QUARTER_RES)).getAttachment(RTAttachmentType::Colour, to_U8(ScreenTargets::ALBEDO)).texture();
            //AlphaAccumulationLow->_name = "Alpha Accumulation Low";
            //AlphaAccumulationLow->_shaderData.set("lodLevel", GFX::PushConstantType::FLOAT, 0.0f);
            //AlphaAccumulationLow->_shaderData.set("unpack2Channel", GFX::PushConstantType::UINT, 0u);
            //AlphaAccumulationLow->_shaderData.set("unpack1Channel", GFX::PushConstantType::UINT, 0u);
            //AlphaAccumulationLow->_shaderData.set("startOnBlue", GFX::PushConstantType::UINT, 0u);

            //DebugView_ptr AlphaRevealageLow = std::make_shared<DebugView>();
            //AlphaRevealageLow->_shader = _renderTargetDraw;
            //AlphaRevealageLow->_texture = renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::OIT_QUARTER_RES)).getAttachment(RTAttachmentType::Colour, to_U8(ScreenTargets::REVEALAGE)).texture();
            //AlphaRevealageLow->_name = "Alpha Revealage Low";
            //AlphaRevealageLow->_shaderData.set("lodLevel", GFX::PushConstantType::FLOAT, 0.0f);
            //AlphaRevealageLow->_shaderData.set("unpack1Channel", GFX::PushConstantType::UINT, 1u);
            //AlphaRevealageLow->_shaderData.set("unpack2Channel", GFX::PushConstantType::UINT, 0u);
            //AlphaRevealageLow->_shaderData.set("startOnBlue", GFX::PushConstantType::UINT, 0u);

            HiZPtr = addDebugView(HiZ);
            addDebugView(DepthPreview);
            addDebugView(NormalPreview);
            addDebugView(VelocityPreview);
            addDebugView(AlphaAccumulationHigh);
            addDebugView(AlphaRevealageHigh);
            //addDebugView(AlphaAccumulationLow);
            //addDebugView(AlphaRevealageLow);

            WAIT_FOR_CONDITION(_previewDepthMapShader->getState() == ResourceState::RES_LOADED);
        }

        if (HiZPtr) {
            //HiZ preview
            I32 LoDLevel = 0;
            RenderTarget& HiZRT = renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::HI_Z));
            LoDLevel = to_I32(std::ceil(Time::ElapsedMilliseconds() / 750.0f)) % (HiZRT.getAttachment(RTAttachmentType::Depth, 0).texture()->getMaxMipLevel() - 1);
            HiZPtr->_shaderData.set("lodLevel", GFX::PushConstantType::FLOAT, to_F32(LoDLevel));
        }

        constexpr I32 maxViewportColumnCount = 10;
        I32 viewCount = to_I32(_debugViews.size());
        for (auto view : _debugViews) {
            if (!view->_enabled) {
                --viewCount;
            }
        }

        I32 columnCount = std::min(viewCount, maxViewportColumnCount);
        I32 rowCount = viewCount / maxViewportColumnCount;
        if (viewCount % maxViewportColumnCount > 0) {
            rowCount++;
        }

        I32 screenWidth = targetViewport.z;
        I32 screenHeight = targetViewport.w;
        F32 aspectRatio = to_F32(screenWidth) / screenHeight;

        I32 viewportWidth = (screenWidth / columnCount) - targetViewport.x;
        I32 viewportHeight = to_I32(viewportWidth / aspectRatio) - targetViewport.y;
        Rect<I32> viewport(screenWidth - viewportWidth, targetViewport.y, viewportWidth, viewportHeight);

        PipelineDescriptor pipelineDesc = {};
        pipelineDesc._stateHash = _state2DRenderingHash;

        GenericDrawCommand triangleCmd = {};
        triangleCmd._primitiveType = PrimitiveType::TRIANGLES;
        triangleCmd._drawCount = 1;

        vectorFast <std::pair<stringImpl, Rect<I32>>> labelStack;

        Rect<I32> crtViewport = getCurrentViewport();
        GFX::SetViewportCommand setViewport = {};
        GFX::SendPushConstantsCommand pushConstants = {};
        GFX::BindPipelineCommand bindPipeline = {};
        GFX::DrawCommand drawCommand = {};
        drawCommand._drawCommands.push_back(triangleCmd);

        for (I16 idx = 0; idx < to_I16(_debugViews.size()); ++idx) {
            DebugView& view = *_debugViews[idx];

            if (!view._enabled) {
                continue;
            }

            pipelineDesc._shaderProgramHandle = view._shader->getGUID();

            bindPipeline._pipeline = newPipeline(pipelineDesc);
            GFX::EnqueueCommand(bufferInOut, bindPipeline);

            pushConstants._constants = view._shaderData;
            GFX::EnqueueCommand(bufferInOut, pushConstants);

            setViewport._viewport.set(viewport);
            GFX::EnqueueCommand(bufferInOut, setViewport);

            GFX::BindDescriptorSetsCommand bindDescriptorSets = {};
            bindDescriptorSets._set._textureData.setTexture(view._texture->getData(), view._textureBindSlot);
            GFX::EnqueueCommand(bufferInOut, bindDescriptorSets);

            GFX::EnqueueCommand(bufferInOut, drawCommand);

            if (!view._name.empty()) {
                labelStack.emplace_back(view._name, viewport);
            }

            if (idx > 0 && idx % (columnCount - 1) == 0) {
                viewport.y += viewportHeight + targetViewport.y;
                viewport.x += viewportWidth * columnCount + targetViewport.x * columnCount;
            }
             
            viewport.x -= viewportWidth + targetViewport.x;
        }

        TextElement text(labelStyleHash, RelativePosition2D(RelativeValue(0.1f, 0.0f), RelativeValue(0.1f, 0.0f)));
        for (const std::pair<stringImpl, Rect<I32>>& entry : labelStack) {
            // Draw labels at the end to reduce number of state changes
            setViewport._viewport.set(entry.second);
            GFX::EnqueueCommand(bufferInOut, setViewport);

            text._position.d_y.d_offset = entry.second.sizeY - 10.0f;
            text.text(entry.first);
            drawText(text, bufferInOut);
        }
    }
}


DebugView* GFXDevice::addDebugView(const std::shared_ptr<DebugView>& view) {
    UniqueLock lock(_debugViewLock);

    _debugViews.push_back(view);
    if (_debugViews.back()->_sortIndex == -1) {
        _debugViews.back()->_sortIndex = to_I16(_debugViews.size());
    }
    std::sort(std::begin(_debugViews),
              std::end(_debugViews),
              [](const std::shared_ptr<DebugView>& a, const std::shared_ptr<DebugView>& b)-> bool {
                  return a->_sortIndex < b->_sortIndex;
               });

    return view.get();
}

bool GFXDevice::removeDebugView(DebugView* view) {
    if (view != nullptr) {
        vector<std::shared_ptr<DebugView>>::iterator it;
        it = std::find_if(std::begin(_debugViews),
                          std::end(_debugViews),
                         [view](const std::shared_ptr<DebugView>& entry) {
                            return view->getGUID() == entry->getGUID();
                         });

        if (it != std::cend(_debugViews)) {
            _debugViews.erase(it);
            return true;
        }
    }

    return false;
}

void GFXDevice::drawDebugFrustum(const mat4<F32>& viewMatrix, GFX::CommandBuffer& bufferInOut) {
    if (_debugFrustum != nullptr) {
        std::array<vec3<F32>, 8> corners;
        _debugFrustum->getCornersViewSpace(viewMatrix, corners);

        vector<Line> lines;
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
        drawDebugFrustum(activeCamera.getViewMatrix(), bufferInOut);

        // Debug axis form the axis arrow gizmo in the corner of the screen
        // This is toggleable, so check if it's actually requested
        if (BitCompare(to_U32(sceneRenderState.gizmoState()), to_base(SceneRenderState::GizmoState::SCENE_GIZMO))) {

            // Apply the inverse view matrix so that it cancels out in the shader
            // Submit the draw command, rendering it in a tiny viewport in the lower
            // right corner
            U16 windowWidth = renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN)).getWidth();
            _axisGizmo->fromLines(_axisLines, Rect<I32>(windowWidth - 120, 8, 128, 128));
        
            // We need to transform the gizmo so that it always remains axis aligned
            // Create a world matrix using a look at function with the eye position
            // backed up from the camera's view direction
            _axisGizmo->worldMatrix(mat4<F32>(-activeCamera.getForwardDir() * 2,
                                               VECTOR3_ZERO,
                                               activeCamera.getUpDir()) * activeCamera.getWorldMatrix());
            bufferInOut.add(_axisGizmo->toCommandBuffer());
        }
    }
}

};