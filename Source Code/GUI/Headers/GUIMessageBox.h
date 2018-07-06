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

#ifndef _GUI_MESSAGE_BOX_H_
#define _GUI_MESSAGE_BOX_H_

#include "GUIElement.h"
#include "GUIText.h"

namespace CEGUI {
class Window;
class Font;
class EventArgs;
};

namespace Divide {

class GUIMessageBox : public GUIElement {
    friend class GUIInterface;
    friend class SceneGUIElements;

   public:
    enum class MessageType : U32 {
        MESSAGE_INFO = 0,
        MESSAGE_WARNING = 1,
        MESSAGE_ERROR = 2
    };

    bool onConfirm(const CEGUI::EventArgs& /*e*/);
    void setTitle(const stringImpl& titleText);
    void setMessage(const stringImpl& message);
    void setOffset(const vec2<I32>& offsetFromCentre);
    void setMessageType(MessageType type);
    inline void show() {
        setActive(true);
        setVisible(true);
    }

   protected:
    GUIMessageBox(U64 guiID,
                  const stringImpl& name,
                  const stringImpl& title,
                  const stringImpl& message,
                  const vec2<I32>& offsetFromCentre = vec2<I32>(0),
                  CEGUI::Window* parent = nullptr);
    ~GUIMessageBox();

    void setVisible(const bool visible);
    void setActive(const bool active);

   protected:
    vec2<I32> _offsetFromCentre;
    CEGUI::Window* _msgBoxWindow;
};

};  // namespace Divide

#endif