/*
Copyright (c) 2018 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef _EDITOR_OUTPUT_WINDOW_H_
#define _EDITOR_OUTPUT_WINDOW_H_

#include "Core/Headers/Console.h"
#include "Editor/Widgets/Headers/DockedWindow.h"
#include <CircularBuffer/CircularBuffer.h>

namespace Divide {

class ApplicationOutput;
class OutputWindow : public DockedWindow {
    public:
        OutputWindow(Editor& parent, const Descriptor& descriptor);
        ~OutputWindow();

        void drawInternal() override;
        void printText(const Console::OutputEntry& entry);
        I32 textEditCallback(ImGuiTextEditCallbackData* data);

    protected:
        void clearLog();
        void executeCommand(const char* command_line);

    protected:
        size_t _consoleCallbackIndex;

        bool _scrollToBottom;
        char _inputBuf[256];
        ImGuiTextFilter _filter;
        circular_buffer<Console::OutputEntry> _log;
};
}; //namespace Divide

#endif //_EDITOR_OUTPUT_WINDOW_H_