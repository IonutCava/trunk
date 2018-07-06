#include "Headers/FrameListenerManager.h"

#include "Core/Headers/Console.h"
#include "Core/Math/Headers/MathHelper.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

/// Register a new Frame Listener to be processed every frame
void FrameListenerManager::registerFrameListener(FrameListener* listener,
                                                 U32 callOrder) {
    // Check if the listener has a name or we should assign an id
    if (listener->getListenerName().empty()) {
        listener->setName(Util::StringFormat("generic_f_listener_%d", _listeners.size()));
    }

    listener->setCallOrder(callOrder);

    _listeners.push_back(listener);

    std::sort(std::begin(_listeners), std::end(_listeners));
}

/// Remove an existent Frame Listener from our collection
void FrameListenerManager::removeFrameListener(FrameListener* listener) {
    const stringImpl& name = listener->getListenerName();
    vectorImpl<FrameListener*>::iterator it;
    it = std::find_if(std::begin(_listeners), std::end(_listeners),
                      [&name](FrameListener const* fl)
                          -> bool { return fl->getListenerName().compare(name) == 0; });
    if (it != std::end(_listeners)) {
        _listeners.erase(it);
    } else {
        Console::errorfn(Locale::get(_ID("ERROR_FRAME_LISTENER_REMOVE")),
                         listener->getListenerName().c_str());
    }
}

/// For each listener, notify of current event and check results
/// If any Listener returns false, the whole manager returns false for this
/// specific step
/// If the manager returns false at any step, the application exists
bool FrameListenerManager::frameEvent(const FrameEvent& evt) {
    switch (evt._type) {
        case FrameEventType::FRAME_EVENT_STARTED:
            return frameStarted(evt);
        case FrameEventType::FRAME_PRERENDER_START:
            return framePreRenderStarted(evt);
        case FrameEventType::FRAME_PRERENDER_END:
            return framePreRenderEnded(evt);
        case FrameEventType::FRAME_POSTRENDER_START:
            return framePostRenderStarted(evt);
        case FrameEventType::FRAME_POSTRENDER_END:
            return framePostRenderEnded(evt);
        case FrameEventType::FRAME_EVENT_PROCESS:
            return frameRenderingQueued(evt);
        case FrameEventType::FRAME_EVENT_ENDED:
            return frameEnded(evt);
    };

    return false;
}

bool FrameListenerManager::frameStarted(const FrameEvent& evt) {
    for (FrameListener* listener : _listeners) {
        if (!listener->frameStarted(evt)) {
            return false;
        }
    }
    return true;
}

bool FrameListenerManager::framePreRenderStarted(const FrameEvent& evt) {
    for (FrameListener* listener : _listeners) {
        if (!listener->framePreRenderStarted(evt)) {
            return false;
        }
    }
    return true;
}

bool FrameListenerManager::framePreRenderEnded(const FrameEvent& evt) {
    for (FrameListener* listener : _listeners) {
        if (!listener->framePreRenderEnded(evt)) {
            return false;
        }
    }
    return true;
}

bool FrameListenerManager::frameRenderingQueued(const FrameEvent& evt) {
    for (FrameListener* listener : _listeners) {
        if (!listener->frameRenderingQueued(evt)) {
            return false;
        }
    }
    return true;
}

bool FrameListenerManager::framePostRenderStarted(const FrameEvent& evt) {
    for (FrameListener* listener : _listeners) {
        if (!listener->framePostRenderStarted(evt)) {
            return false;
        }
    }
    return true;
}

bool FrameListenerManager::framePostRenderEnded(const FrameEvent& evt) {
    for (FrameListener* listener : _listeners) {
        if (!listener->framePostRenderEnded(evt)) {
            return false;
        }
    }
    return true;
}

bool FrameListenerManager::frameEnded(const FrameEvent& evt) {
    for (FrameListener* listener : _listeners) {
        if (!listener->frameEnded(evt)) {
            return false;
        }
    }
    return true;
}

/// When the application is idle, we should really clear up old events
void FrameListenerManager::idle() {}

/// Please see the Ogre3D documentation about this
void FrameListenerManager::createEvent(const U64 currentTime,
                                       FrameEventType type, FrameEvent& evt) {
    evt._currentTime = Time::MicrosecondsToMilliseconds<D32>(currentTime);
    evt._timeSinceLastEvent =
        calculateEventTime(evt._currentTime, FrameEventType::FRAME_EVENT_ANY);
    evt._timeSinceLastFrame = calculateEventTime(evt._currentTime, type);
    evt._type = type;
}

D32 FrameListenerManager::calculateEventTime(const D32 currentTime,
                                             FrameEventType type) {
    EventTimeMap& times = _eventTimers[to_uint(type)];
    times.push_back(currentTime);

    if (times.size() == 1) {
        return 0.0;
    }

    EventTimeMap::const_iterator it = std::cbegin(times);
    EventTimeMap::iterator end = std::end(times) - 2;

    while (it != end) {
        if (currentTime - *it > 0) {
            ++it;
        } else {
            break;
        }
    }

    times.erase(std::cbegin(times), it);
    return (times.back() - times.front()) /
           Time::SecondsToMilliseconds(times.size() - 1);
}
};