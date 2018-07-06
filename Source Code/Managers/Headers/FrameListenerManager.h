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

#ifndef _FRAME_LISTENER_MANAGER_H_
#define _FRAME_LISTENER_MANAGER_H_
#include "resource.h"

enum FRAME_EVENT_TYPE{
	FRAME_EVENT_ANY,
	FRAME_EVENT_STARTED,
	FRAME_EVENT_PROCESS,
	FRAME_EVENT_ENDED
};

class FrameListener;
struct FrameEvent;
DEFINE_SINGLETON(FrameListenerManager)

typedef unordered_map<std::string, FrameListener* > ListenerMap;
typedef std::vector<F32> EventTimeMap;

public:
	void registerFrameListener(FrameListener* listener);
	void removeFrameListener(FrameListener* listener);
	void idle();

	bool frameStarted(const FrameEvent& evt);
	bool frameRenderingQueued(const FrameEvent& evt);
	bool frameEnded(const FrameEvent& evt);

	void createEvent(FRAME_EVENT_TYPE type, FrameEvent& evt);

private:
	F32 calculateEventTime(F32 currentTime, FRAME_EVENT_TYPE type);

private:
	ListenerMap _listeners;
	ListenerMap _removedListeners;
	EventTimeMap _eventTimers[4];

END_SINGLETON
#endif