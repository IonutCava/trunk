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

#ifndef _GUI_BUTTON_H_
#define _GUI_BUTTON_H_

#include "GUIElement.h"
#include "GUIText.h"

namespace CEGUI {
class Window;
class Font;
class EventArgs;
};

namespace Divide {

class AudioDescriptor;
TYPEDEF_SMART_POINTERS_FOR_CLASS(AudioDescriptor);

class GUIButton : public GUIElement {
    typedef DELEGATE_CBK<void, I64> ButtonCallback;
    typedef DELEGATE_CBK<void, AudioDescriptor_ptr> AudioCallback;

    friend class GUIInterface;
    friend class SceneGUIElements;

   public:
    void setTooltip(const stringImpl& tooltipText);
    void setText(const stringImpl& text);
    void setFont(const stringImpl& fontName, const stringImpl& fontFileName, U32 size);
    void setActive(const bool active) override;
    void setVisible(const bool visible) override;
    void setOnClickSound(const AudioDescriptor_ptr& onClickSound);

    // return false if we replace an existing callback
    static bool soundCallback(const AudioCallback& cbk);

   protected:
    GUIButton(U64 guiID,
              const stringImpl& text,
              const stringImpl& guiScheme, 
              const RelativePosition2D& offset,
              const RelativeScale2D& size,
              CEGUI::Window* parent,
              ButtonCallback callback,
              AudioDescriptor_ptr onClickSound);
    ~GUIButton();

    bool joystickButtonPressed(const CEGUI::EventArgs& /*e*/);

   protected:;
    /// A pointer to a function to call if the button is pressed
    ButtonCallback _callbackFunction;
    CEGUI::Window* _btnWindow;
    AudioDescriptor_ptr _onClickSound;

    static AudioCallback _soundCallback;
};

};  // namespace Divide

#endif