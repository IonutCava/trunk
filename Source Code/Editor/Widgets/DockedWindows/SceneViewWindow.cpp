#include "stdafx.h"

#include "Headers/SceneViewWindow.h"
#include "Widgets/Headers/PanelManager.h"

#include "Editor/Headers/Editor.h"
#include "Core/Headers/PlatformContext.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include <imgui_internal.h>

namespace Divide {

    SceneViewWindow::SceneViewWindow(PanelManager& parent)
        : DockedWindow(parent, "SceneView")
    {

    }

    SceneViewWindow::~SceneViewWindow()
    {

    }

    void SceneViewWindow::draw() {
        ImVec2 size(0.0f, 0.0f);
        ImGuiWindow* window = ImGui::GetCurrentWindow();

        const RenderTarget& rt = _parent.context().gfx().renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::EDITOR));
        const Texture_ptr& gameView = rt.getAttachment(RTAttachmentType::Colour, 0).texture();

        int w = (int)gameView->getWidth();
        int h = (int)gameView->getHeight();

        if (window && !window->SkipItems && h > 0) {

            float zoom = 0.85f;
            ImVec2 curPos = ImGui::GetCursorPos();
            const ImVec2 wndSz(size.x > 0 ? size.x : ImGui::GetWindowSize().x - curPos.x, size.y > 0 ? size.y : ImGui::GetWindowSize().y - curPos.y);
            IM_ASSERT(wndSz.x != 0 && wndSz.y != 0 && zoom != 0);

            ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + wndSz.x, window->DC.CursorPos.y + wndSz.y));
            ImGui::ItemSize(bb);
            if (ImGui::ItemAdd(bb, NULL)) {

                ImVec2 imageSz = wndSz - ImVec2(0.2f, 0.2f);
                ImVec2 remainingWndSize(0, 0);
                const float aspectRatio = (float)w / (float)h;

                if (aspectRatio != 0) {
                    const float wndAspectRatio = wndSz.x / wndSz.y;
                    if (aspectRatio >= wndAspectRatio) {
                        imageSz.y = imageSz.x / aspectRatio;
                        remainingWndSize.y = wndSz.y - imageSz.y;
                    }
                    else {
                        imageSz.x = imageSz.y*aspectRatio;
                        remainingWndSize.x = wndSz.x - imageSz.x;
                    }
                }

                const float zoomFactor = .5f / zoom;
                ImVec2 uvExtension = ImVec2(2.f*zoomFactor, 2.f*zoomFactor);
                if (remainingWndSize.x > 0) {
                    const float remainingSizeInUVSpace = 2.f*zoomFactor*(remainingWndSize.x / imageSz.x);
                    const float deltaUV = uvExtension.x;
                    const float remainingUV = 1.f - deltaUV;
                    if (deltaUV < 1) {
                        float adder = (remainingUV < remainingSizeInUVSpace ? remainingUV : remainingSizeInUVSpace);
                        uvExtension.x += adder;
                        remainingWndSize.x -= adder * zoom * imageSz.x;
                        imageSz.x += adder * zoom * imageSz.x;
                    }
                }
                if (remainingWndSize.y > 0) {
                    const float remainingSizeInUVSpace = 2.f*zoomFactor*(remainingWndSize.y / imageSz.y);
                    const float deltaUV = uvExtension.y;
                    const float remainingUV = 1.f - deltaUV;
                    if (deltaUV < 1) {
                        float adder = (remainingUV < remainingSizeInUVSpace ? remainingUV : remainingSizeInUVSpace);
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

                Attorney::EditorSceneViewWindow::setScenePreviewRect(_parent.context().editor(), Rect<I32>(startPos.x, startPos.y, endPos.x, endPos.y));

                ImGui::PopID();
            }
        }
    }
};