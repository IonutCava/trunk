#include "Headers/FrameListenerManager.h"

///Register a new Frame Listener to be processed every frame
void FrameListenerManager::registerFrameListener(FrameListener* listener, U32 callOrder){
    //Check if the listener has a name or we should assign an id
    if(listener->getName().empty()){
        listener->setName("generic_f_listener_" + Util::toString(_listeners.size() + _removedListeners.size()));
    }
    listener->setCallOrder(callOrder);
    //Check if the listener is in the trash bin
    for (vectorImpl<FrameListener* >::iterator it = _removedListeners.begin(); it != _removedListeners.end(); ++it){
        //If it is, restore it, else, just add a new one
        if ((*it)->getName().compare(listener->getName()) == 0){
            _removedListeners.erase(it);
            break;
        }
    }

    _listeners.push_back(listener);

    std::sort(_listeners.begin(), _listeners.end());
}

///Remove an existent Frame Listener from our collection
void FrameListenerManager::removeFrameListener(FrameListener* listener){
    vectorImpl<FrameListener* >::const_iterator it = findListener(listener->getName());
    if (it != _listeners.end()){        //If it is, restore it, else, just add a new one
        _removedListeners.push_back(*it);
    }else{
        ERROR_FN(Locale::get("ERROR_FRAME_LISTENER_REMOVE"), listener->getName().c_str());
    }
}

///For each listener, notify of current event and check results
///If any Listener returns false, the whole manager returns false for this specific step
///If the manager returns false at any step, the application exists
bool FrameListenerManager::frameEvent(const FrameEvent& evt) {
    bool state = false;
    switch (evt._type){
        default: break;
        case FRAME_EVENT_STARTED : 
            state = frameStarted(evt); 
            break;
        case FRAME_PRERENDER_START :
            state = framePreRenderStarted(evt); 
            break;
        case FRAME_PRERENDER_END : 
            state = framePreRenderEnded(evt); 
            break;
        case FRAME_POSTRENDER_START : 
            state = framePostRenderStarted(evt); 
            break;
        case FRAME_POSTRENDER_END : 
            state = framePostRenderEnded(evt); 
            break;
        case FRAME_EVENT_PROCESS :
            state = frameRenderingQueued(evt); 
            break;
        case FRAME_EVENT_ENDED : 
            state = frameEnded(evt); 
            break;
    };
    return state;
}

bool FrameListenerManager::frameStarted(const FrameEvent& evt){
    for(FrameListener* listener : _listeners){
        if(!listener->frameStarted(evt)){
            return false;
        }
    }
    return true;
}

bool FrameListenerManager::framePreRenderStarted(const FrameEvent& evt){
    for (FrameListener* listener : _listeners){
        if(!listener->framePreRenderStarted(evt)){
            return false;
        }
    }
    return true;
}

bool FrameListenerManager::framePreRenderEnded(const FrameEvent& evt){
    for (FrameListener* listener : _listeners){
        if(!listener->framePreRenderEnded(evt)){
            return false;
        }
    }
    return true;
}

bool FrameListenerManager::frameRenderingQueued(const FrameEvent& evt){
    for (FrameListener* listener : _listeners){
        if(!listener->frameRenderingQueued(evt)){
            return false;
        }
    }
    return true;
}

bool FrameListenerManager::framePostRenderStarted(const FrameEvent& evt){
    for (FrameListener* listener : _listeners){
        if (!listener->framePostRenderStarted(evt)){
            return false;
        }
    }
    return true;
}

bool FrameListenerManager::framePostRenderEnded(const FrameEvent& evt){
    for (FrameListener* listener : _listeners){
        if (!listener->framePostRenderEnded(evt)){
            return false;
        }
    }
    return true;
}

bool FrameListenerManager::frameEnded(const FrameEvent& evt){
    for (FrameListener* listener : _listeners){
        if(!listener->frameEnded(evt)){
            return false;
        }
    }
    return true;
}

///When the application is idle, we should really clear up old events
void FrameListenerManager::idle(){
    for (vectorImpl<FrameListener* >::iterator it = _removedListeners.end(); it != _removedListeners.begin(); it--){
        _listeners.erase(findListener((*it)->getName()));
    }

    _removedListeners.clear();
}

///Please see the Ogre3D documentation about this
void FrameListenerManager::createEvent(const U64 currentTime, FrameEventType type, FrameEvent& evt, const D32 interpolationFactor){
    evt._currentTime = getUsToMs(currentTime);
    evt._timeSinceLastEvent = calculateEventTime(evt._currentTime, FRAME_EVENT_ANY);
    evt._timeSinceLastFrame = calculateEventTime(evt._currentTime, type);
    evt._interpolationFactor = interpolationFactor;
    evt._type = type;
}

D32 FrameListenerManager::calculateEventTime(const D32 currentTime, FrameEventType type){

    EventTimeMap& times = _eventTimers[type];
    times.push_back(currentTime);

    if(times.size() == 1){
        return 0.0;
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
    return (times.back() - times.front()) / getSecToMs(times.size()-1);
}

vectorImpl<FrameListener* >::const_iterator FrameListenerManager::findListener(const std::string& name){
    vectorImpl<FrameListener* >::const_iterator it = _listeners.begin();
    for (;it != _listeners.end(); ++it){
        ///If it is, restore it, else, just add a new one
        if ((*it)->getName().compare(name) == 0)
            break;
    }

    return it;
}