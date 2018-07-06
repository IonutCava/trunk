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

#ifndef _FRAME_LISTENER_MANAGER_H_
#define _FRAME_LISTENER_MANAGER_H_

#include "core.h"
///Add this include here so that any FrameListner derived class only needs to include the manager
#include "Rendering/Headers/FrameListener.h"

enum FrameEventType{
	FRAME_EVENT_ANY,
	FRAME_EVENT_STARTED,
	FRAME_PRERENDER_START,
	FRAME_PRERENDER_END,
	FRAME_EVENT_PROCESS,
	FRAME_EVENT_ENDED,
};

class FrameListener;
struct FrameEvent;
DEFINE_SINGLETON(FrameListenerManager)

typedef Unordered_map<std::string, FrameListener* > ListenerMap;
typedef vectorImpl<F32> EventTimeMap;

public:
	void registerFrameListener(FrameListener* listener);
	void removeFrameListener(FrameListener* listener);
	void idle();

	bool frameStarted(const FrameEvent& evt);
	bool framePreRenderStarted(const FrameEvent& evt);
	bool framePreRenderEnded(const FrameEvent& evt);
	bool frameRenderingQueued(const FrameEvent& evt);
	bool frameEnded(const FrameEvent& evt);

	void createEvent(FrameEventType type, FrameEvent& evt);

private:
	F32 calculateEventTime(F32 currentTime, FrameEventType type);

private:
	ListenerMap _listeners;
	ListenerMap _removedListeners;
	EventTimeMap _eventTimers[6];

END_SINGLETON

inline void REGISTER_FRAME_LISTENER(FrameListener* listener){
	FrameListenerManager::getInstance().registerFrameListener(listener);
}

#endif