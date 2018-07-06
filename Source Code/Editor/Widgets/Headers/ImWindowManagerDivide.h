/*
Copyright (c) 2017 DIVIDE-Studio
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

#ifndef _IM_WINDOW_MANAGER_DIVIDE_H_
#define _IM_WINDOW_MANAGER_DIVIDE_H_

#include "ImwConfig.h"
#include "ImwWindowManager.h"

namespace Divide {

namespace Attorney {
    class WindowManagerWindow;
};

class Editor;
class ImwWindowDivide;
class PlatformContext;
class ImwWindowManagerDivide : public ImWindow::ImwWindowManager
{
    friend class Attorney::WindowManagerWindow;

  public:
    explicit ImwWindowManagerDivide(Editor& parent);
    virtual  ~ImwWindowManagerDivide();

    void renderDrawList(ImDrawData* pDrawData, I64 windowGUID);

  protected:
    void registerWindow(ImwWindowDivide* window);
    void unregisterWindow(ImwWindowDivide* window);

    virtual bool CanCreateMultipleWindow() { return true; }

    virtual ImWindow::ImwPlatformWindow* CreatePlatformWindow(ImWindow::EPlatformWindowType eType, ImWindow::ImwPlatformWindow* pParent);

    virtual ImVec2 GetCursorPos();
    virtual bool IsLeftClickDown();

   private:
     U32 _windowCount;
     Editor& _parent;
};

namespace Attorney {
    class WindowManagerWindow {
      private:
      static void registerWindow(ImwWindowManagerDivide& mgr, ImwWindowDivide* window) {
          mgr.registerWindow(window);
      }
      static void unregisterWindow(ImwWindowManagerDivide& mgr, ImwWindowDivide* window) {
          mgr.unregisterWindow(window);
      }
      friend class Divide::ImwWindowDivide;
    };
};
}; //namespace Divide

#endif //_IM_WINDOW_MANAGER_DIVIDE_H_