#include "stdafx.h"

#include "Headers/PostFXWindow.h"
#include "Editor/Headers/Editor.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Headers/StringHelper.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Rendering/Headers/Renderer.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/PostFX/CustomOperators/Headers/SSAOPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/BloomPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/DoFPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/PostAAPreRenderOperator.h"

namespace Divide {
    PostFXWindow::PostFXWindow(Editor& parent, PlatformContext& context, const Descriptor& descriptor)
        : PlatformContextComponent(context),
          DockedWindow(parent, descriptor),
          _postFX(context.gfx().getRenderer().postFX())
    {
    }

    PostFXWindow::~PostFXWindow()
    {

    }

    void PostFXWindow::drawInternal() {
        PreRenderBatch* batch = _postFX.getFilterBatch();

        auto checkBox = [this](FilterType filter) {
            bool filterEnabled = _postFX.getFilterState(filter);
            if (ImGui::Checkbox("Enabled", &filterEnabled)) {
                if (filterEnabled) {
                    _postFX.pushFilter(filter);
                } else {
                    _postFX.popFilter(filter);
                }
            }
        };

        if (ImGui::CollapsingHeader("SS Antialiasing")) {
            checkBox(FilterType::FILTER_SS_ANTIALIASING);
            PreRenderOperator& op = batch->getOperator(FilterType::FILTER_SS_ANTIALIASING);
            PostAAPreRenderOperator& aaOp = static_cast<PostAAPreRenderOperator&>(op);
            I32 samples = to_I32(aaOp.aaSamples());
            bool usesSMAA = aaOp.usesSMAA();
            if (ImGui::SliderInt("Sample Count", &samples, 0, 16)) {
                aaOp.setAASamples(to_U8(samples));
            }
            if (ImGui::Checkbox("Use SMAA", &usesSMAA)) {
                aaOp.useSMAA(usesSMAA);
            }
        }
        if (ImGui::CollapsingHeader("SS Reflections")) {
            checkBox(FilterType::FILTER_SS_REFLECTIONS);
            PreRenderOperator& op = batch->getOperator(FilterType::FILTER_SS_REFLECTIONS);
            ACKNOWLEDGE_UNUSED(op);
        }
        if (ImGui::CollapsingHeader("SS Ambient Occlusion")) {
            checkBox(FilterType::FILTER_SS_AMBIENT_OCCLUSION);
            PreRenderOperator& op = batch->getOperator(FilterType::FILTER_SS_AMBIENT_OCCLUSION);
            SSAOPreRenderOperator& ssaoOp = static_cast<SSAOPreRenderOperator&>(op);
            F32 radius = ssaoOp.radius();
            F32 power = ssaoOp.power();
            if (ImGui::SliderFloat("Radius", &radius, 0.01f, 10.0f)) {
                ssaoOp.radius(radius);
            }
            if (ImGui::SliderFloat("Power", &power, 1.0f, 5.0f)) {
                ssaoOp.power(power);
            }
        }
        if (ImGui::CollapsingHeader("Depth of Field")) {
            checkBox(FilterType::FILTER_DEPTH_OF_FIELD);
            PreRenderOperator& op = batch->getOperator(FilterType::FILTER_DEPTH_OF_FIELD);
            DoFPreRenderOperator& dofOp = static_cast<DoFPreRenderOperator&>(op);
            F32 focalDepth = dofOp.focalDepth();
            bool autoFocus = dofOp.autoFocus();
            if (ImGui::SliderFloat("Focal Depth", &focalDepth, 0.0f, 1.0f)) {
                dofOp.focalDepth(focalDepth);
            }
            if (ImGui::Checkbox("Auto Focus", &autoFocus)) {
                dofOp.autoFocus(autoFocus);
            }
        }
        if (ImGui::CollapsingHeader("Motion Blur")) {
            checkBox(FilterType::FILTER_MOTION_BLUR);
            PreRenderOperator& op = batch->getOperator(FilterType::FILTER_MOTION_BLUR);
            ACKNOWLEDGE_UNUSED(op);
        }
        if (ImGui::CollapsingHeader("Bloom")) {
            checkBox(FilterType::FILTER_BLOOM);
            PreRenderOperator& op = batch->getOperator(FilterType::FILTER_BLOOM);
            BloomPreRenderOperator& bloomOp = static_cast<BloomPreRenderOperator&>(op);
            F32 factor = bloomOp.factor();
            F32 threshold = bloomOp.luminanceThreshold();
            if (ImGui::SliderFloat("Factor", &factor, 0.01f, 3.0f)) {
                bloomOp.factor(factor);
            }
            if (ImGui::SliderFloat("Luminance Threshold", &threshold, 0.0f, 1.0f)) {
                bloomOp.luminanceThreshold(threshold);
            }
        }
        if (ImGui::CollapsingHeader("Tone Mapping")) {
            checkBox(FilterType::FILTER_LUT_CORECTION);
            PreRenderOperator& op = batch->getOperator(FilterType::FILTER_LUT_CORECTION);
            ACKNOWLEDGE_UNUSED(op);
        }
        if (ImGui::CollapsingHeader("Noise")) {
            checkBox(FilterType::FILTER_NOISE);
            PreRenderOperator& op = batch->getOperator(FilterType::FILTER_NOISE);
            ACKNOWLEDGE_UNUSED(op);
        }
        if (ImGui::CollapsingHeader("Vignette")) {
            checkBox(FilterType::FILTER_VIGNETTE);
            PreRenderOperator& op = batch->getOperator(FilterType::FILTER_VIGNETTE);
            ACKNOWLEDGE_UNUSED(op);
        }
        if (ImGui::CollapsingHeader("Underwater")) {
            checkBox(FilterType::FILTER_UNDERWATER);
            PreRenderOperator& op = batch->getOperator(FilterType::FILTER_UNDERWATER);
            ACKNOWLEDGE_UNUSED(op);
        }
    }
};