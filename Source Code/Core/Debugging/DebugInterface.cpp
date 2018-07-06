#include "stdafx.h"

#include "Headers/DebugInterface.h"

namespace Divide {

DebugInterface::DebugVar::DebugVar()
    : GUIDWrapper(),
     _varPointer(nullptr),
     _locked(true),
     _type(CallbackParam::TYPE_VOID),
     _locationLine(0),
     _group(-1)
{
}

DebugInterface::DebugGroup::DebugGroup()
    : GUIDWrapper(),
    _parentGroup(-1)
{
}

I64 DebugInterface::addDebugVar(const char* file, I32 line, void* variable, CallbackParam type, I64 debugGroup, bool locked) {
    DebugVar temp;
    temp._varPointer = variable;
    temp._type = type;
    temp._locked = locked;
    temp._locationFile = file;
    temp._locationLine = static_cast<U16>(line);
    temp._group = debugGroup;

    WriteLock lock(_varMutex);
    _debugVariables.insert(std::make_pair(temp.getGUID(), temp));

    return temp.getGUID();
}

void DebugInterface::onDebugVarTrigger(I64 guid, bool invert) {
    WriteLock lock(_varMutex);
    hashMapImpl<I64, DebugVar>::iterator it = _debugVariables.find(guid);
    if (it != std::end(_debugVariables)) {
        DebugVar& var = it->second;
        if (!var._locked) {
            switch (var._type) {
                case CallbackParam::TYPE_BOOL: {
                    bool& variable = *reinterpret_cast<bool*>(var._varPointer);
                    variable = !variable;
                } break;
                case CallbackParam::TYPE_SMALL_INTEGER: {
                    I8& variable = *reinterpret_cast<I8*>(var._varPointer);
                    variable = invert ? (variable - 1) : (variable + 1);
                } break;
                case CallbackParam::TYPE_MEDIUM_INTEGER: {
                    I16& variable = *reinterpret_cast<I16*>(var._varPointer);
                    variable = invert ? (variable - 1) : (variable + 1);
                } break;
                case CallbackParam::TYPE_INTEGER: {
                    I32& variable = *reinterpret_cast<I32*>(var._varPointer);
                    variable = invert ? (variable - 1) : (variable + 1);
                } break;
                case CallbackParam::TYPE_LARGE_INTEGER: {
                    I64& variable = *reinterpret_cast<I64*>(var._varPointer);
                    variable = invert ? (variable - 1) : (variable + 1);
                } break;
                case CallbackParam::TYPE_SMALL_UNSIGNED_INTEGER: {
                    U8& variable = *reinterpret_cast<U8*>(var._varPointer);
                    variable = invert ? (std::max(variable, (U8)1) - 1) : (variable + 1);
                } break;
                case CallbackParam::TYPE_MEDIUM_UNSIGNED_INTEGER: {
                    U16& variable = *reinterpret_cast<U16*>(var._varPointer);
                    variable = invert ? (std::max(variable, (U16)1) - 1) : (variable + 1);
                } break;
                case CallbackParam::TYPE_UNSIGNED_INTEGER: {
                    U32& variable = *reinterpret_cast<U32*>(var._varPointer);
                    variable = invert ? (std::max(variable, 1u) - 1) : (variable + 1);
                } break;
                case CallbackParam::TYPE_LARGE_UNSIGNED_INTEGER: {
                    U64& variable = *reinterpret_cast<U64*>(var._varPointer);
                    variable = invert ? (std::max(variable, (U64)1) - 1) : (variable + 1);
                } break;
                case CallbackParam::TYPE_FLOAT: {
                    F32& variable = *reinterpret_cast<F32*>(var._varPointer);
                    variable = invert ? (variable - 1.0f) : (variable + 1.0f);
                } break;
                case CallbackParam::TYPE_DOUBLE: {
                    D64& variable = *reinterpret_cast<D64*>(var._varPointer);
                    variable = invert ? (variable - 1.0) : (variable + 1.0);
                } break;
                // unsupported
                default: break;
            };
        }
    }
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