#include "stdafx.h"

#include "Headers/SDLEventManager.h"
#include "Headers/SDLEventListener.h"

namespace Divide {

    SharedMutex SDLEventManager::s_eventListenerLock;
    vectorFast<SDLEventListener*> SDLEventManager::s_eventListeners;

    void SDLEventManager::registerListener(SDLEventListener& listener) {
        UniqueLockShared lock(s_eventListenerLock);

        const U64 targetID = listener.listenerID();

        // prevent double add
        vectorFast<SDLEventListener*>::const_iterator it;
        it = std::find_if(std::cbegin(s_eventListeners), std::cend(s_eventListeners), [targetID](SDLEventListener* listener) {
            return listener && listener->listenerID() == targetID;
        });
        assert(it == std::cend(s_eventListeners));

        s_eventListeners.push_back(&listener);
    }

    void SDLEventManager::unregisterListener(SDLEventListener& listener) {
        UniqueLockShared lock(s_eventListenerLock);

        const U64 targetID = listener.listenerID();

        // prevent double remove
        vectorFast<SDLEventListener*>::const_iterator it;
        it = std::find_if(std::cbegin(s_eventListeners), std::cend(s_eventListeners), [targetID](SDLEventListener* listener) {
            return listener && listener->listenerID() == targetID;
        });
        assert(it != std::cend(s_eventListeners));

        s_eventListeners.erase(it);
    }

    void SDLEventManager::pollEvents() {
        OPTICK_EVENT();

        SDL_Event evt;
        while (SDL_PollEvent(&evt)) {
            SharedLock lock(s_eventListenerLock);

            for (SDLEventListener* listener : s_eventListeners) {
                if (listener->onSDLEvent(evt)) {
                    break;
                }
            }
        }
    }
}; //namespace Divide
