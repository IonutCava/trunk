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

#ifndef _CORE_WINDOW_MANAGER_H_
#define _CORE_WINDOW_MANAGER_H_

#include "Core/Math/Headers/MathMatrices.h"

namespace Divide {

enum class WindowType : U32 {
    WINDOW = 0,
    SPLASH = 1,
    FULLSCREEN = 2,
    FULLSCREEN_WINDOWED = 3,
    COUNT
};

enum class WindowEvent : U32 {
    HIDDEN = 0,
    SHOWN = 1,
    MINIMIZED = 2,
    MAXIMIZED = 3,
    LOST_FOCUS = 4,
    GAINED_FOCUS = 5,
    RESIZED_INTERNAL = 6,
    RESIZED_EXTERNAL = 7,
    MOVED = 8
};

class WindowManager {
public:
    WindowManager();

    inline bool hasFocus() const;
    inline void hasFocus(const bool state);

    inline WindowType mainWindowType() const;
    inline void mainWindowType(WindowType type);

    inline I32 targetDisplay() const;
    inline void targetDisplay(I32 displayIndex);

    /// Application resolution (either fullscreen resolution or window dimensions)
    inline const vec2<U16>& getResolution() const;
    inline const vec2<U16>& getPreviousResolution() const;

    inline void setResolutionWidth(U16 w);
    inline void setResolutionHeight(U16 h);
    inline void setResolution(const vec2<U16>& resolution);

    inline void setWindowDimensions(WindowType windowType, const vec2<U16>& dimensions);
    inline const vec2<U16>& getWindowDimensions() const;
    inline const vec2<U16>& getWindowDimensions(WindowType windowType) const;

    inline void setWindowPosition(const vec2<U16>& position);
    inline const vec2<U16>& getWindowPosition() const;

protected:
    /// this is false if the window/application lost focus (e.g. clicked another
    /// window, alt + tab, etc)
    bool _hasFocus;
    I32  _displayIndex;
    WindowType _activeWindowType;
    vec2<U16> _prevResolution;
    vec2<U16> _windowPosition;

    std::array<vec2<U16>, to_const_uint(WindowType::COUNT)> _windowDimensions;
};
}; //namespace Divide
#endif //_CORE_WINDOW_MANAGER_H_

#include "WindowManager.inl"