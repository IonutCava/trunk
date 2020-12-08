#include "stdafx.h"

#include "Headers/PostFXWindow.h"

#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Editor/Headers/Editor.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Rendering/Headers/Renderer.h"
#include "Rendering/PostFX/CustomOperators/Headers/BloomPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/DoFPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/MotionBlurPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/PostAAPreRenderOperator.h"
#include "Rendering/PostFX/CustomOperators/Headers/SSAOPreRenderOperator.h"
#include "Rendering/PostFX/Headers/PostFX.h"

namespace Divide {
namespace {
    bool PreviewTextureButton(I32 &id, Texture* tex, const bool readOnly) {
        bool ret = false;
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 15);
        ImGui::PushID(4321234 + id++);
        if (readOnly) {
            PushReadOnly();
        }
        if (ImGui::SmallButton("T")) {
            ret = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(Util::StringFormat("Preview texture : %s", tex->assetName().c_str()).c_str());
        }
        if (readOnly) {
            PopReadOnly();
        }
        ImGui::PopID();
        return ret;
    }

    constexpr I32 g_exposureRefreshFrameCount = 3;

}

    PostFXWindow::PostFXWindow(Editor& parent, PlatformContext& context, const Descriptor& descriptor)
        : DockedWindow(parent, descriptor),
          PlatformContextComponent(context),
          _postFX(context.gfx().getRenderer().postFX())
    {
    }

    void PostFXWindow::drawInternal() {
        PreRenderBatch& batch = _postFX.getFilterBatch();

        const auto checkBox = [this](const FilterType filter, const char* label = "Enabled", const bool overrideScene = false) {
            bool filterEnabled = _postFX.getFilterState(filter);
            ImGui::PushID(to_base(filter));
            if (ImGui::Checkbox(label, &filterEnabled)) {
                if (filterEnabled) {
                    _postFX.pushFilter(filter, overrideScene);
                } else {
                    _postFX.popFilter(filter, overrideScene);
                }
            }
            ImGui::PopID();
        };

        F32 edgeThreshold = batch.edgeDetectionThreshold();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Edge Threshold: "); ImGui::SameLine();
        ImGui::PushItemWidth(170);
        if (ImGui::SliderFloat("##hidelabel", &edgeThreshold, 0.01f, 1.0f)) {
            batch.edgeDetectionThreshold(edgeThreshold);
        }
        ImGui::PopItemWidth();
        if (ImGui::CollapsingHeader("SS Antialiasing")) {
            PreRenderOperator* op = batch.getOperator(FilterType::FILTER_SS_ANTIALIASING);
            PostAAPreRenderOperator& aaOp = static_cast<PostAAPreRenderOperator&>(*op);
            I32 level = to_I32(aaOp.postAAQualityLevel());

            ImGui::PushItemWidth(175);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Quality level: "); ImGui::SameLine();
            ImGui::PushID("quality_level_slider");
            if (ImGui::SliderInt("##hidelabel", &level, 0, 5)) {
                aaOp.postAAQualityLevel(to_U8(level));
            }
            ImGui::PopID();
            ImGui::PopItemWidth();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Method: "); ImGui::SameLine();
            static I32 selection = 0;
            const bool a = ImGui::RadioButton("SMAA", &selection, 0); ImGui::SameLine();
            const bool b = ImGui::RadioButton("FXAA", &selection, 1); ImGui::SameLine();
            const bool c = ImGui::RadioButton("NONE", &selection, 2);
            if (a || b|| c) {
                if (selection != 2) {
                    _postFX.pushFilter(FilterType::FILTER_SS_ANTIALIASING);
                    aaOp.useSMAA(selection == 0);
                } else {
                    _postFX.popFilter(FilterType::FILTER_SS_ANTIALIASING);
                }
            }
        }
        if (ImGui::CollapsingHeader("SS Ambient Occlusion")) {
            checkBox(FilterType::FILTER_SS_AMBIENT_OCCLUSION);
            PreRenderOperator* op = batch.getOperator(FilterType::FILTER_SS_AMBIENT_OCCLUSION);
            SSAOPreRenderOperator& ssaoOp = static_cast<SSAOPreRenderOperator&>(*op);
            F32 radius = ssaoOp.radius();
            F32 power = ssaoOp.power();
            F32 bias = ssaoOp.bias();
            bool halfRes = ssaoOp.genHalfRes();

            if (ImGui::Checkbox("Generate Half Resolution", &halfRes)) {
                ssaoOp.genHalfRes(halfRes);
            }
            if (ImGui::SliderFloat("Radius", &radius, 0.01f, 50.0f)) {
                ssaoOp.radius(radius);
            }
            if (ImGui::SliderFloat("Power", &power, 1.0f, 10.0f)) {
                ssaoOp.power(power);
            }
            if (ImGui::SliderFloat("Bias", &bias, 0.001f, 0.99f)) {
                ssaoOp.bias(bias);
            }
            bool blur = ssaoOp.blurResults();
            if (ImGui::Checkbox("Blur results", &blur)) {
                ssaoOp.blurResults(blur);
            }
            if (!blur) {
                PushReadOnly();
            }
            F32 blurThreshold = ssaoOp.blurThreshold();
            if (ImGui::SliderFloat("Blur threshold", &blurThreshold, 0.01f, 0.999f)) {
                ssaoOp.blurThreshold(blurThreshold);
            }
            if (!blur) {
                PopReadOnly();
            }
            ImGui::Text("SSAO Sample Count: %d", ssaoOp.sampleCount());
        }
        if (ImGui::CollapsingHeader("Depth of Field")) {
            checkBox(FilterType::FILTER_DEPTH_OF_FIELD);
            PreRenderOperator* op = batch.getOperator(FilterType::FILTER_DEPTH_OF_FIELD);
            DoFPreRenderOperator& dofOp = static_cast<DoFPreRenderOperator&>(*op);
            DoFPreRenderOperator::Parameters params = dofOp.parameters();
            bool dirty = false;

            F32& focalLength = params._focalLength;
            if (ImGui::SliderFloat("Focal Length (mm)", &focalLength, 0.0f, 100.0f)) {
                dirty = true;
            }

            I32 crtStop = to_I32(params._fStop);
            const char* crtStopName = TypeUtil::FStopsToString(params._fStop);
            if (ImGui::SliderInt("FStop", &crtStop, 0, to_base(FStops::COUNT) - 1, crtStopName)) {
                params._fStop = static_cast<FStops>(crtStop);
                dirty = true;
            }

            bool& autoFocus = params._autoFocus;
            if (ImGui::Checkbox("Auto Focus", &autoFocus)) {
                dirty = true;
            }

            if (autoFocus) {
                PushReadOnly();
            }
            F32& focalDepth = params._focalDepth;
            if (ImGui::SliderFloat("Focal Depth (m)", &focalDepth, 0.0f, 100.0f)) {
                dirty = true;
            }
            vec2<F32>& focalPosition = params._focalPoint;
            if (ImGui::SliderFloat2("Focal Position", focalPosition._v, 0.0f, 1.0f)) {
                dirty = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Position of focused point on screen (0.0,0.0 - left lower corner, 1.0,1.0 - upper right)");
            }
            if (autoFocus) {
                PopReadOnly();
            }

            bool& manualdof = params._manualdof;
            if (ImGui::Checkbox("Manual dof calculation", &manualdof)) {
                dirty = true;
            }
            if (!manualdof) {
                PushReadOnly();
            }
            F32& ndofstart = params._ndofstart;
            if (ImGui::SliderFloat("Near dof blur start", &ndofstart, 0.0f, 100.0f)) {
                dirty = true;
            }
            F32& ndofdist = params._ndofdist;
            if (ImGui::SliderFloat("Near dof blur falloff distance", &ndofdist, 0.0f, 100.0f)) {
                dirty = true;
            }
            F32& fdofstart = params._fdofstart;
            if (ImGui::SliderFloat("Far dof blur start", &fdofstart, 0.0f, 100.0f)) {
                dirty = true;
            }
            F32& fdofdist = params._fdofdist;
            if (ImGui::SliderFloat("Far dof blur falloff distance", &fdofdist, 0.0f, 100.0f)) {
                dirty = true;
            }
            if (!manualdof) {
                PopReadOnly();
            }
            bool& vignetting = params._vignetting;
            if (ImGui::Checkbox("Use optical lens vignetting", &vignetting)) {
                dirty = true;
            }
            if (!vignetting) {
                PushReadOnly();
            }
            F32& vignout = params._vignout;
            if (ImGui::SliderFloat("Vignetting outer border", &vignout, 0.0f, 100.0f)) {
                dirty = true;
            }
            F32& vignin = params._vignin;
            if (ImGui::SliderFloat("Vignetting inner border", &vignin, 0.0f, 100.0f)) {
                dirty = true;
            }
            if (!vignetting) {
                PopReadOnly();
            }
          
            bool& debugFocus = params._debugFocus;
            if (ImGui::Checkbox("Show debug focus point and focal range", &debugFocus)) {
                dirty = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("red = focal point, green = focal range");
            }
 
            if (dirty) {
                dofOp.parameters(params);
            }
        }

        if (ImGui::CollapsingHeader("Bloom")) {
            checkBox(FilterType::FILTER_BLOOM);
            PreRenderOperator* op = batch.getOperator(FilterType::FILTER_BLOOM);
            BloomPreRenderOperator& bloomOp = static_cast<BloomPreRenderOperator&>(*op);
            F32 factor = bloomOp.factor();
            F32 threshold = bloomOp.luminanceThreshold();
            if (ImGui::SliderFloat("Factor", &factor, 0.01f, 3.0f)) {
                bloomOp.factor(factor);
            }
            if (ImGui::SliderFloat("Luminance Threshold", &threshold, 0.0f, 1.0f)) {
                bloomOp.luminanceThreshold(threshold);
            }
        }

        if (ImGui::CollapsingHeader("Motion Blur")) {
            checkBox(FilterType::FILTER_MOTION_BLUR);
            PreRenderOperator* op = batch.getOperator(FilterType::FILTER_MOTION_BLUR);
            MotionBlurPreRenderOperator& blurOP = static_cast<MotionBlurPreRenderOperator&>(*op);
            F32 velocity = blurOP.velocityScale();
            if (ImGui::SliderFloat("Veclocity Scale", &velocity, 0.01f, 3.0f)) {
                blurOP.velocityScale(velocity);
            }
            U8 samples = blurOP.maxSamples(); constexpr U8 min = 1u, max = 16u;
            if (ImGui::SliderScalar("Max Samples", ImGuiDataType_U8, &samples, &min, &max)) {
                blurOP.maxSamples(samples);
            }
        }

        if (ImGui::CollapsingHeader("Tone Mapping")) {
            bool adaptiveExposure = batch.adaptiveExposureControl();
            if (ImGui::Checkbox("Adaptive Exposure", &adaptiveExposure)) {
                batch.adaptiveExposureControl(adaptiveExposure);
            }

            bool dirty = false;
            ToneMapParams params = batch.toneMapParams();
            if (adaptiveExposure) {
                if (ImGui::SliderFloat("Min Log Luminance", &params._minLogLuminance, -16.0f, 0.0f)) {
                    dirty = true;
                }
                if (ImGui::SliderFloat("Max Log Luminance", &params._maxLogLuminance, 0.0f, 16.0f)) {
                    dirty = true;
                }
                if (ImGui::SliderFloat("Tau", &params._tau, 0.1f, 2.0f)) {
                    dirty = true;
                }

                static I32 exposureRefreshFrameCount = 1;
                static F32 exposure = 1.0f;
                if (--exposureRefreshFrameCount == 0) {
                    exposure = batch.adaptiveExposureValue();
                    exposureRefreshFrameCount = g_exposureRefreshFrameCount;
                }

                ImGui::Text("Current exposure value: %5.2f", exposure);
                I32 id = 32132131;
                if (PreviewTextureButton(id, batch.luminanceTex().get(), false)) {
                    _previewTexture = batch.luminanceTex().get();
                }
            }

            if (ImGui::SliderFloat("Manual exposure", &params._manualExposure, -20.0f, 100.0f)) {
                dirty = true;
            }

            I32 crtFunc = to_I32(params._function);
            const char* crtFuncName = TypeUtil::ToneMapFunctionsToString(params._function);
            if (ImGui::SliderInt("ToneMap Function", &crtFunc, 0, to_base(ToneMapParams::MapFunctions::COUNT), crtFuncName)) {
                params._function = static_cast<ToneMapParams::MapFunctions>(crtFunc);
                dirty = true;
            }

            if (dirty) {
                batch.toneMapParams(params);
            }
            checkBox(FilterType::FILTER_LUT_CORECTION, PostFX::FilterName(FilterType::FILTER_LUT_CORECTION), false);
        }
        if (ImGui::CollapsingHeader("Test Effects")) {
            checkBox(FilterType::FILTER_NOISE, PostFX::FilterName(FilterType::FILTER_NOISE), true);
            checkBox(FilterType::FILTER_VIGNETTE, PostFX::FilterName(FilterType::FILTER_VIGNETTE), true);
            checkBox(FilterType::FILTER_UNDERWATER, PostFX::FilterName(FilterType::FILTER_UNDERWATER), true);
        }

        PushReadOnly();
        if (ImGui::CollapsingHeader("SS Reflections")) {
            checkBox(FilterType::FILTER_SS_REFLECTIONS);
        }
        PopReadOnly();

        if (_previewTexture != nullptr) {
            if (Attorney::EditorGeneralWidget::modalTextureView(_context.editor(), Util::StringFormat("Image Preview: %s", _previewTexture->resourceName().c_str()).c_str(), _previewTexture, vec2<F32>(512, 512), true, false)) {
                _previewTexture = nullptr;
            }
        }
    }
}