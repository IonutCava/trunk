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

#ifndef _GUI_TEXT_H_
#define _GUI_TEXT_H_
#include "GUIElement.h"
#include "Utility/Headers/TextLabel.h"

namespace Divide {

struct GUITextBatchEntry {
    GUITextBatchEntry()
        : _textLabel(nullptr),
          _stateHash(0)
    {
    }

    GUITextBatchEntry(const TextLabel *textLabel,
                      const vec2<F32>& position,
                      size_t stateHash)
        : _textLabel(textLabel),
          _position(position),
          _stateHash(stateHash)
    {
    }

    const TextLabel *_textLabel = 0;
    vec2<F32> _position;
    size_t _stateHash;
};

class GUIText : public GUIElement, public TextLabel {
    friend class GUIInterface;

   public:
    GUIText(U64 guiID,
            const stringImpl& text,
            const vec2<F32>& relativePosition,
            const stringImpl& font,
            const vec3<F32>& colour,
            CEGUI::Window* parent,
            U32 fontSize = 16);

    void draw(GFXDevice& context) const;
    void mouseMoved(const GUIEvent& event);
    void onMouseUp(const GUIEvent& event);
    void onMouseDown(const GUIEvent& event);
    void onChangeResolution(U16 w, U16 h);
    vec2<F32> getPosition() const;
protected:
    inline void initialHeightCache(F32 height) { _heightCache = height; }

private:
    F32 _heightCache;
    vec2<F32> _position;
};

};  // namespace Divide

#endif