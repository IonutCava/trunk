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

#ifndef _CORE_WINDOW_MANAGER_INL_
#define _CORE_WINDOW_MANAGER_INL_

namespace Divide {

inline const vec2<U16>& WindowManager::getResolution() const {
    return _windowDimensions[to_uint(WindowType::WINDOW)];
}

inline const vec2<U16>& WindowManager::getPreviousResolution() const {
    return _prevResolution;
}

inline void WindowManager::setResolutionWidth(U16 w) {
    _prevResolution.set(_windowDimensions[to_uint(WindowType::WINDOW)]);
    _windowDimensions[to_uint(WindowType::WINDOW)].width = w;
}

inline void WindowManager::setResolutionHeight(U16 h) {
    _prevResolution.set(_windowDimensions[to_uint(WindowType::WINDOW)]);
    _windowDimensions[to_uint(WindowType::WINDOW)].height = h;
}

inline void WindowManager::setResolution(const vec2<U16>& resolution) {
    _prevResolution.set(_windowDimensions[to_uint(WindowType::WINDOW)]);
    _windowDimensions[to_uint(WindowType::WINDOW)].set(resolution);
}

inline void WindowManager::setWindowDimensions(WindowType windowType, const vec2<U16>& dimensions) {
    _windowDimensions[to_uint(windowType)].set(dimensions);
}

inline const vec2<U16>& WindowManager::getWindowDimensions() const {
    return _windowDimensions[to_uint(mainWindowType())];
}

inline const vec2<U16>& WindowManager::getWindowDimensions(WindowType windowType) const {
    return _windowDimensions[to_uint(windowType)];
}

inline void WindowManager::setWindowPosition(const vec2<U16>& position) {
    _windowPosition.set(position);
}

inline const vec2<U16>& WindowManager::getWindowPosition() const {
    return _windowPosition;
}

inline bool WindowManager::hasFocus() const {
    return _hasFocus;
}

inline void WindowManager::hasFocus(const bool state) {
    _hasFocus = state;
}

inline bool WindowManager::minimized() const {
    return _minimized;
}

inline void WindowManager::minimized(const bool state) {
    _minimized = state;
}

inline I32 WindowManager::targetDisplay() const {
    return _displayIndex;
}

inline void WindowManager::targetDisplay(I32 displayIndex) {
    _displayIndex = displayIndex;
}

inline WindowType WindowManager::mainWindowType() const {
    return _activeWindowType;
}

inline void WindowManager::mainWindowType(WindowType type) {
    _activeWindowType = type;
}

}; //namespace Divide

#endif //_CORE_WINDOW_MANAGER_INL_
