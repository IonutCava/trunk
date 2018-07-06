#include "stdafx.h"

#include "Headers/CEGUIInput.h"

#if !defined(CEGUI_STATIC)
#define CEGUI_STATIC
#endif
#include <CEGUI/CEGUI.h>

#include "GUI/Headers/GUI.h"

namespace Divide {

CEGUIInput::CEGUIInput(GUI& parent)
    : _parent(parent)
{
}

// return true if the input was consumed
bool CEGUIInput::injectOISKey(bool pressed, const Input::KeyEvent& inKey) {
    
    bool consumed = false;
    if (pressed) {
        if (CEGUI_DEFAULT_CTX.injectKeyDown((CEGUI::Key::Scan)inKey._key)) {
            CEGUI_DEFAULT_CTX.injectChar((CEGUI::Key::Scan)inKey._text);
            begin(inKey);
            consumed = true;
        }
    } else {
        if (CEGUI_DEFAULT_CTX.injectKeyUp((CEGUI::Key::Scan)inKey._key)) {
            end(inKey);
            consumed = true;
        }
    }
    return consumed;
}

void CEGUIInput::repeatKey(I32 inKey, U32 Char) {
    // Now remember the key is still down, so we need to simulate the key being
    // released, and then repressed immediatly
    CEGUI_DEFAULT_CTX.injectKeyUp((CEGUI::Key::Scan)inKey);    // Key UP
    CEGUI_DEFAULT_CTX.injectKeyDown((CEGUI::Key::Scan)inKey);  // Key Down
    CEGUI_DEFAULT_CTX.injectChar(Char);  // What that key means
}

// Return true if input was consumed
bool CEGUIInput::onKeyDown(const Input::KeyEvent& key) {
    return injectOISKey(true, key);
}

// Return true if input was consumed
bool CEGUIInput::onKeyUp(const Input::KeyEvent& key) {
    return injectOISKey(false, key);
}

// Return true if input was consumed
bool CEGUIInput::mouseMoved(const Input::MouseEvent& arg) {
    bool wheel = CEGUI_DEFAULT_CTX.injectMouseWheelChange(to_F32(arg.Z().rel));
    bool move = CEGUI_DEFAULT_CTX.injectMousePosition(to_F32(arg.X().abs), to_F32(arg.Y().abs));
    return wheel || move;
}

// Return true if input was consumed
bool CEGUIInput::mouseButtonPressed(const Input::MouseEvent& arg,
                                    Input::MouseButton button) {
    bool consumed = false;
    switch (button) {
        case Input::MouseButton::MB_Left: {
            consumed = CEGUI_DEFAULT_CTX.injectMouseButtonDown(CEGUI::LeftButton);
        } break;
        case Input::MouseButton::MB_Middle: {
            consumed = CEGUI_DEFAULT_CTX.injectMouseButtonDown(CEGUI::MiddleButton);
        } break;
        case Input::MouseButton::MB_Right: {
            consumed = CEGUI_DEFAULT_CTX.injectMouseButtonDown(CEGUI::RightButton);
        } break;
        case Input::MouseButton::MB_Button3: {
            consumed = CEGUI_DEFAULT_CTX.injectMouseButtonDown(CEGUI::X1Button);
        } break;
        case Input::MouseButton::MB_Button4: {
            consumed = CEGUI_DEFAULT_CTX.injectMouseButtonDown(CEGUI::X2Button);
        } break;
        default: break;
    };

    return consumed;
}

// Return true if input was consumed
bool CEGUIInput::mouseButtonReleased(const Input::MouseEvent& arg,
                                     Input::MouseButton button) {
    bool consumed = false;
    switch (button) {
        case Input::MouseButton::MB_Left: {
            consumed = CEGUI_DEFAULT_CTX.injectMouseButtonUp(CEGUI::LeftButton);
        } break;
        case Input::MouseButton::MB_Middle: {
            consumed = CEGUI_DEFAULT_CTX.injectMouseButtonUp(CEGUI::MiddleButton);
        } break;
        case Input::MouseButton::MB_Right: {
            consumed = CEGUI_DEFAULT_CTX.injectMouseButtonUp(CEGUI::RightButton);
        } break;
        case Input::MouseButton::MB_Button3: {
            consumed = CEGUI_DEFAULT_CTX.injectMouseButtonUp(CEGUI::X1Button);
        } break;
        case Input::MouseButton::MB_Button4: {
            consumed = CEGUI_DEFAULT_CTX.injectMouseButtonUp(CEGUI::X2Button);
        } break;
        default: break;
    };

    return consumed;
}

// Return true if input was consumed
bool CEGUIInput::joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis) {
    bool consumed = false;

    return consumed;
}

// Return true if input was consumed
bool CEGUIInput::joystickPovMoved(const Input::JoystickEvent& arg, I8 pov) {
    bool consumed = false;

    return consumed;
}

// Return true if input was consumed
bool CEGUIInput::joystickButtonPressed(const Input::JoystickEvent& arg,
                                       Input::JoystickButton button) {
    bool consumed = false;

    return consumed;
}

// Return true if input was consumed
bool CEGUIInput::joystickButtonReleased(const Input::JoystickEvent& arg,
                                        Input::JoystickButton button) {
    bool consumed = false;

    return consumed;
}

// Return true if input was consumed
bool CEGUIInput::joystickSliderMoved(const Input::JoystickEvent& arg,
                                     I8 index) {
    bool consumed = false;

    return consumed;
}

// Return true if input was consumed
bool CEGUIInput::joystickvector3Moved(const Input::JoystickEvent& arg,
                                       I8 index) {
    bool consumed = false;

    return consumed;
}
};