#include "stdafx.h"

#include "Headers/ApplicationOutput.h"

#include "Core/Headers/StringHelper.h"

namespace Divide {
    namespace {
        static int TextEditCallbackStub(ImGuiTextEditCallbackData* data) // In C++11 you are better off using lambdas for this sort of forwarding callbacks
        {
            ApplicationOutput* console = (ApplicationOutput*)data->UserData;
            return console->textEditCallback(data);
        }
    };

    ApplicationOutput::ApplicationOutput(PlatformContext& context, U16 maxEntries)
        : PlatformContextComponent(context),
          _log(maxEntries),
          _scrollToBottom(true)
    {
        memset(_inputBuf, 0, sizeof(_inputBuf));
    }

    ApplicationOutput::~ApplicationOutput()
    {
        clearLog();
    }

    void ApplicationOutput::clearLog() {
        _log.reset();
        _scrollToBottom = true;
    }

    void ApplicationOutput::draw() {
        ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

        if (ImGui::SmallButton("Clear")) {
            clearLog();
        }
        ImGui::SameLine();

        bool copy_to_clipboard = ImGui::SmallButton("Copy");
        ImGui::SameLine();
        if (ImGui::SmallButton("Scroll to bottom")) {
            _scrollToBottom = true;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        _filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
        ImGui::PopStyleVar();
        ImGui::Separator();

        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
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

        _scrollToBottom = false;
        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::Separator();

        if (ImGui::InputText("Input", _inputBuf, IM_ARRAYSIZE(_inputBuf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory, &TextEditCallbackStub, (void*)this))
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
        if (ImGui::IsItemHovered() || (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))) {
            ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
        }
        ImGui::PopStyleColor();
    }

    void ApplicationOutput::printText(const Console::OutputEntry& entry) {
        _log.put(entry);
        _scrollToBottom = true;
    }


    int ApplicationOutput::textEditCallback(ImGuiTextEditCallbackData* data) {
        switch (data->EventFlag)
        {
            case ImGuiInputTextFlags_CallbackCompletion:
                break;
            case ImGuiInputTextFlags_CallbackHistory:
                break;
        };

        return -1;
    }

    void ApplicationOutput::executeCommand(const char* command_line) {
        printText(
            {
                Util::StringFormat("# %s\n", command_line),
                Console::EntryType::Command
            }
        );
    }
}; //namespace Divide