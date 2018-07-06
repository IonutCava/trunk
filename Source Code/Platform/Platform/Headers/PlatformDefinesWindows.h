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

#ifndef _PLATFORM_DEFINES_WINDOWS_H_
#define _PLATFORM_DEFINES_WINDOWS_H_

/// Reduce Build time on Windows Platform
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif  // VC_EXTRALEAN
#endif  // WIN32_LEAN_AND_MEAN

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL

//#define GLFW_EXPOSE_NATIVE_EG
#include <windows.h>
#ifdef DELETE
#undef DELETE
#endif

#if defined(_WIN64)
#define WIN64
#else
#define WIN32
#endif

LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

namespace Divide {
    struct SysInfo {
        SysInfo() : _windowHandle(0),
                    _availableRam(0)
        {
        }

        HWND _windowHandle;
        size_t _availableRam;
    };

    typedef LONGLONG TimeValue;
}; //namespace Divide


#endif //_PLATFORM_DEFINES_WINDOWS_H_