/*
Copyright (c) 2018 DIVIDE-Studio
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

    inline bool DisplayWindow::fullscreen() const {
        return type() == WindowType::FULLSCREEN ||
               type() == WindowType::FULLSCREEN_WINDOWED;
    }

    inline void DisplayWindow::setPosition(I32 positionX, I32 positionY) {
        _windowPosition.set(positionX, positionY);
        setPositionInternal(positionX, positionY);
    }

    inline void DisplayWindow::setPosition(const vec2<I32>& position) {
        setPosition(position.x, position.y);
    }

    inline const vec2<I32>& DisplayWindow::getPosition() const {
        return _windowPosition;
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

    inline void DisplayWindow::clearColour(const FColour& colour, bool force) {
        _clearColour.set(colour);
        if (force) {
            _clearColourOriginal.set(colour);
        }
    }

    inline FColour& DisplayWindow::clearColour() {
        return _clearColour;
    }

    inline const FColour& DisplayWindow::clearColour() const {
        return _clearColour;
    }

    inline FColour& DisplayWindow::originalClearColour() {
        return _clearColourOriginal;
    }

    inline const FColour& DisplayWindow::originalClearColour() const {
        return _clearColourOriginal;
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

    inline void DisplayWindow::changeType(WindowType newType) {
        if (newType != _type) {
            _queuedType = newType;
            handleChangeWindowType(_queuedType);
        }
    }

    inline void DisplayWindow::changeToPreviousType() {
        changeType(_previousType);
    }

    inline const stringImpl& DisplayWindow::title() const {
        return _title;
    }

    inline const WindowHandle& DisplayWindow::handle() const { 
        return _handle;
    }

    inline I64 DisplayWindow::addEventListener(WindowEvent windowEvent, const EventListener& listener) {
        EventListeners& listeners = _eventListeners[to_base(windowEvent)];
        listeners.emplace_back(std::make_shared<GUID_DELEGATE_CBK<void, WindowEventArgs>>(listener));
        return listeners.back()->getGUID();
    }

    inline void DisplayWindow::removeEventlistener(WindowEvent windowEvent, I64 listenerGUID) {
        EventListeners& listeners = _eventListeners[to_base(windowEvent)];
        listeners.erase(
            std::find_if(std::begin(listeners), std::end(listeners),
                           [&listenerGUID](const std::shared_ptr<GUID_DELEGATE_CBK<void, WindowEventArgs>>& it)
                           -> bool { 
                                return it->getGUID() == listenerGUID;
                            }),
            std::end(listeners));
    }

    inline Input::InputInterface& DisplayWindow::inputHandler() {
        return *_inputHandler;
    }

    inline void DisplayWindow::destroyCbk(const DELEGATE_CBK<void>& destroyCbk) {
        _destroyCbk = destroyCbk;
    }

    inline bool DisplayWindow::warp() const {
        return _warp;
    }

    inline const Rect<I32>& DisplayWindow::warpRect() const {
        return _warpRect;
    }

    inline const Rect<I32>& DisplayWindow::renderingViewport() const {
        return _renderingViewport;
    }
}; //namespace Divide

#endif //_DISPLAY_WINDOW_INL_