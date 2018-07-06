#include "stdafx.h"

#include "Headers/DebugInterface.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"

namespace Divide {

DebugInterface::DebugInterface(Kernel& parent)
    : KernelComponent(parent),
      _dirty(false)
{

}

DebugInterface::~DebugInterface()
{

}

DebugInterface::DebugVar::DebugVar(const DebugVarDescriptor& descriptor)
    : GUIDWrapper(),
     _descriptor(descriptor)
{
}

DebugInterface::DebugGroup::DebugGroup() noexcept
    : GUIDWrapper(),
    _parentGroup(-1)
{
}

void DebugInterface::idle() {
    if (_parent.platformContext().config().gui.enableDebugVariableControls) {
        if (_dirty) {
            //ToDo: Use a IMGUI window to list these. Maybe use the PushConstant / Component view approach used in the Editor? -Ionut
            _dirty = false;
        }
    }
}

I64 DebugInterface::addDebugVar(const DebugVarDescriptor& descriptor) {
    DebugVar temp(descriptor);

    WriteLock lock(_varMutex);
    _debugVariables.insert(hashAlg::make_pair(temp.getGUID(), temp));

    _dirty = true;

    return temp.getGUID();
}

I64 DebugInterface::addDebugGroup(const char* name, I64 parentGUID) {
    DebugGroup temp;
    temp._name = name;

    WriteLock lock(_groupMutex);
    hashMap<I64, DebugGroup>::iterator it = _debugGroups.find(parentGUID);

    if (it != std::cend(_debugGroups)) {
        temp._parentGroup = parentGUID;
        it->second._childGroupsGUID.push_back(temp.getGUID());
    }

    hashAlg::insert(_debugGroups, hashAlg::make_pair(temp.getGUID(), temp));

    return temp.getGUID();
}

I64 DebugInterface::getDebugGroup(const char* name) {
    ReadLock lock (_groupMutex);
    for (hashMap<I64, DebugGroup>::value_type& group : _debugGroups) {
        if (group.second._name.compare(name) == 0) {
            return group.first;
        }
    }

    return -1;
}

}; //namsepace Divide