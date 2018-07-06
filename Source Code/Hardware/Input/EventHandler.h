#ifndef _INPUT_EVENT_HANDLER_H_
#define _INPUT_EVENT_HANDLER_H_

#include "OIS.h"
#include "Hardware/Platform/PlatformDefines.h"

class InputManagerInterface;
class JoystickManager;
class EffectManager;
class Scene;
class EventHandler : public OIS::KeyListener, public OIS::JoyStickListener,public OIS::MouseListener
{
  protected:

    InputManagerInterface*     _pApplication;
    JoystickManager*		   _pJoystickMgr;
	EffectManager*			   _pEffectMgr;
	Scene*					   _activeScene;

  public:

    EventHandler(InputManagerInterface* pApp);
    void initialize(JoystickManager* pJoystickMgr, EffectManager* pEffectMgr);
	//Keyboard
	bool keyPressed( const OIS::KeyEvent &arg );
	bool keyReleased( const OIS::KeyEvent &arg );
	//Joystick\Gamepad
	bool buttonPressed( const OIS::JoyStickEvent &arg, I8 button );
	bool buttonReleased( const OIS::JoyStickEvent &arg, I8 button );
	bool axisMoved( const OIS::JoyStickEvent &arg, I8 axis );
	bool povMoved( const OIS::JoyStickEvent &arg, I8 pov );
	//Mouse
	bool mouseMoved( const OIS::MouseEvent &arg );
	bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
	bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id );

};

#endif