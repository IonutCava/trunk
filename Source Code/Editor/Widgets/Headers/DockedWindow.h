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

#ifndef _EDITOR_DOCKED_WINDOW_H_
#define _EDITOR_DOCKED_WINDOW_H_

#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

class Editor;
class DockedWindow {
    public:
        struct Descriptor {
            ImVec2 size;
            ImVec2 position;
            ImVec2 minSize = ImVec2(0, 0);
            ImVec2 maxSize = ImVec2(FLT_MAX, FLT_MAX);
            stringImpl name;
            ImGuiWindowFlags flags = 0;
        };

    public:
        explicit DockedWindow(Editor& parent, const Descriptor& descriptor);
        virtual ~DockedWindow();

        void draw();

        virtual const char* name() const { return _descriptor.name.c_str(); }
        
        virtual bool hasFocus() const { return _focused; }
        virtual bool isHovered() const { return _isHovered; }

        inline const Descriptor& descriptor() const { return _descriptor; }

    protected:
        virtual void drawInternal() = 0;

    protected:
        Editor & _parent;

        bool _focused;
        bool _isHovered;
        Descriptor _descriptor;
};
}; //namespace Divide

#endif //_EDITOR_DOCKED_WINDOW_H_