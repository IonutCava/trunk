/*“Copyright 2009-2011 DIVIDE-Studio”*/
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
#include "resource.h"

//As you might of guessed it, it's the same system used in Ogre3D (http://www.ogre3d.org/docs/api/html/OgreFrameListener_8h_source.html)
//I'm sorry for using it, but it's so simple to implement, I decided to use something that people already know and are comfortable with
//-Ionut
struct FrameEvent{
  F32 _timeSinceLastEvent;
  F32 _timeSinceLastFrame;
};

class FrameListener{
public:
	FrameListener(const std::string& name){_name = name;}

	virtual ~FrameListener(){}
	//Adapter patern instead of pure interface for the same reason as the Ogre boys pointed out:
	//Implement what you need without filling classes with dummy functions
	virtual bool frameStarted(const FrameEvent& evt) {return true;}
	virtual bool frameRenderingQueued(const FrameEvent& evt) {return true;}
	virtual bool frameEnded(const FrameEvent& evt) {return true;}
	const std::string& getName() {return _name;}

private:
	std::string _name;
};
#endif