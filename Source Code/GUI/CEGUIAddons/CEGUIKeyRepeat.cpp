#include "Headers/CEGUIKeyRepeat.h"

#include <CEGUI/CEGUI.h>
#include "GUI/Headers/GUI.h"

bool GUIInput::injectOISKey(bool pressed,const OIS::KeyEvent& inKey){
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

void GUIInput::repeatKey(I32 inKey, U32 Char) {
	// Now remember the key is still down, so we need to simulate the key being released, and then repressed immediatly
    CEGUI_DEFAULT_CONTEXT.injectKeyUp( (CEGUI::Key::Scan)inKey );   // Key UP
    CEGUI_DEFAULT_CONTEXT.injectKeyDown( (CEGUI::Key::Scan)inKey ); // Key Down
    CEGUI_DEFAULT_CONTEXT.injectChar( Char );     // What that key means
}