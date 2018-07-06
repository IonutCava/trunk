/*
   Copyright (c) 2013 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

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