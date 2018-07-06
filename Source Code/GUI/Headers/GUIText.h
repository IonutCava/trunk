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

#ifndef _GUI_TEXT_H_
#define _GUI_TEXT_H_
#include "GUIElement.h"
#include "Utility/Headers/TextLabel.h"
   
class GUIText : public GUIElement, public TextLabel {
friend class GUI;
public:
    GUIText(const std::string& id,
            const std::string& text,
            const vec2<I32>& position,
            const std::string& font,
            const vec3<F32>& color,
            CEGUI::Window* parent,
            U32 textHeight = 16) : GUIElement(parent,GUI_TEXT,position), 
                                   TextLabel(text, font, color, textHeight)
    {
    }
    void mouseMoved(const GUIEvent &event);
    void onMouseUp(const GUIEvent &event);
    void onMouseDown(const GUIEvent &event);
    void onResize(const vec2<I32>& newSize);
/*  void onRightMouseUp(const GUIEvent &event);
    void onRightMouseDown(const GUIEvent &event);
    bool onKeyUp(const GUIEvent &event);
    bool onKeyDown(const GUIEvent &event);
*/
};

#endif