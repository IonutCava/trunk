/*
   Copyright (c) 2016 DIVIDE-Studio
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

#pragma warning(disable : 4127)  //< Constant conditional expressions
#pragma warning(disable : 4201)  //< nameless struct

#ifndef USE_VLD
//#define USE_VLD
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif //_CRT_SECURE_NO_WARNINGS

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

#ifndef _RESTRICT_
#define _RESTRICT_ __restrict
#endif

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

#if defined(CPP_VERSION)
#   undef CPP_VERSION
#   if _MSC_VER == 1600 || _MSC_VER == 1700
#       define CPP_VERSION 1
#   elif _MSC_VER == 1800
#       define CPP_VERSION 201103L
#   elif _MSC_VER == 1900
#       define CPP_VERSION 201402L
#   elif _MSC_VER > 1900
#       define CPP_VERSION 201500 //This may change for C++17
#   else
#       define CPP_VERSION 0
#   endif
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
}; //namespace Divide


#endif //_PLATFORM_DEFINES_WINDOWS_H_
