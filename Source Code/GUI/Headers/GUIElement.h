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

#ifndef _GUI_ELEMENT_H_
#define _GUI_ELEMENT_H_

#include "Core/Math/Headers/MathMatrices.h"

namespace CEGUI {
    class Window;
};

namespace Divide {

namespace GFX {
    class CommandBuffer;
};

enum class GUIType : U32 {
    GUI_TEXT = 0,
    GUI_BUTTON = 1,
    GUI_FLASH = 2,
    GUI_CONSOLE = 3,
    GUI_MESSAGE_BOX = 4,
    GUI_CONFIRM_DIALOG = 5,
    COUNT
};

struct GUIEvent {
    /// ascii character code 'a', 'A', 'b', '*', etc (if device==keyboard) -
    /// possibly a uchar or something
    U16 ascii;
    /// SI_LSHIFT, etc
    U8 modifier;
    /// for unprintables, 'tab', 'return', ...
    U16 keyCode;
    /// for mouse events
    vec2<F32> mousePoint;
    U8 mouseClickCount;
};

template<typename T = GUIElement>
GUIType getTypeEnum() {
    static_assert(std::is_base_of<GUIElement, T>::value,
        "getGuiElement error: Target is not a valid GUI item");

    if (std::is_same<T, GUIText>::value) {
        return GUIType::GUI_TEXT;
    } else if (std::is_same < T, GUIButton>::value) {
        return GUIType::GUI_BUTTON;
    } else if (std::is_same < T, GUIFlash>::value) {
        return GUIType::GUI_FLASH;
    } else if (std::is_same < T, GUIConsole>::value) {
        return GUIType::GUI_CONSOLE;
    } else if (std::is_same < T, GUIMessageBox>::value) {
        return GUIType::GUI_MESSAGE_BOX;
    } 
    //else if (std::is_same < T, GUIConfirmDialog::value) {
    //    return GUIType::GUI_CONFIRM_DIALOG;
    //}

    return GUIType::COUNT;
}

class GFXDevice;
class RenderStateBlock;
struct SizeChangeParams;
class GUIElement : public GUIDWrapper {
    friend class GUI;

   public:
    GUIElement(U64 guiID, CEGUI::Window* const parent, const GUIType& type);
    virtual ~GUIElement();

    virtual void draw(GFXDevice& context, GFX::CommandBuffer& bufferInOut) const = 0;

    inline const stringImpl& getName() const { return _name; }
    inline const GUIType getType() const { return _guiType; }
    inline const bool isActive() const { return _active; }
    virtual const bool isVisible() const { return _visible; }

    inline  void setName(const stringImpl& name) { _name = name; }
    virtual void setVisible(const bool visible) { _visible = visible; }
    virtual void setActive(const bool active) { _active = active; }

    inline void addChildElement(GUIElement* child) {}

    virtual void setTooltip(const stringImpl& tooltipText) {}

    // Return true if input was consumed
    virtual bool mouseMoved(const GUIEvent& event) { return false; }
    // Return true if input was consumed
    virtual bool onMouseUp(const GUIEvent& event) { return false; }
    // Return true if input was consumed
    virtual bool onMouseDown(const GUIEvent& event) { return false; }

    virtual void onSizeChange(const SizeChangeParams& params) {
        ACKNOWLEDGE_UNUSED(params);
    }

   protected:
    GUIType _guiType;
    CEGUI::Window* _parent;

   private:
    stringImpl _name;
    bool _visible;
    bool _active;
    U64  _guiID;
};

};  // namespace Divide

#endif