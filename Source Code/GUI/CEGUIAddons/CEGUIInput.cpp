#include "Headers/CEGUIInput.h"

#include <CEGUI/CEGUI.h>
#include "GUI/Headers/GUI.h"

namespace Divide {

bool CEGUIInput::injectOISKey(bool pressed, const Input::KeyEvent& inKey) {
    if (pressed) {
        CEGUI_DEFAULT_CTX.injectKeyDown((CEGUI::Key::Scan)inKey._key);
        CEGUI_DEFAULT_CTX.injectChar((CEGUI::Key::Scan)inKey._text);
        begin(inKey);
    } else {
        CEGUI_DEFAULT_CTX.injectKeyUp((CEGUI::Key::Scan)inKey._key);
        end(inKey);
    }
    return true;
}

void CEGUIInput::repeatKey(I32 inKey, U32 Char) {
    // Now remember the key is still down, so we need to simulate the key being
    // released, and then repressed immediatly
    CEGUI_DEFAULT_CTX.injectKeyUp((CEGUI::Key::Scan)inKey);    // Key UP
    CEGUI_DEFAULT_CTX.injectKeyDown((CEGUI::Key::Scan)inKey);  // Key Down
    CEGUI_DEFAULT_CTX.injectChar(Char);  // What that key means
}

bool CEGUIInput::onKeyDown(const Input::KeyEvent& key) {
    return injectOISKey(true, key);
}

bool CEGUIInput::onKeyUp(const Input::KeyEvent& key) {
    return injectOISKey(false, key);
}

bool CEGUIInput::mouseMoved(const Input::MouseEvent& arg) {
    CEGUI_DEFAULT_CTX.injectMouseWheelChange(arg.state.Z.abs);
    CEGUI_DEFAULT_CTX.injectMouseMove(arg.state.X.rel, arg.state.Y.rel);
    return true;
}

bool CEGUIInput::mouseButtonPressed(const Input::MouseEvent& arg,
                                    Input::MouseButton button) {
    switch (button) {
        case Input::MouseButton::MB_Left: {
            CEGUI_DEFAULT_CTX.injectMouseButtonDown(CEGUI::LeftButton);
        } break;
        case Input::MouseButton::MB_Middle: {
            CEGUI_DEFAULT_CTX.injectMouseButtonDown(CEGUI::MiddleButton);
        } break;
        case Input::MouseButton::MB_Right: {
            CEGUI_DEFAULT_CTX.injectMouseButtonDown(CEGUI::RightButton);
        } break;
        case Input::MouseButton::MB_Button3: {
            CEGUI_DEFAULT_CTX.injectMouseButtonDown(CEGUI::X1Button);
        } break;
        case Input::MouseButton::MB_Button4: {
            CEGUI_DEFAULT_CTX.injectMouseButtonDown(CEGUI::X2Button);
        } break;
        default:
            return false;
    };

    return true;
}

bool CEGUIInput::mouseButtonReleased(const Input::MouseEvent& arg,
                                     Input::MouseButton button) {
    switch (button) {
        case Input::MouseButton::MB_Left: {
            CEGUI_DEFAULT_CTX.injectMouseButtonUp(CEGUI::LeftButton);
        } break;
        case Input::MouseButton::MB_Middle: {
            CEGUI_DEFAULT_CTX.injectMouseButtonUp(CEGUI::MiddleButton);
        } break;
        case Input::MouseButton::MB_Right: {
            CEGUI_DEFAULT_CTX.injectMouseButtonUp(CEGUI::RightButton);
        } break;
        case Input::MouseButton::MB_Button3: {
            CEGUI_DEFAULT_CTX.injectMouseButtonUp(CEGUI::X1Button);
        } break;
        case Input::MouseButton::MB_Button4: {
            CEGUI_DEFAULT_CTX.injectMouseButtonUp(CEGUI::X2Button);
        } break;
        default:
            return false;
    };

    return true;
}

bool CEGUIInput::joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis) {
    return true;
}

bool CEGUIInput::joystickPovMoved(const Input::JoystickEvent& arg, I8 pov) {
    return true;
}

bool CEGUIInput::joystickButtonPressed(const Input::JoystickEvent& arg,
                                       I8 button) {
    return true;
}

bool CEGUIInput::joystickButtonReleased(const Input::JoystickEvent& arg,
                                        I8 button) {
    return true;
}

bool CEGUIInput::joystickSliderMoved(const Input::JoystickEvent& arg,
                                     I8 index) {
    return true;
}

bool CEGUIInput::joystickVector3DMoved(const Input::JoystickEvent& arg,
                                       I8 index) {
    return true;
}
};