#ifndef _CEGUI_KEY_REPEAT_H_
#define _CEGUI_KEY_REPEAT_H_
#include "Hardware/Input/Headers/AutoKeyRepeat.h"

///This class defines AutoRepeatKey::repeatKey(...) as CEGUI key inputs
class GUIInput : public AutoRepeatKey {

public:
  ///Called on key events
  bool injectOISKey(bool pressed,const OIS::KeyEvent& inKey);
 
protected:
   void repeatKey(OIS::KeyCode inKey, U32 Char);
};

#endif