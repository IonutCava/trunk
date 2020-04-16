#include "stdafx.h"

#include "Headers/OutputWindow.h"
#include "Editor/Headers/Editor.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Headers/StringHelper.h"

namespace Divide {
    OutputWindow::OutputWindow(Editor& parent, const Descriptor& descriptor)
        : DockedWindow(parent, descriptor),
         _log(512),
         _scrollToBottom(true)
    {
        memset(_inputBuf, 0, sizeof(_inputBuf));

        _consoleCallbackIndex = Console::bindConsoleOutput([this](const Console::OutputEntry& entry) {
            printText(entry);
        });
    }

    OutputWindow::~OutputWindow()
    {
        Console::unbindConsoleOutput(_consoleCallbackIndex);
        clearLog();
    }


    void OutputWindow::clearLog() {
        _log.reset();
        _scrollToBottom = true;
    }

    void OutputWindow::drawInternal() {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

        if (ImGui::SmallButton("Clear")) {
            clearLog();
        }
        ImGui::SameLine();

        bool copy_to_clipboard = ImGui::SmallButton("Copy");
        ImGui::SameLine();
        if (ImGui::Checkbox("Scroll to bottom", &_scrollToBottom)) {
            
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        _filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
        ImGui::PopStyleVar();
        ImGui::Separator();

        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::Selectable("Clear")) {
                clearLog();
            }
            ImGui::EndPopup();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing

        if (copy_to_clipboard) {
            ImGui::LogToClipboard();
        }

        size_t entryCount = _log.size();
        static ImVec4 colours[] = {
            ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
            ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
            ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
            ImVec4(0.0f, 0.0f, 1.0f, 1.0f)
        };

        if (entryCount > 0) {
            for (size_t i = 0; i < entryCount; ++i) {
                const Console::OutputEntry& message = _log.get(i);

                if (!_filter.PassFilter(message._text.c_str())) {
                    continue;
                }

                ImGui::PushStyleColor(ImGuiCol_Text, colours[to_U8(message._type)]);
                ImGui::TextUnformatted(message._text.c_str());
                ImGui::PopStyleColor();
            }
        }
        if (copy_to_clipboard) {
            ImGui::LogFinish();
        }
        if (_scrollToBottom) {
            ImGui::SetScrollHere();
        }

        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::Separator();

        if (ImGui::InputText("Input",
                             _inputBuf,
                             IM_ARRAYSIZE(_inputBuf),
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory,
                             [](ImGuiTextEditCallbackData* data){
                                OutputWindow* console = (OutputWindow*)data->UserData;
                                return console->textEditCallback(data);
                             },
                             (void*)this))
        {
            char* input_end = _inputBuf + strlen(_inputBuf);
            while (input_end > _inputBuf && input_end[-1] == ' ') {
                input_end--;
            }
            *input_end = 0;

            if (_inputBuf[0]) {
                executeCommand(_inputBuf);
            }
            strcpy(_inputBuf, "");
        }

        // Demonstrate keeping auto focus on the input box
        if (ImGui::IsItemHovered() || (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))) {
            ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
        }
        ImGui::PopStyleColor();
    }

    void OutputWindow::printText(const Console::OutputEntry& entry) {
        _log.put(entry);
    }

    void OutputWindow::executeCommand(const char* command_line) {
        printText(
            {
                Util::StringFormat("# %s\n", command_line).c_str(),
                Console::EntryType::Command
            }
        );
    }

    I32 OutputWindow::textEditCallback(ImGuiTextEditCallbackData* data) {
        switch (data->EventFlag)
        {
            case ImGuiInputTextFlags_CallbackCompletion:
                break;
            case ImGuiInputTextFlags_CallbackHistory:
                break;
        };

        return -1;
    }
};