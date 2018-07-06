#include "stdafx.h"

#include "Headers/DebugInterface.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include <AntTweakBar/include/AntTweakBar.h>

namespace Divide {

namespace {
    vectorImpl<TwBar*> g_TweakWindows;
    hashMapImpl<vectorAlg::vecSize, I64> g_barToGroupMap;
    hashMapImpl<I64, DELEGATE_CBK<void, void*>> g_varToCallbackMap;

    inline TwBar* getBar(I64 groupID) {
        for (hashMapImpl<vectorAlg::vecSize, I64>::value_type& entry : g_barToGroupMap) {
            if (entry.second == groupID) {
                return g_TweakWindows[entry.first];
            }
        }

        return nullptr;
    }

    inline TwType toTwType(CallbackParam type) {
        switch (type) {
            case CallbackParam::TYPE_SMALL_UNSIGNED_INTEGER: return TW_TYPE_UINT8;
            case CallbackParam::TYPE_MEDIUM_UNSIGNED_INTEGER: return TW_TYPE_UINT16;
            case CallbackParam::TYPE_UNSIGNED_INTEGER:
            case CallbackParam::TYPE_LARGE_UNSIGNED_INTEGER: return TW_TYPE_UINT32;


            case CallbackParam::TYPE_SMALL_INTEGER: return TW_TYPE_INT8;
            case CallbackParam::TYPE_MEDIUM_INTEGER: return TW_TYPE_INT16;
            case CallbackParam::TYPE_INTEGER:
            case CallbackParam::TYPE_LARGE_INTEGER: return TW_TYPE_INT32;
        };

        return TW_TYPE_INT32;
    }

    void TW_CALL SetCB(const void *value, void *clientData) {
        I64 variableGUID = *(I64*)&clientData;
        auto result = g_varToCallbackMap.find(variableGUID);
        if (result != std::cend(g_varToCallbackMap)) {
            DELEGATE_CBK<void, void*>& cbk = result->second;
            cbk(const_cast<void*>(value));
        }
    }

    void TW_CALL GetCB(void *value, void *clientData) {
        I64 variableGUID = *(I64*)&clientData;
        auto result = g_varToCallbackMap.find(variableGUID);
        if (result != std::cend(g_varToCallbackMap)) {
            DELEGATE_CBK<void, void*>& cbk = result->second;
            cbk(value);
        }
    }
};

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

DebugInterface::DebugGroup::DebugGroup()
    : GUIDWrapper(),
    _parentGroup(-1)
{
}

void DebugInterface::idle() {
    if (_parent.platformContext().config().gui.enableDebugVariableControls) {
        if (_dirty) {
            if (Config::USE_ANT_TWEAK_BAR) {
                // Remove all bars
                TwDeleteAllBars();
                g_TweakWindows.resize(0);
                g_barToGroupMap.clear();

                // Create a new bar per group
                for (hashMapImpl<I64, DebugGroup>::value_type& group : _debugGroups) {
                    g_TweakWindows.push_back(TwNewBar(group.second._name.c_str()));
                    hashAlg::insert(g_barToGroupMap, g_TweakWindows.size() - 1, group.first);
                }

                g_varToCallbackMap.clear();
                // Not the fastest, but meh ... debug stuff -Ionut
                for (hashMapImpl<I64, DebugVar>::value_type& var : _debugVariables) {
                    DebugVar& variable = var.second;
                    DebugVarDescriptor& descriptor = variable._descriptor;

                    TwBar* targetBar = getBar(descriptor._debugGroup);
                    assert(targetBar != nullptr);
                    if (!descriptor._cbk) {
                        TwAddVarRO(targetBar, variable._descriptor._displayName.c_str(), toTwType(descriptor._type), descriptor._variable, "");
                    } else {
                        auto result = hashAlg::insert(g_varToCallbackMap, variable.getGUID(), descriptor._cbk);
                        TwAddVarCB(targetBar, descriptor._displayName.c_str(), toTwType(descriptor._type), SetCB, GetCB, &(result.first->second), "");
                    }
                }
            }
            _dirty = false;
        }
    }
}

I64 DebugInterface::addDebugVar(const DebugVarDescriptor& descriptor) {
    DebugVar temp(descriptor);

    WriteLock lock(_varMutex);
    _debugVariables.insert(std::make_pair(temp.getGUID(), temp));

    _dirty = true;

    return temp.getGUID();
}

I64 DebugInterface::addDebugGroup(const char* name, I64 parentGUID) {
    DebugGroup temp;
    temp._name = name;

    WriteLock lock(_groupMutex);
    hashMapImpl<I64, DebugGroup>::iterator it = _debugGroups.find(parentGUID);

    if (it != std::cend(_debugGroups)) {
        temp._parentGroup = parentGUID;
        it->second._childGroupsGUID.push_back(temp.getGUID());
    }

    _debugGroups.insert(std::make_pair(temp.getGUID(), temp));

    return temp.getGUID();
}

I64 DebugInterface::getDebugGroup(const char* name) {
    ReadLock lock (_groupMutex);
    for (hashMapImpl<I64, DebugGroup>::value_type& group : _debugGroups) {
        if (group.second._name.compare(name) == 0) {
            return group.first;
        }
    }

    return -1;
}

}; //namsepace Divide