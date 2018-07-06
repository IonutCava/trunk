/*�Copyright 2009-2013 DIVIDE-Studio�*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CEGUI_KEY_REPEAT_H_
#define _CEGUI_KEY_REPEAT_H_
#include "Hardware/Input/Headers/AutoKeyRepeat.h"

///This class defines AutoRepeatKey::repeatKey(...) as CEGUI key inputs
class GUIInput : public AutoRepeatKey {
public:
  ///Called on key events
  bool injectOISKey(bool pressed,const OIS::KeyEvent& inKey);

protected:
   void repeatKey(I32 inKey, U32 Char);
};

#endif