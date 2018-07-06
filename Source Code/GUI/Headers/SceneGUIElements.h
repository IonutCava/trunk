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
#ifndef _SCENE_GUI_ELEMENTS_H_
#define _SCENE_GUI_ELEMENTS_H_

#include "GUI.h"
#include "GUIElement.h"
#include "Scenes/Headers/SceneComponent.h"

namespace Divide {

class SceneGUIElements : public SceneComponent {
public:
    SceneGUIElements(Scene& parentScene);
    ~SceneGUIElements();

    void draw();
    void onChangeResolution(U16 w, U16 h);

    GUIElement* get(ULL elementName) const;
    GUIElement* get(I64 elementGUID) const;

    void onEnable();
    void onDisable();

    // ToDo: Move these out to a different class (with the element map)
    // and have both this class and the GUI class inherit from it
    GUIText* addText(ULL ID,
                     const vec2<I32>& position,
                     const stringImpl& font,
                     const vec4<U8>& color,
                     const stringImpl& text,
                     U32 fontSize = 16);

    GUIText* modifyText(ULL ID,
                        const stringImpl& text);

    GUIMessageBox* addMsgBox(ULL ID,
                             const stringImpl& title,
                             const stringImpl& message,
                             const vec2<I32>& offsetFromCentre = vec2<I32>(0));
    GUIButton* addButton(ULL ID,
                         const stringImpl& text,
                         const vec2<I32>& position,
                         const vec2<U32>& dimensions,
                         GUI::ButtonCallback callback,
                         const stringImpl& rootSheetID = "");

    GUIFlash* addFlash(ULL ID,
                       stringImpl movie,
                       const vec2<U32>& position,
                       const vec2<U32>& extent);

    void mouseMoved(const GUIEvent& event);
    void onMouseUp(const GUIEvent& event);
    void onMouseDown(const GUIEvent& event);

    const vec2<U16>& getDisplayResolution() const;


    /// Get a pointer to an element by name/id
    template<typename T = GUIElement>
    inline T* getGUIElement(I64 sceneID, ULL elementName) {
        static_assert(std::is_base_of<GUIElement, T>::value,
                      "getGuiElement error: Target is not a valid GUI item");

        return static_cast<T*>(getGUIElementImpl(sceneID, elementName));
    }

    template<typename T = GUIElement>
    inline T* getGUIElement(I64 sceneID, I64 elementID) {
        static_assert(std::is_base_of<GUIElement, T>::value,
                      "getGuiElement error: Target is not a valid GUI item");

        return static_cast<T*>(getGUIElementImpl(sceneID, elementID));
    }

protected:
    void addElement(ULL id, GUIElement* element);
    GUIElement* getGUIElementImpl(I64 sceneID, ULL elementName) const;
    GUIElement* getGUIElementImpl(I64 sceneID, I64 elementID) const;

private:
    GUI::GUIMap _sceneElements;
};
}; //namespace Divide;

#endif //_SCENE_GUI_ELEMENTS_H_