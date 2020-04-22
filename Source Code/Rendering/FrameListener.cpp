#include "stdafx.h"

#include "Headers/FrameListener.h"
#include "Managers/Headers/FrameListenerManager.h"

namespace Divide {
    /// Either give it a name
    FrameListener::FrameListener(const Str64& name, FrameListenerManager& parent, U32 callOrder)
        : GUIDWrapper(),
         _callOrder(callOrder),
         _mgr(parent)
    {
        _listenerName = name;
        _mgr.registerFrameListener(this, callOrder);
    }

    FrameListener::~FrameListener()
    {
        _mgr.removeFrameListener(this);
    }
}; //namespace Divide