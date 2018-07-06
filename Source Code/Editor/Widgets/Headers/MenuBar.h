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

#ifndef _EDITOR_MENU_BAR_H_
#define _EDITOR_MENU_BAR_H_

#include "Core/Headers/PlatformContextComponent.h"
#include "Platform/Headers/PlatformDefines.h"

namespace Divide {
    class MenuBar : public PlatformContextComponent {
      public:

        explicit MenuBar(PlatformContext& context, bool mainMenu);
        ~MenuBar();

        void draw();

      protected:
          void drawFileMenu();
          void drawEditMenu();
          void drawProjectMenu();
          void drawObjectMenu();
          void drawToolsMenu();
          void drawWindowsMenu();
          void drawHelpMenu();
      protected:
        bool _isMainMenu;
    };
}; //namespace Divide

#endif //_EDITOR_MENU_BAR_H_