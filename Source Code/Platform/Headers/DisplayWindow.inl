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

#ifndef _DISPLAY_WINDOW_INL_
#define _DISPLAY_WINDOW_INL_

namespace Divide {

    inline SDL_Window* DisplayWindow::getRawWindow() const {
        return _mainWindow;
    }

    inline const vec2<U16>& DisplayWindow::getPreviousDimensions(WindowType windowType) const {
        return _prevDimensions[to_const_uint(windowType)];
    }

    inline void DisplayWindow::setDimensions(WindowType windowType, U16 dimensionX, U16 dimensionY) {
        _prevDimensions[to_const_uint(type())].set(_windowDimensions[to_const_uint(type())]);
        _windowDimensions[to_const_uint(type())].set(dimensionX, dimensionY);

        if (windowType == type()) {
            setDimensionsInternal(dimensionX, dimensionY);
        }
    }

    inline void DisplayWindow::setDimensions(WindowType windowType, const vec2<U16>& dimensions) {
        setDimensions(windowType, dimensions.x, dimensions.y);
    }

    inline const vec2<U16>& DisplayWindow::getDimensions(WindowType windowType) const {
        return _windowDimensions[to_uint(windowType)];
    }

    inline const vec2<U16>& DisplayWindow::getDimensions() const {
        return getDimensions(type());
    }

    inline const vec2<U16>& DisplayWindow::getPreviousDimensions() const {
        return getPreviousDimensions(type());
    }

    inline void DisplayWindow::setPosition(WindowType windowType, I32 positionX, I32 positionY) {
        _windowPosition[to_const_uint(windowType)].set(positionX, positionY);

        if (windowType == type()) {
            setPositionInternal(positionX, positionY);
        }
    }

    inline void DisplayWindow::setPosition(WindowType windowType, const vec2<I32>& position) {
        setPosition(windowType, position.x, position.y);
    }

    inline const vec2<I32>& DisplayWindow::getPosition(WindowType windowType) const {
        return _windowPosition[to_const_uint(windowType)];
    }

    inline bool DisplayWindow::hasFocus() const {
        return _hasFocus;
    }

    inline void DisplayWindow::hasFocus(const bool state) {
        _hasFocus = state;
    }

    inline bool DisplayWindow::minimized() const {
        return _minimized;
    }

    inline void DisplayWindow::minimized(const bool state) {
        _minimized = state;
    }

    inline bool DisplayWindow::hidden() const {
        return _hidden;
    }

    inline void DisplayWindow::hidden(const bool state) {
        _hidden = state;
    }

    inline WindowType DisplayWindow::type() const {
        return _type;
    }

    inline void DisplayWindow::type(WindowType type) {
        if (type != _type) {
            _queuedType = type;
            handleChangeWindowType(_queuedType);
        }
    }

    inline void DisplayWindow::previousType() {
        type(_previousType);
    }

}; //namespace Divide

#endif //_DISPLAY_WINDOW_INL_