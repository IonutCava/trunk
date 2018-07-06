/*“Copyright 2009-2012 DIVIDE-Studio”*/
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

#ifndef _INPUT_EVENT_HANDLER_H_
#define _INPUT_EVENT_HANDLER_H_

#include "OIS.h"
#include "Hardware/Platform/Mutex.h"
#include "Hardware/Platform/PlatformDefines.h"

class InputInterface;
class JoystickInterface;
class EffectManager;
class Kernel;

class EventHandler : public OIS::KeyListener, public OIS::JoyStickListener,public OIS::MouseListener {
  protected:

    InputInterface*    _pApplication;
    JoystickInterface* _pJoystickInterface;
	EffectManager*	   _pEffectMgr;
	Kernel*			   _kernel;

  public:

    EventHandler(InputInterface* pApp, Kernel* const kernel);
    void initialize(JoystickInterface* pJoystickInterface, EffectManager* pEffectMgr);

	///Keyboard
	bool keyPressed( const OIS::KeyEvent &arg );
	bool keyReleased( const OIS::KeyEvent &arg );
	///Joystick\Gamepad
	bool buttonPressed( const OIS::JoyStickEvent &arg, I8 button );
	bool buttonReleased( const OIS::JoyStickEvent &arg, I8 button );
	bool axisMoved( const OIS::JoyStickEvent &arg, I8 axis );
	bool povMoved( const OIS::JoyStickEvent &arg, I8 pov );
	///Mouse
	bool mouseMoved( const OIS::MouseEvent &arg );
	bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
	bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
};

#endif