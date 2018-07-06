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

#ifndef _FRAME_LISTENER_H_
#define _FRAME_LISTENER_H_
#include "core.h"

///As you might of guessed it, it's the same system used in Ogre3D (http://www.ogre3d.org/docs/api/html/OgreFrameListener_8h_source.html)
///I'm sorry for using it, but it's so simple to implement, I decided to use something that people already know and are comfortable with
///-Ionut
struct FrameEvent{
  F32 _timeSinceLastEvent;
  F32 _timeSinceLastFrame;
  F32 _currentTime;
};
/// FrameListener class.
///Has 3 events, associated with the start of rendering a frame, the end of rendering and the end of buffer swapping after frames
///All events have timers associated with them for update timing
class FrameListener{
public:
	///Either give it a name
	FrameListener(const std::string& name){_listenerName = name;}
	///Or the frame listenr manager will assing it an ID
	FrameListener() {}
	virtual ~FrameListener(){}
	inline const std::string& getName() {return _listenerName;}

protected:
	friend class FrameListenerManager;
	inline void  setName(const std::string& name) {_listenerName = name;}
	///Adapter patern instead of pure interface for the same reason as the Ogre boys pointed out:
	///Implement what you need without filling classes with dummy functions
	///frameStarted is calld at the beggining of a new frame
	virtual bool frameStarted(const FrameEvent& evt) {return true;}
	///framePreRenderEnded is called after all the prerendering has finished and rendering should start
	virtual bool framePreRenderEnded(const FrameEvent& evt) {return true;}
	///frameRendering Queued is called after all the frame setup/rendering but before the call to SwapBuffers
	virtual bool frameRenderingQueued(const FrameEvent& evt) {return true;}
	///frameEnded is called after the buffers have been swapped
	virtual bool frameEnded(const FrameEvent& evt) {return true;}

private:
	///not _name so that it doesn't conflict with Resource base class
	std::string _listenerName;
};
#endif