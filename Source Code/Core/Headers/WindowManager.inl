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

#ifndef _CORE_WINDOW_MANAGER_INL_
#define _CORE_WINDOW_MANAGER_INL_

namespace Divide {

inline I32 WindowManager::targetDisplay() const {
    return _displayIndex;
}

inline void WindowManager::targetDisplay(I32 displayIndex) {
    _displayIndex = displayIndex;
}

inline DisplayWindow& WindowManager::getWindow(I64 guid) {
    for (DisplayWindow* win : _windows) {
        if (win->getGUID() == guid) {
            return *win;
        }
    }

    return *_windows.front();
}

inline const DisplayWindow& WindowManager::getWindow(I64 guid) const {
    for (const DisplayWindow* win : _windows) {
        if (win->getGUID() == guid) {
            return *win;
        }
    }

    return *_windows.front();
}

inline DisplayWindow& WindowManager::getActiveWindow() {
    return getWindow(_activeWindowGUID);
}

inline const DisplayWindow& WindowManager::getActiveWindow() const {
    return getWindow(_activeWindowGUID);
}

}; //namespace Divide

#endif //_CORE_WINDOW_MANAGER_INL_
