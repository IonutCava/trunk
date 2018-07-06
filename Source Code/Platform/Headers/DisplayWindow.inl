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

#ifndef _DISPLAY_WINDOW_INL_
#define _DISPLAY_WINDOW_INL_

namespace Divide {

    inline SDL_Window* DisplayWindow::getRawWindow() const {
        return _sdlWindow;
    }

    inline const vec2<U16>& DisplayWindow::getPreviousDimensions(WindowType windowType) const {
        return _prevDimensions[to_base(windowType)];
    }

    inline void DisplayWindow::setDimensions(WindowType windowType, U16 dimensionX, U16 dimensionY) {
        _prevDimensions[to_base(type())].set(_windowDimensions[to_base(type())]);
        _windowDimensions[to_base(type())].set(dimensionX, dimensionY);

        if (windowType == type()) {
            setDimensionsInternal(dimensionX, dimensionY);
        }
    }

    inline void DisplayWindow::setDimensions(WindowType windowType, const vec2<U16>& dimensions) {
        setDimensions(windowType, dimensions.x, dimensions.y);
    }

    inline void DisplayWindow::setDimensions(U16 dimensionX, U16 dimensionY) {
        setDimensions(type(), dimensionX, dimensionY);
    }

    inline const vec2<U16>& DisplayWindow::getDimensions(WindowType windowType) const {
        return _windowDimensions[to_U32(windowType)];
    }

    inline const vec2<U16>& DisplayWindow::getDimensions() const {
        return getDimensions(type());
    }

    inline const vec2<U16>& DisplayWindow::getPreviousDimensions() const {
        return getPreviousDimensions(type());
    }

    inline void DisplayWindow::setPosition(WindowType windowType, I32 positionX, I32 positionY) {
        _windowPosition[to_base(windowType)].set(positionX, positionY);

        if (windowType == type()) {
            setPositionInternal(positionX, positionY);
        }
    }

    inline void DisplayWindow::setPosition(WindowType windowType, const vec2<I32>& position) {
        setPosition(windowType, position.x, position.y);
    }

    inline void DisplayWindow::setPosition(I32 positionX, I32 positionY) {
        setPosition(type(), positionX, positionY);
    }

    inline const vec2<I32>& DisplayWindow::getPosition(WindowType windowType) const {
        return _windowPosition[to_base(windowType)];
    }

    inline const vec2<I32>& DisplayWindow::getPosition() const {
        return getPosition(type());
    }

    inline bool DisplayWindow::swapBuffers() const {
        return _swapBuffers;
    }

    inline void DisplayWindow::swapBuffers(const bool state) {
        _swapBuffers = state;
    }

    inline bool DisplayWindow::hasFocus() const {
        return _hasFocus;
    }

    inline void DisplayWindow::hasFocus(const bool state) {
        _hasFocus = state;
    }

    inline U8 DisplayWindow::opacity() const {
        return _opacity;
    }

    inline bool DisplayWindow::minimized() const {
        return _minimized;
    }

    inline bool DisplayWindow::maximized() const {
        return _maximized;
    }


    inline bool DisplayWindow::hidden() const {
        return _hidden;
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

    inline const stringImpl& DisplayWindow::title() const {
        return _title;
    }

    inline const WindowHandle& DisplayWindow::handle() const { 
        return _handle;
    }

    inline void DisplayWindow::addEventListener(WindowEvent windowEvent, const EventListener& listener) {
        _eventListeners[to_base(windowEvent)].push_back(listener);
    }
}; //namespace Divide

#endif //_DISPLAY_WINDOW_INL_