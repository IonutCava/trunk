#include "stdafx.h"

#include "Headers/SceneViewWindow.h"

#include "Editor/Headers/Editor.h"

#include "Editor/Widgets/Headers/ImGuiExtensions.h"

#include "Core/Headers/PlatformContext.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include <imgui_internal.h>

namespace Divide {

    SceneViewWindow::SceneViewWindow(Editor& parent, const Descriptor& descriptor)
        : DockedWindow(parent, descriptor),
          _scenePlaying(true)
    {
    }

    SceneViewWindow::~SceneViewWindow()
    {
    }

    bool SceneViewWindow::step() {
        if (_scenePlaying) {
            return true;
        }

        return _stepQueue > 0 ? _stepQueue-- > 0 : false;
    }

    void SceneViewWindow::drawInternal() {

        auto button = [](bool disabled, const char* label, const char* tooltip, bool small = false) -> bool {
            bool ret = false;
            if (disabled) {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }
            ret = small ? ImGui::SmallButton(label) : ImGui::Button(label);

            if (disabled) {
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip(tooltip);
            }

            return ret;
        };

        ImGui::ToggleButton("Play", &_scenePlaying);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Toggle scene playback");
        }
        ImGui::SameLine();
        bool enableStepButtons = !_scenePlaying && _stepQueue == 0;
        if (button(!enableStepButtons,
                   ">|",
                    "When playback is paused, advanced the simulation by 1 full frame"))
        {
            _stepQueue = 1;
        }

        ImGui::SameLine();

        if (button(!enableStepButtons,
                   ">>|",
                   Util::StringFormat("When playback is paused, advanced the simulation by %d full frame", Config::TARGET_FRAME_RATE).c_str()))
        {
            _stepQueue = Config::TARGET_FRAME_RATE;
        }

        ImGui::SameLine();

        ImGuiWindow* window = ImGui::GetCurrentWindow();

        bool enableGizmo = Attorney::EditorSceneViewWindow::editorEnableGizmo(_parent);
        TransformSettings settings = _parent.getTransformSettings();

        const F32 ItemSpacing = ImGui::GetStyle().ItemSpacing.x;
        static F32 TButtonWidth = 10.0f;
        static F32 RButtonWidth = 10.0f;
        static F32 SButtonWidth = 10.0f;
        static F32 NButtonWidth = 10.0f;
        if (_focused && enableGizmo) {
            if (ImGui::IsKeyPressed(Input::KeyCode::KC_T)) {
                settings.currentGizmoOperation = ImGuizmo::TRANSLATE;
            }

            if (ImGui::IsKeyPressed(Input::KeyCode::KC_R)) {
                settings.currentGizmoOperation = ImGuizmo::ROTATE;
            }

            if (ImGui::IsKeyPressed(Input::KeyCode::KC_S)) {
                settings.currentGizmoOperation = ImGuizmo::SCALE;
            }
        }

        F32 pos = SButtonWidth + ItemSpacing + 25;
        ImGui::SameLine(window->SizeContents.x - pos);
        if (button(!enableGizmo || settings.currentGizmoOperation == ImGuizmo::SCALE, "S", "Scale", true)) {
            settings.currentGizmoOperation = ImGuizmo::SCALE;
        }
        SButtonWidth = ImGui::GetItemRectSize().x;

        pos += RButtonWidth + ItemSpacing + 5;
        ImGui::SameLine(window->SizeContents.x - pos);
        if (button(!enableGizmo || settings.currentGizmoOperation == ImGuizmo::ROTATE, "R", "Rotate", true)) {
            settings.currentGizmoOperation = ImGuizmo::ROTATE;
        }
        RButtonWidth = ImGui::GetItemRectSize().x;

        pos += TButtonWidth + ItemSpacing + 5;
        ImGui::SameLine(window->SizeContents.x - pos);
        if (button(!enableGizmo || settings.currentGizmoOperation == ImGuizmo::TRANSLATE, "T", "Translate", true)) {
            settings.currentGizmoOperation = ImGuizmo::TRANSLATE;
        }
        TButtonWidth = ImGui::GetItemRectSize().x;

        pos += NButtonWidth + ItemSpacing + 5;
        ImGui::SameLine(window->SizeContents.x - pos);
        if (button(false, "N", "Select", true)) {
            
        }
        NButtonWidth = ImGui::GetItemRectSize().x;

        const RenderTarget& rt = _parent.context().gfx().renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::EDITOR));
        const Texture_ptr& gameView = rt.getAttachment(RTAttachmentType::Colour, 0).texture();

        I32 w = (I32)gameView->getWidth();
        I32 h = (I32)gameView->getHeight();

        if (window && !window->SkipItems && h > 0) {

            F32 zoom = 1.f;
            ImVec2 curPos = ImGui::GetCursorPos();
            const ImVec2 wndSz(ImGui::GetWindowSize().x - curPos.x - 30.0f, ImGui::GetWindowSize().y - curPos.y - 30.0f);
            IM_ASSERT(wndSz.x != 0 && wndSz.y != 0 && zoom != 0);

            ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + wndSz.x, window->DC.CursorPos.y + wndSz.y));
            ImGui::ItemSize(bb);
            if (ImGui::ItemAdd(bb, NULL)) {

                ImVec2 imageSz = wndSz - ImVec2(0.2f, 0.2f);
                ImVec2 remainingWndSize(0, 0);
                const F32 aspectRatio = (F32)w / (F32)h;

                if (aspectRatio != 0) {
                    const F32 wndAspectRatio = wndSz.x / wndSz.y;
                    if (aspectRatio >= wndAspectRatio) {
                        imageSz.y = imageSz.x / aspectRatio;
                        remainingWndSize.y = wndSz.y - imageSz.y;
                    }
                    else {
                        imageSz.x = imageSz.y*aspectRatio;
                        remainingWndSize.x = wndSz.x - imageSz.x;
                    }
                }

                const F32 zoomFactor = .5f / zoom;
                ImVec2 uvExtension = ImVec2(2.f*zoomFactor, 2.f*zoomFactor);
                if (remainingWndSize.x > 0) {
                    const F32 remainingSizeInUVSpace = 2.f*zoomFactor*(remainingWndSize.x / imageSz.x);
                    const F32 deltaUV = uvExtension.x;
                    const F32 remainingUV = 1.f - deltaUV;
                    if (deltaUV < 1) {
                        F32 adder = (remainingUV < remainingSizeInUVSpace ? remainingUV : remainingSizeInUVSpace);
                        uvExtension.x += adder;
                        remainingWndSize.x -= adder * zoom * imageSz.x;
                        imageSz.x += adder * zoom * imageSz.x;
                    }
                }
                if (remainingWndSize.y > 0) {
                    const F32 remainingSizeInUVSpace = 2.f*zoomFactor*(remainingWndSize.y / imageSz.y);
                    const F32 deltaUV = uvExtension.y;
                    const F32 remainingUV = 1.f - deltaUV;
                    if (deltaUV < 1) {
                        F32 adder = (remainingUV < remainingSizeInUVSpace ? remainingUV : remainingSizeInUVSpace);
                        uvExtension.y += adder;
                        remainingWndSize.y -= adder * zoom * imageSz.y;
                        imageSz.y += adder * zoom * imageSz.y;
                    }
                }


                ImVec2 startPos = bb.Min, endPos = bb.Max;
                startPos.x += remainingWndSize.x*.5f;
                startPos.y += remainingWndSize.y*.5f;
                endPos.x = startPos.x + imageSz.x;
                endPos.y = startPos.y + imageSz.y;

                const ImGuiID id = (ImGuiID)((U64)&_parent) + 1;
                ImGui::PushID(id);

                window->DrawList->AddImage((void *)(intptr_t)gameView->getHandle(), startPos, endPos, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

                _sceneRect.set(startPos.x, startPos.y, endPos.x, endPos.y);

                ImGui::PopID();
            }
        }

        if (ImGui::RadioButton("Local", settings.currentGizmoMode == ImGuizmo::LOCAL)) {
            settings.currentGizmoMode = ImGuizmo::LOCAL;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("World", settings.currentGizmoMode == ImGuizmo::WORLD)) {
            settings.currentGizmoMode = ImGuizmo::WORLD;
        }
        ImGui::SameLine();
        ImGui::Checkbox("Snap", &settings.useSnap);
        if (settings.useSnap) {
            ImGui::SameLine();
            ImGui::Text("Step:");
            ImGui::SameLine();
            ImGui::PushItemWidth(150);
            switch (settings.currentGizmoOperation)
            {
                case ImGuizmo::TRANSLATE:
                    ImGui::InputFloat3("Position", &settings.snap[0]);
                    break;
                case ImGuizmo::ROTATE:
                    ImGui::InputFloat("Angle", &settings.snap[0]);
                    break;
                case ImGuizmo::SCALE:
                    ImGui::InputFloat("Scale", &settings.snap[0]);
                    break;
            }
            ImGui::PopItemWidth();
        }
        ImGui::Separator();
        _parent.setTransformSettings(settings);
    }
};