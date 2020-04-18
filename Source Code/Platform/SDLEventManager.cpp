#include "stdafx.h"

#include "Headers/SDLEventManager.h"
#include "Headers/SDLEventListener.h"

namespace Divide {

    SharedMutex SDLEventManager::s_eventListenerLock;
    vectorEASTLFast<SDLEventListener*> SDLEventManager::s_eventListeners;

    void SDLEventManager::registerListener(SDLEventListener& listener) {
        UniqueLock<SharedMutex> lock(s_eventListenerLock);

        const U64 targetID = listener.listenerID();

        // prevent double add
        vectorEASTLFast<SDLEventListener*>::const_iterator it;
        it = eastl::find_if(eastl::cbegin(s_eventListeners), eastl::cend(s_eventListeners), [targetID](SDLEventListener* listener) {
            return listener && listener->listenerID() == targetID;
        });
        assert(it == eastl::cend(s_eventListeners));

        s_eventListeners.push_back(&listener);
    }

    void SDLEventManager::unregisterListener(SDLEventListener& listener) {
        UniqueLock<SharedMutex> lock(s_eventListenerLock);

        const U64 targetID = listener.listenerID();

        // prevent double remove
        vectorEASTLFast<SDLEventListener*>::const_iterator it;
        it = eastl::find_if(eastl::cbegin(s_eventListeners), eastl::cend(s_eventListeners), [targetID](SDLEventListener* listener) {
            return listener && listener->listenerID() == targetID;
        });
        assert(it != eastl::cend(s_eventListeners));

        s_eventListeners.erase(it);
    }

    void SDLEventManager::pollEvents() {
        OPTICK_EVENT();

        SDL_Event evt;
        while (SDL_PollEvent(&evt)) {
            OPTICK_EVENT("OnLoop");
            OPTICK_TAG("Event", evt.type);

            SharedLock<SharedMutex> lock(s_eventListenerLock);

            for (SDLEventListener* listener : s_eventListeners) {
                assert(listener != nullptr);

                if (listener->onSDLEvent(evt)) {
                    break;
                }
            }
        }
    }
}; //namespace Divide
