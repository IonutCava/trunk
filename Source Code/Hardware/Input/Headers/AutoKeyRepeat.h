/*“Copyright 2009-2013 DIVIDE-Studio”*/
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
#ifndef _AUTO_KEY_REPEAT_H_
#define _AUTO_KEY_REPEAT_H_

//Adapted from: http://www.ogre3d.org/tikiwiki/tiki-index.php?page=Auto+Repeat+Key+Input
#include <OIS.h>

#include "Hardware/Platform/Headers/PlatformDefines.h"

/// A class that repeatedly calls "repeatKey" between "begin" and "end" calls at specified intervals
class AutoRepeatKey {
private:

    I32 _key;
    U32 _char;

    F32 _elapsed;
    F32 _delay;

    F32 _repeatDelay;  //< Time intervals between key injections
    F32 _initialDelay; //< The time after begin() and before repeatKey() is called. If end() is called in that interval, the key will not repeat

protected:
	///Override this to define custom events for key repeats
	virtual void repeatKey(I32 inKey, U32 Char) = 0;

public:
	///Default constructor
     AutoRepeatKey(F32 repeatDelay = 0.035f, F32 initialDelay = 0.300f);
	///Called when a key is pressed
    void begin(const OIS::KeyEvent &evt);
	///Called when a key is released
    void end(const OIS::KeyEvent &evt);
	///Update the internal time interval between frames
    void update(F32 elapsed);
	///Adjust delay between key injections
	inline void setRepeatDelay(F32 repeatDelay) {_repeatDelay = repeatDelay;}
	///Adjust the initial delay before we start injecting key repeats
	inline void setInitialDelay(F32 initialDelay) {_initialDelay = initialDelay;}
};

#endif