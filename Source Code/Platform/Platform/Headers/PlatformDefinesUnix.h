/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _PLATFORM_DEFINES_UNIX_H_
#define _PLATFORM_DEFINES_UNIX_H_

//#pragma GCC diagnostic ignored "-Wall"

#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX

#ifndef NOINITVTABLE
#define NOINITVTABLE
#endif  //NOINITVTABLE

#ifndef THREAD_LOCAL
#define THREAD_LOCAL __thread
#endif  //THREAD_LOCAL

#include <X11/Xlib.h>
void checkX11Events();

namespace Divide {
    struct SysInfo {
        SysInfo() : _windowHandle(0),
                    _availableRam(0),
                    _systemResolutionWidth(0),
                    _systemResolutionHeight(0)
        {
        }

        Window _windowHandle;
        size_t _availableRam;
        int _systemResolutionWidth;
        int _systemResolutionHeight;
    };

    typedef timeVal TimeValue;
}; //namespace Divide

#endif //_PLATFORM_DEFINES_UNIX_H_