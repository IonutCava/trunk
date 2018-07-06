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

#ifndef _GUI_TEXT_H_
#define _GUI_TEXT_H_

#include "GUIElement.h"
#include "Utility/Headers/TextLabel.h"

namespace Divide {

class GUIText : public GUIElement, public TextElement {
    friend class GUIInterface;

   public:
    GUIText(U64 guiID,
            const stringImpl& name,
            const stringImpl& text,
            const RelativePosition2D& relativePosition,
            const stringImpl& font,
            const UColour& colour,
            CEGUI::Window* parent,
            U8 fontSize = 16u);

    // Return true if input was consumed
    bool mouseMoved(const GUIEvent& event) override;
    // Return true if input was consumed
    bool onMouseUp(const GUIEvent& event) override;
    // Return true if input was consumed
    bool onMouseDown(const GUIEvent& event) override;

    const RelativePosition2D& getPosition() const;
};

};  // namespace Divide

#endif