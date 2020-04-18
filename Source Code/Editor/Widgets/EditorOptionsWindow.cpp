#include "stdafx.h"

#include "Headers/UndoManager.h"

#include "Editor/Headers/Editor.h"
#include "Headers/EditorOptionsWindow.h"
#include "Core/Headers/PlatformContext.h"

namespace Divide {
    EditorOptionsWindow::EditorOptionsWindow(PlatformContext& context)
        : PlatformContextComponent(context)
    {
    }

    EditorOptionsWindow::~EditorOptionsWindow()
    {
    }

    void EditorOptionsWindow::update(const U64 deltaTimeUS) {
        ACKNOWLEDGE_UNUSED(deltaTimeUS);
    }

    void EditorOptionsWindow::draw(bool& open) {
        if (!open) {
            return;
        }

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetMainViewport()->Pos.x + ImGui::GetIO().DisplaySize.x * 0.5f,
                                       ImGui::GetMainViewport()->Pos.y + ImGui::GetIO().DisplaySize.y * 0.5f),
                                ImGuiCond_Always,
                                ImVec2(0.5f, 0.5f));
        if (!ImGui::Begin("Editor options", NULL, ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking))
        {
            ImGui::End();
            return;
        }
        
        static UndoEntry<I32> undo = {};

        const I32 crtThemeIdx = to_I32(Attorney::EditorGeneralWidget::getTheme(_context.editor()));
        I32 selection = crtThemeIdx;

        if (ImGui::Combo("Editor Theme", &selection, ImGui::GetDefaultStyleNames(), ImGuiStyle_Count)) {

            ImGui::ResetStyle(static_cast<ImGuiStyleEnum>(selection));
            Attorney::EditorGeneralWidget::setTheme(_context.editor(), static_cast<ImGuiStyleEnum>(selection));

            undo._type = GFX::PushConstantType::INT;
            undo._name = "Theme Selection";
            undo.oldVal = crtThemeIdx;
            undo.newVal = selection;
            undo._dataSetter = [this](const void* data) {
                const ImGuiStyleEnum style = *(ImGuiStyleEnum*)data;
                ImGui::ResetStyle(style);
                Attorney::EditorGeneralWidget::setTheme(_context.editor(), style);
            };
            _context.editor().registerUndoEntry(undo);
            ++_changeCount;
        }

        ImGui::Separator();

        ImGui::Separator();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            open = false;
            for (U16 i = 0; i < _changeCount; ++i) {
                _context.editor().Undo();
            }
            _changeCount = 0u;
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Save", ImVec2(120, 0))) {
            open = false;
            _changeCount = 0u;
            _context.editor().saveToXML();
        }
        ImGui::SameLine();
        if (ImGui::Button("Defaults", ImVec2(120, 0))) {
            _changeCount = 0u;
            Attorney::EditorGeneralWidget::setTheme(_context.editor(), ImGuiStyleEnum::ImGuiStyle_DarkCodz01);
            ImGui::ResetStyle(ImGuiStyleEnum::ImGuiStyle_DarkCodz01);
        }
        ImGui::End();
    }
};