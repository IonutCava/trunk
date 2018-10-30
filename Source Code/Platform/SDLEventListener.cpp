#include "stdafx.h"

#include "Headers/SDLEventListener.h"
#include "Headers/SDLEventManager.h"

namespace Divide {

    std::atomic<U64> SDLEventListener::s_listenerIDCounter;

    SDLEventListener::SDLEventListener()
        : _listenerID(s_listenerIDCounter.fetch_add(1))
    {
        SDLEventManager::registerListener(*this);
    }

    SDLEventListener::~SDLEventListener()
    {
        SDLEventManager::unregisterListener(*this);
    }
};