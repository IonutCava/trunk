#include "stdafx.h"

#include "Headers/MenuBar.h"

#include "Editor/Headers/Editor.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/PlatformContext.h"

#include "Managers/Headers/SceneManager.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Rendering/PostFX/Headers/PostFX.h"
#include "Rendering/PostFX/Headers/PreRenderOperator.h"

#include <imgui/addons/imguifilesystem/imguifilesystem.h>

namespace Divide {
namespace {
    const char* UsageToString(RenderTargetUsage usage) {
        switch (usage) {
            case RenderTargetUsage::EDITOR: return "Editor";
            case RenderTargetUsage::ENVIRONMENT: return "Environment";
            case RenderTargetUsage::HI_Z: return "HI-Z";
            case RenderTargetUsage::HI_Z_REFLECT: return "HI-Z Low";
            case RenderTargetUsage::OIT: return "OIT";
            case RenderTargetUsage::OTHER: return "Other";
            case RenderTargetUsage::REFLECTION_CUBE: return "Cube Reflection";
            case RenderTargetUsage::REFLECTION_PLANAR: return "Planar Reflection";
            case RenderTargetUsage::REFLECTION_PLANAR_BLUR: return "Planar Reflection Blur";
            case RenderTargetUsage::REFRACTION_PLANAR: return "Planar Refraction";
            case RenderTargetUsage::SCREEN: return "Screen";
            case RenderTargetUsage::SHADOW: return "Shadow";
        };

        return "Unknown";
    }
};


MenuBar::MenuBar(PlatformContext& context, bool mainMenu)
    : PlatformContextComponent(context),
      _isMainMenu(mainMenu),
      _previewTexture(nullptr),
      _quitPopup(false),
      _closePopup(false)
{

}

MenuBar::~MenuBar()
{

}

void MenuBar::draw() {
    if ((ImGui::BeginMenuBar()))
    {
        drawFileMenu();
        drawEditMenu();
        drawProjectMenu();
        drawObjectMenu();
        drawToolsMenu();
        drawWindowsMenu();
        drawPostFXMenu();
        drawHelpMenu();

        ImGui::EndMenuBar();

        if (_previewTexture != nullptr) {
            ImGui::OpenPopup("Image Preview");
            if (Attorney::EditorGeneralWidget::modalTextureView(_context.editor(), "Image Preview", _previewTexture, vec2<F32>(512, 512), true)) {
                _previewTexture = nullptr;
            }
        }

        if (_closePopup) {
            ImGui::OpenPopup("Confirm Close");

            if (ImGui::BeginPopupModal("Confirm Close", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Are you sure you want to close the editor? You have unsaved items!");
                ImGui::Separator();

                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                    _closePopup = false;
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button("Yes", ImVec2(120, 0))) {

                    ImGui::CloseCurrentPopup();
                    _closePopup = false;
                    _context.editor().toggle(false);
                }
                ImGui::EndPopup();
            }
        }

        if (_quitPopup) {
            ImGui::OpenPopup("Confirm Quit");
            if (ImGui::BeginPopupModal("Confirm Quit", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Are you sure you want to quit?");
                ImGui::Separator();

                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                    _quitPopup = false;
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button("Yes", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                    _quitPopup = false;
                    context().app().RequestShutdown();
                }
                ImGui::EndPopup();
            }
        }
    }
}

void MenuBar::drawFileMenu() {
    bool showFileOpenDialog = false;
    bool showFileSaveDialog = false;

    bool hasUnsavedElements = Attorney::EditorGeneralWidget::hasUnsavedElements(_context.editor());

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("New")) {}
        showFileOpenDialog = ImGui::MenuItem("Open", "Ctrl+O");
        if (ImGui::BeginMenu("Open Recent"))
        {
            ImGui::MenuItem("_PLACEHOLDER_A");
            ImGui::MenuItem("_PLACEHOLDER_B");
            ImGui::EndMenu();
        }
        showFileSaveDialog = ImGui::MenuItem(hasUnsavedElements ? "Save*" : "Save", "Ctrl+S");
        
        if (ImGui::MenuItem(hasUnsavedElements ? "Save All*" : "Save All")) {
            Attorney::EditorGeneralWidget::saveElement(_context.editor(), -1);
            _context.kernel().sceneManager().saveActiveScene(false);
            _context.editor().saveToXML();
        }

        ImGui::Separator();
        if (ImGui::BeginMenu("Options"))
        {
            static bool enabled = true;
            ImGui::MenuItem("Enabled", "", &enabled);
            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Close Editor"))
        {
            if (hasUnsavedElements) {
                _closePopup = true;
            } else {
                _context.editor().toggle(false);
            }
        }

        if (ImGui::MenuItem("Quit", "Alt+F4"))
        {
            _quitPopup = true;
        }

        ImGui::EndMenu();
    }

    static ImGuiFs::Dialog s_fileOpenDialog(true, true);
    static ImGuiFs::Dialog s_fileSaveDialog(true, true);
    s_fileOpenDialog.chooseFileDialog(showFileOpenDialog);
    s_fileSaveDialog.chooseFileDialog(showFileSaveDialog);

    if (strlen(s_fileOpenDialog.getChosenPath()) > 0) {
        ImGui::Text("Chosen file: \"%s\"", s_fileOpenDialog.getChosenPath());
    }

    if (strlen(s_fileSaveDialog.getChosenPath()) > 0) {
        ImGui::Text("Chosen file: \"%s\"", s_fileSaveDialog.getChosenPath());
        Attorney::EditorGeneralWidget::saveElement(_context.editor(), -1);
    }
}

void MenuBar::drawEditMenu() {
    if (ImGui::BeginMenu("Edit"))
    {
        if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
        if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
        ImGui::Separator();
        if (ImGui::MenuItem("Cut", "CTRL+X")) {}
        if (ImGui::MenuItem("Copy", "CTRL+C")) {}
        if (ImGui::MenuItem("Paste", "CTRL+V")) {}
        ImGui::Separator();
        ImGui::EndMenu();
    }
}
void MenuBar::drawProjectMenu() {
    if (ImGui::BeginMenu("Project"))
    {
        if(ImGui::MenuItem("Configuration")) {}
        ImGui::EndMenu();
    }
}
void MenuBar::drawObjectMenu() {
    if (ImGui::BeginMenu("Object"))
    {
        if(ImGui::MenuItem("New Node")) {}
        ImGui::EndMenu();
    }

}
void MenuBar::drawToolsMenu() {
    if (ImGui::BeginMenu("Tools")) {
        bool memEditorEnabled = Attorney::EditorMenuBar::memoryEditorEnabled(_context.editor());
        if (ImGui::MenuItem("Memory Editor", NULL, memEditorEnabled)) {
            Attorney::EditorMenuBar::toggleMemoryEditor(_context.editor(), !memEditorEnabled);
            _context.editor().saveToXML();
        }

        if (ImGui::BeginMenu("Render Targets")) {
            const GFXRTPool& pool = _context.gfx().renderTargetPool();
            for (U8 i = 0; i < to_U8(RenderTargetUsage::COUNT); ++i) {
                RenderTargetUsage usage = static_cast<RenderTargetUsage>(i);
                auto rTargets = pool.renderTargets(usage);
                if (rTargets.empty()) {
                    ImGui::MenuItem(UsageToString(usage));
                } else {
                    if (ImGui::BeginMenu(UsageToString(usage))) {
                        for (const RenderTarget* rt : rTargets) {
                            if (rt != nullptr && ImGui::BeginMenu(rt->name().c_str())) {
                                for (U8 j = 0; j < to_U8(RTAttachmentType::COUNT); ++j) {
                                    RTAttachmentType type = static_cast<RTAttachmentType>(j);
                                    U8 count = rt->getAttachmentCount(type);
                                    for (U8 k = 0; k < count; ++k) {
                                        const RTAttachment& attachment = rt->getAttachment(type, k);
                                        const Texture_ptr& tex = attachment.texture();
                                        if (tex != nullptr && ImGui::MenuItem(attachment.texture()->resourceName().c_str())) {
                                            _previewTexture = tex;
                                        }
                                    }
                                }

                                ImGui::EndMenu();
                            }
                        }
                        ImGui::EndMenu();
                    }
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
}

void MenuBar::drawWindowsMenu() {
    if (ImGui::BeginMenu("Window"))
    {
        bool& sampleWindowEnabled = Attorney::EditorMenuBar::sampleWindowEnabled(_context.editor());
        if (ImGui::MenuItem("Sample Window", NULL, &sampleWindowEnabled)) {
            
        }
        ImGui::EndMenu();
    }

}

void MenuBar::drawPostFXMenu() {
    if (ImGui::BeginMenu("PostFX"))
    {
        for (FilterType f : FilterType::_values()) {
            bool filterEnabled = _context.gfx().postFX().getFilterState(f);
            if (f._value == FilterType::FILTER_COUNT) {
                continue;
            }
            if (ImGui::MenuItem(f._to_string(), NULL, &filterEnabled)) {
                if (filterEnabled) {
                    _context.gfx().postFX().pushFilter(f);
                } else {
                    _context.gfx().postFX().popFilter(f);
                }
            }
        }
        ImGui::EndMenu();
    }
}

void MenuBar::drawHelpMenu() {
    if (ImGui::BeginMenu("Help"))
    {
        ImGui::Text("Copyright(c) 2018 DIVIDE - Studio");
        ImGui::Text("Copyright(c) 2009 Ionut Cava");
        ImGui::EndMenu();
    }
}
}; //namespace Divide