#include "stdafx.h"

#include "Headers/MenuBar.h"

#include "Editor/Headers/Editor.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"

#include "Managers/Headers/SceneManager.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"

#include "Rendering/Headers/Renderer.h"
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
            case RenderTargetUsage::HI_Z_REFLECT: return "HI-Z Reflect";
            case RenderTargetUsage::OIT: return "OIT";
            case RenderTargetUsage::OIT_MS: return "OIT_MS";
            case RenderTargetUsage::OIT_REFLECT: return "OIT_REFLECT";
            case RenderTargetUsage::OTHER: return "Other";
            case RenderTargetUsage::REFLECTION_CUBE: return "Cube Reflection";
            case RenderTargetUsage::REFLECTION_PLANAR: return "Planar Reflection";
            case RenderTargetUsage::REFLECTION_PLANAR_BLUR: return "Planar Reflection Blur";
            case RenderTargetUsage::REFRACTION_PLANAR: return "Planar Refraction";
            case RenderTargetUsage::SCREEN: return "Screen";
            case RenderTargetUsage::SCREEN_MS: return "Screen_MS";
            case RenderTargetUsage::SHADOW: return "Shadow";
        };

        return "Unknown";
    }

    const char* EdgeMethodName(PreRenderBatch::EdgeDetectionMethod method) {
        switch (method) {
            case PreRenderBatch::EdgeDetectionMethod::Depth: return "Depth";
            case PreRenderBatch::EdgeDetectionMethod::Luma: return "Luma";
            case PreRenderBatch::EdgeDetectionMethod::Colour: return "Colour";
        };
        return "Unknown";
    }
};


MenuBar::MenuBar(PlatformContext& context, bool mainMenu)
    : PlatformContextComponent(context),
      _isMainMenu(mainMenu),
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
        drawDebugMenu();
        drawHelpMenu();

        ImGui::EndMenuBar();

       for (vectorSTD<Texture_ptr>::iterator it = std::begin(_previewTextures); it != std::end(_previewTextures); ) {
            if (Attorney::EditorGeneralWidget::modalTextureView(_context.editor(), Util::StringFormat("Image Preview: %s", (*it)->resourceName().c_str()).c_str(), *it, vec2<F32>(512, 512), true, false)) {
                it = _previewTextures.erase(it);
            } else {
                ++it;
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

    if (ImGui::BeginMenu("File"))
    {
        const bool hasUnsavedElements = Attorney::EditorGeneralWidget::hasUnsavedElements(_context.editor());

        if (ImGui::MenuItem("New", "Ctrl+N", false, false))
        {
        }

        showFileOpenDialog = ImGui::MenuItem("Open", "Ctrl+O");
        if (ImGui::BeginMenu("Open Recent"))
        {
            ImGui::Text("Empty");
            ImGui::EndMenu();
        }

        showFileSaveDialog = ImGui::MenuItem(hasUnsavedElements ? "Save*" : "Save", "Ctrl+S");
        
        if (ImGui::MenuItem(hasUnsavedElements ? "Save All*" : "Save All"))
        {
            Attorney::EditorGeneralWidget::saveElement(_context.editor(), -1);
            _context.kernel().sceneManager().saveActiveScene(false);
            _context.config().save();
            _context.editor().saveToXML();
        }

        ImGui::Separator();
        if (ImGui::BeginMenu("Options"))
        {
            GFXDevice& gfx = _context.gfx();
            const Configuration& config = _context.config();

            if (ImGui::BeginMenu("MSAA"))
            {
                const U8 maxMSAASamples = gfx.gpuState().maxMSAASampleCount();
                for (U8 i = 0; (1 << i) <= maxMSAASamples; ++i) {
                    const U8 sampleCount = i == 0u ? 0u : 1 << i;
                    if (sampleCount % 2 == 0) {
                        bool msaaEntryEnabled = config.rendering.MSAAsamples == sampleCount;
                        if (ImGui::MenuItem(Util::StringFormat("%dx", to_U32(sampleCount)).c_str(), "", &msaaEntryEnabled))
                        {
                            gfx.setScreenMSAASampleCount(sampleCount);
                        }
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("CSM MSAA"))
            {
                const U8 maxMSAASamples = gfx.gpuState().maxMSAASampleCount();
                for (U8 i = 0; (1 << i) <= maxMSAASamples; ++i) {
                    const U8 sampleCount = i == 0u ? 0u : 1 << i;
                    if (sampleCount % 2 == 0) {
                        bool msaaEntryEnabled = config.rendering.shadowMapping.MSAAsamples == sampleCount;
                        if (ImGui::MenuItem(Util::StringFormat("%dx", to_U32(sampleCount)).c_str(), "", &msaaEntryEnabled))
                        {
                            gfx.setShadowMSAASampleCount(sampleCount);
                        }
                    }
                }
                ImGui::EndMenu();
            }

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
        if (ImGui::MenuItem("Undo", "CTRL+Z"))
        {
            _context.editor().Undo();
        }

        if (ImGui::MenuItem("Redo", "CTRL+Y"))
        {
            _context.editor().Redo();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Cut", "CTRL+X", false, false))
        {
        }

        if (ImGui::MenuItem("Copy", "CTRL+C", false, false))
        {
        }

        if (ImGui::MenuItem("Paste", "CTRL+V", false, false))
        {
        }

        ImGui::Separator();
        ImGui::EndMenu();
    }
}

void MenuBar::drawProjectMenu() {
    if (ImGui::BeginMenu("Project"))
    {
        if(ImGui::MenuItem("Configuration", "", false, false))
        {
        }

        ImGui::EndMenu();
    }
}
void MenuBar::drawObjectMenu() {
    if (ImGui::BeginMenu("Object"))
    {
        if(ImGui::MenuItem("New Node", "", false, false))
        {
        }

        ImGui::EndMenu();
    }

}
void MenuBar::drawToolsMenu() {
    if (ImGui::BeginMenu("Tools"))
    {
        bool memEditorEnabled = Attorney::EditorMenuBar::memoryEditorEnabled(_context.editor());
        if (ImGui::MenuItem("Memory Editor", NULL, memEditorEnabled)) {
            Attorney::EditorMenuBar::toggleMemoryEditor(_context.editor(), !memEditorEnabled);
            _context.editor().saveToXML();
        }

        if (ImGui::BeginMenu("Render Targets"))
        {
            const GFXRTPool& pool = _context.gfx().renderTargetPool();
            for (U8 i = 0; i < to_U8(RenderTargetUsage::COUNT); ++i) {
                const RenderTargetUsage usage = static_cast<RenderTargetUsage>(i);
                const auto& rTargets = pool.renderTargets(usage);

                if (rTargets.empty()) {
                    if (ImGui::MenuItem(UsageToString(usage), "", false, false))
                    {
                    }
                } else {
                    if (ImGui::BeginMenu(UsageToString(usage)))
                    {
                        for (const auto& rt : rTargets) {
                            if (rt == nullptr) {
                                continue;
                            }

                            if (ImGui::BeginMenu(rt->name().c_str()))
                            {
                                for (U8 j = 0; j < to_U8(RTAttachmentType::COUNT); ++j) {
                                    const RTAttachmentType type = static_cast<RTAttachmentType>(j);
                                    const U8 count = rt->getAttachmentCount(type);

                                    for (U8 k = 0; k < count; ++k) {
                                        const RTAttachment& attachment = rt->getAttachment(type, k);
                                        const Texture_ptr& tex = attachment.texture();
                                        if (tex == nullptr) {
                                            continue;
                                        }

                                        if (ImGui::MenuItem(tex->resourceName().c_str()))
                                        {
                                            _previewTextures.push_back(tex);
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

            const Texture_ptr& prevDepthBufferTex = _context.gfx().getPrevDepthBuffer();
            if (prevDepthBufferTex != nullptr) {
                if (ImGui::BeginMenu("Miscellaneous"))
                {
                    if (ImGui::MenuItem(prevDepthBufferTex->resourceName().c_str()))
                    {
                        _previewTextures.push_back(prevDepthBufferTex);
                    }
                    ImGui::EndMenu();
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
        PostFX& postFX = _context.gfx().getRenderer().postFX();
        for (U8 i = 0; i < to_base(FilterType::FILTER_COUNT); ++i) {
            if (i == to_U8(FilterType::FILTER_COUNT)) {
                continue;
            }

            const FilterType f = static_cast<FilterType>(toBit(i));
            const bool filterEnabled = postFX.getFilterState(f);
            if (ImGui::MenuItem(PostFX::FilterName(f), NULL, &filterEnabled))
            {
                if (filterEnabled) {
                    postFX.pushFilter(f, true);
                } else {
                    postFX.popFilter(f, true);
                }
            }
        }
        ImGui::EndMenu();
    }
}

void MenuBar::drawDebugMenu() {
    if (ImGui::BeginMenu("Debug"))
    {
        GFXDevice& gfx = _context.gfx();
        Configuration& config = _context.config();

        bool& enableLighting = config.debug.enableLighting;
        if (ImGui::MenuItem("Enable lighting", "", &enableLighting))
        {
            config.changed(true);
        }

        bool& showCSMSplits = config.debug.showShadowCascadeSplits;
        if (ImGui::MenuItem("Enable CSM Split View", "", &showCSMSplits))
        {
            config.changed(true);
        }

        if (ImGui::BeginMenu("Edge Detection Method")) {

            PreRenderBatch* batch = _context.gfx().getRenderer().postFX().getFilterBatch();
            bool noneSelected = batch->edgeDetectionMethod() == PreRenderBatch::EdgeDetectionMethod::COUNT;
            if (ImGui::MenuItem("None", "", &noneSelected)) {
                batch->edgeDetectionMethod(PreRenderBatch::EdgeDetectionMethod::COUNT);
            }

            for (U8 i = 0; i < to_U8(PreRenderBatch::EdgeDetectionMethod::COUNT) + 1; ++i) {
                PreRenderBatch::EdgeDetectionMethod method = static_cast<PreRenderBatch::EdgeDetectionMethod>(i);

                bool selected = batch->edgeDetectionMethod() == method;
                if (ImGui::MenuItem(EdgeMethodName(method), "", &selected)) {
                    batch->edgeDetectionMethod(method);
                }
            }

            ImGui::EndMenu();
        }

        LightPool& pool = Attorney::EditorGeneralWidget::getActiveLightPool(_context.editor());

        bool shadowDebug = pool.isDebugLight(LightType::DIRECTIONAL, 0);
        if (ImGui::MenuItem("Debug Main CSM", "", &shadowDebug))
        {
            pool.setDebugLight(shadowDebug ? LightType::DIRECTIONAL : LightType::COUNT, 0u);
        }

        bool lightImpostors = pool.lightImpostorsEnabled();
        if (ImGui::MenuItem("Draw Light Impostors", "", &lightImpostors))
        {
            pool.lightImpostorsEnabled(lightImpostors);
        }

        if (ImGui::BeginMenu("Debug Views"))
        {
            vectorEASTL<std::tuple<stringImpl, I16, bool>> viewNames = {};
            gfx.getDebugViewNames(viewNames);

            for (auto[name, index, enabled] : viewNames) {
                if (ImGui::MenuItem(name.c_str(), "", &enabled))
                {
                    gfx.toggleDebugView(index, enabled);
                }
            }
            ImGui::EndMenu();
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