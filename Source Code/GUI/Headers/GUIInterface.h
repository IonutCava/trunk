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

#pragma once
#ifndef _GUI_INTERFACE_H_
#define _GUI_INTERFACE_H_

#include "GUIElement.h"

namespace CEGUI {
    class String;
};

namespace Divide {

class GUIText;
class GUIFlash;
class GUIButton;
class GUIMessageBox;

class GUIInterface {
public:
    typedef hashMap<U64, std::pair<GUIElement*, bool/*last state*/>> GUIMap;
    typedef DELEGATE<void, I64> ButtonCallback;

public:
    explicit GUIInterface(GUI& context);
    virtual ~GUIInterface();

    virtual void onLanguageChange(const char* newLanguage);

    inline GUI& getParentContext() { return *_context; }
    inline const GUI& getParentContext() const { return *_context; }
    /// Get a pointer to an element by name/id
    template <typename T>
    typename std::enable_if<std::is_base_of<GUIElement, T>::value, T*>::type
    getGUIElement(U64 elementName) const {
        return static_cast<T*>(getGUIElementImpl(elementName, T::Type));
    }

    template <typename T>
    typename std::enable_if<std::is_base_of<GUIElement, T>::value, T*>::type
    getGUIElement(I64 elementID) const {
        return static_cast<T*>(getGUIElementImpl(elementID, T::Type));
    }

    virtual GUIText* addText(const char* name,
                             const RelativePosition2D& position,
                             const stringImpl& font,
                             const UColour4& colour,
                             const stringImpl& text,
                             bool multiLine = false,
                             U8 fontSize = 16u);

    virtual GUIText* modifyText(const char* name, const stringImpl& text, bool multiLine);

    virtual GUIMessageBox* addMsgBox(const char* name,
                                     const stringImpl& title,
                                     const stringImpl& message,
                                     const vec2<I32>& offsetFromCentre = vec2<I32>(0));

    virtual GUIButton* addButton(const char* name,
                                 const stringImpl& text,
                                 const RelativePosition2D& offset,
                                 const RelativeScale2D& size,
                                 const stringImpl& rootSheetID = "");

    virtual GUIFlash* addFlash(const char* name,
                               stringImpl movie,
                               const RelativePosition2D& position,
                               const RelativeScale2D& size);

    virtual CEGUI::Window* createWindow(const CEGUI::String& type, const CEGUI::String& name);
    virtual CEGUI::Window* loadWindowFromLayoutFile(const char* layoutFileName);
    virtual bool unloadWindow(CEGUI::Window*& window);

protected:
    void addElement(U64 id, GUIElement* element);
    virtual GUIElement* getGUIElementImpl(U64 elementName, GUIType type) const;
    virtual GUIElement* getGUIElementImpl(I64 elementID, GUIType type) const;

protected:
    GUI* _context;

    std::array<GUIMap, to_base(GUIType::COUNT)> _guiElements;
};

}; //namespace Divide
#endif //_GUI_INTERFACE_H_
