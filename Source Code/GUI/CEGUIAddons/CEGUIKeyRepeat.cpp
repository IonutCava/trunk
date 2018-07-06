#include "Headers/CEGUIKeyRepeat.h"

#include "CEGUI.h"
#include "GUI/Headers/GUI.h"

bool GUIInput::injectOISKey(bool pressed,const OIS::KeyEvent& inKey){
	 if (pressed){
		CEGUI::System::getSingleton().injectKeyDown(inKey.key);
		CEGUI::System::getSingleton().injectChar(inKey.text);
		begin(inKey);
	}else{
		CEGUI::System::getSingleton().injectKeyUp(inKey.key);
		end(inKey);
	}
	 return true;
 }
 
 void GUIInput::repeatKey(OIS::KeyCode inKey, U32 Char) {
	// Now remember the key is still down, so we need to simulate the key being released, and then repressed immediatly
    CEGUI::System::getSingleton().injectKeyUp( inKey );   // Key UP
    CEGUI::System::getSingleton().injectKeyDown( inKey ); // Key Down
    CEGUI::System::getSingleton().injectChar( Char );     // What that key means
 }