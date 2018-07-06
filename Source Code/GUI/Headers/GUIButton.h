/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _GUI_BUTTON_H_
#define _GUI_BUTTON_H_

#include "GUIElement.h"
#include "GUIText.h"

namespace CEGUI{
    class Window;
    class Font;
    class EventArgs;
};

class GUIButton : public GUIElement {
typedef DELEGATE_CBK ButtonCallback;
friend class GUI;
protected:
    GUIButton(const std::string& id,
              const std::string& text,
              const std::string& guiScheme,
              const vec2<I32>& position,
              const vec2<U32>& dimensions,
              const vec3<F32>& color,
              CEGUI::Window* parent,
              ButtonCallback callback);
    ~GUIButton();

    void setTooltip(const std::string& tooltipText);
    void setFont(const std::string& fontName, const std::string& fontFileName, U32 size);
    bool joystickButtonPressed(const CEGUI::EventArgs& /*e*/);

protected:
    std::string _text;
    vec2<U32>	_dimensions;
    vec3<F32>	_color;
    bool		_pressed;
    bool		_highlight;
    ButtonCallback _callbackFunction;	/* A pointer to a function to call if the button is pressed */
    CEGUI::Window *_btnWindow;
};
#endif