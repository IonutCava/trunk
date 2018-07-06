#include "Headers/CEGUIInput.h"

#ifndef CEGUI_STATIC
#define CEGUI_STATIC
#include <CEGUI/CEGUI.h>
#endif //CEGUI_STATIC

#include "GUI/Headers/GUI.h"

namespace Divide {

bool CEGUIInput::injectOISKey(bool pressed, const Input::KeyEvent& inKey) {
    bool processed = false;
    if (pressed) {
        if (CEGUI_DEFAULT_CTX.injectKeyDown((CEGUI::Key::Scan)inKey._key)) {
            CEGUI_DEFAULT_CTX.injectChar((CEGUI::Key::Scan)inKey._text);
            begin(inKey);
            processed = true;
        }
    } else {
        if (CEGUI_DEFAULT_CTX.injectKeyUp((CEGUI::Key::Scan)inKey._key)) {
            end(inKey);
            processed = true;
        }
    }
    return processed;
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
    return (CEGUI_DEFAULT_CTX.injectMouseWheelChange(to_float(arg.state.Z.abs)) ||
            CEGUI_DEFAULT_CTX.injectMouseMove(to_float(arg.state.X.rel),
                                              to_float(arg.state.Y.rel)));
}

bool CEGUIInput::mouseButtonPressed(const Input::MouseEvent& arg,
                                    Input::MouseButton button) {
    bool processed = false;
    switch (button) {
        case Input::MouseButton::MB_Left: {
            processed = CEGUI_DEFAULT_CTX.injectMouseButtonDown(CEGUI::LeftButton);
        } break;
        case Input::MouseButton::MB_Middle: {
            processed = CEGUI_DEFAULT_CTX.injectMouseButtonDown(CEGUI::MiddleButton);
        } break;
        case Input::MouseButton::MB_Right: {
            processed = CEGUI_DEFAULT_CTX.injectMouseButtonDown(CEGUI::RightButton);
        } break;
        case Input::MouseButton::MB_Button3: {
            processed = CEGUI_DEFAULT_CTX.injectMouseButtonDown(CEGUI::X1Button);
        } break;
        case Input::MouseButton::MB_Button4: {
            processed = CEGUI_DEFAULT_CTX.injectMouseButtonDown(CEGUI::X2Button);
        } break;
        default: break;
    };

    return processed;
}

bool CEGUIInput::mouseButtonReleased(const Input::MouseEvent& arg,
                                     Input::MouseButton button) {
    bool processed = false;
    switch (button) {
        case Input::MouseButton::MB_Left: {
            processed = CEGUI_DEFAULT_CTX.injectMouseButtonUp(CEGUI::LeftButton);
        } break;
        case Input::MouseButton::MB_Middle: {
            processed = CEGUI_DEFAULT_CTX.injectMouseButtonUp(CEGUI::MiddleButton);
        } break;
        case Input::MouseButton::MB_Right: {
            processed = CEGUI_DEFAULT_CTX.injectMouseButtonUp(CEGUI::RightButton);
        } break;
        case Input::MouseButton::MB_Button3: {
            processed = CEGUI_DEFAULT_CTX.injectMouseButtonUp(CEGUI::X1Button);
        } break;
        case Input::MouseButton::MB_Button4: {
            processed = CEGUI_DEFAULT_CTX.injectMouseButtonUp(CEGUI::X2Button);
        } break;
        default: break;
    };

    return processed;
}

bool CEGUIInput::joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis) {
    return false;
}

bool CEGUIInput::joystickPovMoved(const Input::JoystickEvent& arg, I8 pov) {
    return false;
}

bool CEGUIInput::joystickButtonPressed(const Input::JoystickEvent& arg,
                                       Input::JoystickButton button) {
    return false;
}

bool CEGUIInput::joystickButtonReleased(const Input::JoystickEvent& arg,
                                        Input::JoystickButton button) {
    return false;
}

bool CEGUIInput::joystickSliderMoved(const Input::JoystickEvent& arg,
                                     I8 index) {
    return false;
}

bool CEGUIInput::joystickVector3DMoved(const Input::JoystickEvent& arg,
                                       I8 index) {
    return false;
}
};