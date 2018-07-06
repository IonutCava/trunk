#include "Headers/FrameListenerManager.h"
#include "Rendering/Headers/FrameListener.h"

//Register a new Frame Listener to be processed every frame
void FrameListenerManager::registerFrameListener(FrameListener* listener){
		//Check if the listener is in the trash bin
		ListenerMap::iterator it = _removedListeners.find(listener->getName());
		//If it is, restore it, else, just add a new one
		if (it != _removedListeners.end()){
			_removedListeners.erase(it);
		}else{
			_listeners.insert(std::make_pair(listener->getName(), listener));
		}

}

//Remove an existent Frame Listener from our map
void FrameListenerManager::removeFrameListener(FrameListener* listener){
	ListenerMap::iterator it = _listeners.find(listener->getName());
	if(it != _listeners.end()){
		_removedListeners.insert(std::make_pair(it->second->getName(),it->second));
	}else{
		Console::getInstance().errorfn("FrameListener: Trying to remove non-existant listener [ %s ]", listener->getName().c_str());
	}
}

//For each listener, notify of current event and check results
//If any Listener returns false, the whole manager returns false for this specific step
//If the manager returns false at any step, the application exists
bool FrameListenerManager::frameStarted(const FrameEvent& evt){
	for_each(ListenerMap::value_type& listener, _listeners){
		if(!listener.second->frameStarted(evt)){
			return false;
		}
	}
	return true;
}

bool FrameListenerManager::frameRenderingQueued(const FrameEvent& evt){
	for_each(ListenerMap::value_type& listener, _listeners){
		if(!listener.second->frameRenderingQueued(evt)){
			return false;
		}
	}
	return true;
}

bool FrameListenerManager::frameEnded(const FrameEvent& evt){
	for_each(ListenerMap::value_type& listener, _listeners){
		if(!listener.second->frameEnded(evt)){
			return false;
		}
	}
	return true;
}

//When the application is idle, we should really clear up old events
void FrameListenerManager::idle(){
	for_each(ListenerMap::value_type& listener, _removedListeners){
		_listeners.erase(listener.second->getName());
	}
	_removedListeners.clear();
}

//Please see the Ogre3D documentation about this
void FrameListenerManager::createEvent(FRAME_EVENT_TYPE type, FrameEvent& evt){
		F32 now = GETMSTIME();
		evt._timeSinceLastEvent = calculateEventTime(now, FRAME_EVENT_ANY);
		evt._timeSinceLastFrame = calculateEventTime(now, type);
}

F32 FrameListenerManager::calculateEventTime(F32 currentTime, FRAME_EVENT_TYPE type){
        EventTimeMap& times = _eventTimers[type];
        times.push_back(currentTime);

		if(times.size() == 1){
            return 0;
		}
        EventTimeMap::iterator it = times.begin(), end = times.end()-2;
        while(it != end){
			if (currentTime - *it > 0){
                ++it;
			}else{
                break;
			}
        }
        times.erase(times.begin(), it);
        return F32(times.back() - times.front()) / ((times.size()-1) * 1000);
}