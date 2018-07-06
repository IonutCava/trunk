#include "Headers/CEGUIInput.h"

#include <CEGUI/CEGUI.h>
#include "GUI/Headers/GUI.h"

bool CEGUIInput::injectOISKey(bool pressed,const OIS::KeyEvent& inKey){
     if (pressed){
        CEGUI_DEFAULT_CONTEXT.injectKeyDown((CEGUI::Key::Scan)inKey.key);
        CEGUI_DEFAULT_CONTEXT.injectChar((CEGUI::Key::Scan)inKey.text);
        begin(inKey);
    }else{
        CEGUI_DEFAULT_CONTEXT.injectKeyUp((CEGUI::Key::Scan)inKey.key);
        end(inKey);
    }
    return true;
}

void CEGUIInput::repeatKey(I32 inKey, U32 Char) {
    // Now remember the key is still down, so we need to simulate the key being released, and then repressed immediatly
    CEGUI_DEFAULT_CONTEXT.injectKeyUp( (CEGUI::Key::Scan)inKey );   // Key UP
    CEGUI_DEFAULT_CONTEXT.injectKeyDown( (CEGUI::Key::Scan)inKey ); // Key Down
    CEGUI_DEFAULT_CONTEXT.injectChar( Char );     // What that key means
}

bool CEGUIInput::onKeyDown(const OIS::KeyEvent& key) {
    return injectOISKey(true, key);
}

bool CEGUIInput::onKeyUp(const OIS::KeyEvent& key) {
    return injectOISKey(false, key);
}

bool CEGUIInput::mouseMoved(const OIS::MouseEvent& arg) {
    CEGUI_DEFAULT_CONTEXT.injectMouseWheelChange(arg.state.Z.abs);
    CEGUI_DEFAULT_CONTEXT.injectMouseMove(arg.state.X.rel, arg.state.Y.rel);
    return true;
}

bool CEGUIInput::mouseButtonPressed(const OIS::MouseEvent& arg, OIS::MouseButtonID button) {
    switch (button) {
        case OIS::MB_Left : {
            CEGUI_DEFAULT_CONTEXT.injectMouseButtonDown(CEGUI::LeftButton);
        } break;
        case OIS::MB_Middle : {
            CEGUI_DEFAULT_CONTEXT.injectMouseButtonDown(CEGUI::MiddleButton);
        } break;
        case OIS::MB_Right : {
            CEGUI_DEFAULT_CONTEXT.injectMouseButtonDown(CEGUI::RightButton);
        } break;
        case OIS::MB_Button3 : {
            CEGUI_DEFAULT_CONTEXT.injectMouseButtonDown(CEGUI::X1Button);
        } break;
        case OIS::MB_Button4 : {
            CEGUI_DEFAULT_CONTEXT.injectMouseButtonDown(CEGUI::X2Button);
        } break;
        default : return false;
    };
 
    return true;
}

bool CEGUIInput::mouseButtonReleased(const OIS::MouseEvent& arg, OIS::MouseButtonID button) {
    switch (button) {
        case OIS::MB_Left : {
            CEGUI_DEFAULT_CONTEXT.injectMouseButtonUp(CEGUI::LeftButton);
        } break;
        case OIS::MB_Middle : {
            CEGUI_DEFAULT_CONTEXT.injectMouseButtonUp(CEGUI::MiddleButton);
        } break;
        case OIS::MB_Right : {
            CEGUI_DEFAULT_CONTEXT.injectMouseButtonUp(CEGUI::RightButton);
        } break;
        case OIS::MB_Button3 : {
            CEGUI_DEFAULT_CONTEXT.injectMouseButtonUp(CEGUI::X1Button);
        } break;
        case OIS::MB_Button4 : {
            CEGUI_DEFAULT_CONTEXT.injectMouseButtonUp(CEGUI::X2Button);
        } break;
        default : return false;
    };
 
    return true;
}

bool CEGUIInput::joystickAxisMoved(const OIS::JoyStickEvent& arg, I8 axis) {
    return true;
}

bool CEGUIInput::joystickPovMoved(const OIS::JoyStickEvent& arg, I8 pov){
    return true;
}

bool CEGUIInput::joystickButtonPressed(const OIS::JoyStickEvent& arg, I8 button){
    return true;
}

bool CEGUIInput::joystickButtonReleased(const OIS::JoyStickEvent& arg, I8 button){
    return true;
}

bool CEGUIInput::joystickSliderMoved( const OIS::JoyStickEvent &arg, I8 index){
    return true;
}

bool CEGUIInput::joystickVector3DMoved( const OIS::JoyStickEvent &arg, I8 index){
    return true;
}