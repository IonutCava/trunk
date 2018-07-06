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

/// Comment this out to show the debug console
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")

#pragma warning(disable : 4127)  //< Constant conditional expressions
#pragma warning(disable : 4201)  //< nameless struct

/// Reduce Build time on Windows Platform
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif  // WIN32_LEAN_AND_MEAN

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif  //VC_EXTRALEAN

#ifndef NOMINMAX
#define NOMINMAX
#endif  //NOMINMAX

#ifndef NOINITVTABLE
#define NOINITVTABLE __declspec(novtable)
#endif  //NOINITVTABLE

#ifndef FORCE_INLINE
#define FORCE_INLINE __forceinline
#endif //FORCE_INLINE

#include <windows.h>
#include <limits.h>

#ifdef DELETE
#undef DELETE
#endif

#if defined(_WIN64)
#define WIN64
#else
#define WIN32
#endif

#ifdef strdup
#define strdup _strdup
#endif

LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#ifndef thread_local
#define thread_local __declspec( thread )
#endif //thread_local

namespace Divide {
    struct SysInfo {
        SysInfo() : _windowHandle(0),
                    _availableRam(0),
                    _systemResolutionWidth(0),
                    _systemResolutionHeight(0)
        {
        }

        HWND _windowHandle;
        size_t _availableRam;
        int _systemResolutionWidth;
        int _systemResolutionHeight;
    };

    typedef LONGLONG TimeValue;
}; //namespace Divide


#endif //_PLATFORM_DEFINES_WINDOWS_H_
