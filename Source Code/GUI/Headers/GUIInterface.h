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

#ifndef _GUI_INTERFACE_H_
#define _GUI_INTERFACE_H_

#include "GUIElement.h"

namespace Divide {

class GUIText;
class GUIFlash;
class GUIButton;
class GUIMessageBox;

class GUIInterface {
public:
    typedef hashMapImpl<U64, std::pair<GUIElement*, bool/*last state*/>> GUIMap;
    typedef DELEGATE_CBK_PARAM<I64> ButtonCallback;

public:
    explicit GUIInterface(const vec2<U16>& resolution);
    virtual ~GUIInterface();

    virtual void onChangeResolution(U16 w, U16 h);
    virtual const vec2<U16>& getDisplayResolution() const;

    /// Get a pointer to an element by name/id
    template<typename T = GUIElement>
    inline T* getGUIElement(U64 elementName) const {
        static_assert(std::is_base_of<GUIElement, T>::value,
            "getGuiElement error: Target is not a valid GUI item");

        return static_cast<T*>(getGUIElementImpl(elementName));
    }

    template<typename T = GUIElement>
    inline T* getGUIElement(I64 elementID) const {
        static_assert(std::is_base_of<GUIElement, T>::value,
            "getGuiElement error: Target is not a valid GUI item");

        return static_cast<T*>(getGUIElementImpl(elementID));
    }

    virtual GUIText* addText(U64 guiID,
                             const vec2<I32>& position,
                             const stringImpl& font,
                             const vec4<U8>& colour,
                             const stringImpl& text,
                             U32 fontSize = 16);

    virtual GUIText* modifyText(U64 guiID,
                                const stringImpl& text);

    virtual GUIMessageBox* addMsgBox(U64 guiID,
                                     const stringImpl& title,
                                     const stringImpl& message,
                                     const vec2<I32>& offsetFromCentre = vec2<I32>(0));

    virtual GUIButton* addButton(U64 guiID,
                                 const stringImpl& text,
                                 const vec2<I32>& position,
                                 const vec2<U32>& dimensions,
                                 ButtonCallback callback,
                                 const stringImpl& rootSheetID = "");

    virtual GUIFlash* addFlash(U64 guiID,
                               stringImpl movie,
                               const vec2<U32>& position,
                               const vec2<U32>& extent);

    virtual void mouseMoved(const GUIEvent& event);
    virtual void onMouseUp(const GUIEvent& event);
    virtual void onMouseDown(const GUIEvent& event);

protected:
    void addElement(U64 id, GUIElement* element);
    virtual GUIElement* getGUIElementImpl(U64 elementName) const;
    virtual GUIElement* getGUIElementImpl(I64 elementID) const;

protected:
    GUIMap _guiElements;

    vec2<U16> _resolutionCache;
};

}; //namespace Divide
#endif //_GUI_INTERFACE_H_
