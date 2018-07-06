/*
Copyright (c) 2017 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef _DEBUG_INTERFACE_H_
#define _DEBUG_INTERFACE_H_

#include "Platform/Headers/PlatformDefines.h"
#include "Core/Headers/KernelComponent.h"

namespace Divide {

class Configuration;
class DebugInterface : public KernelComponent {
public:
    struct DebugVarDescriptor {
        I64 _debugGroup = -1;
        stringImpl _displayName = "";
        void* _variable = nullptr;
        CallbackParam _type = CallbackParam::COUNT;
        DELEGATE_CBK<void, void*> _cbk;
        bool _useMinMax = false;
        std::pair<AnyParam, AnyParam> _minMax;
    };

    class DebugVar : public GUIDWrapper {
    public:
        explicit DebugVar(const DebugVarDescriptor& descriptor);
        DebugVarDescriptor _descriptor;
        
    };

    class DebugGroup : public GUIDWrapper {
    public:
        DebugGroup() noexcept;
        stringImpl _name;
        I64 _parentGroup;
        vectorImpl<I64> _childGroupsGUID;
    };

public:
    explicit DebugInterface(Kernel& parent);
    ~DebugInterface();

    void idle();

    I64 addDebugVar(const DebugVarDescriptor& descriptor);

    I64 addDebugGroup(const char* name, I64 parentGUID);
    I64 getDebugGroup(const char* name);

    inline size_t debugVarCount() const {
        ReadLock lock(_varMutex);
        return _debugVariables.size();
    }

protected:
    inline hashMapImpl<I64, DebugVar>& getDebugVariables() noexcept {
        return _debugVariables;
    }

    inline hashMapImpl<I64, DebugGroup>& getDebugGroups() noexcept {
        return _debugGroups;
    }

protected:
    bool _dirty;
    mutable SharedLock _varMutex;
    mutable SharedLock _groupMutex;
    hashMapImpl<I64, DebugVar> _debugVariables;
    hashMapImpl<I64, DebugGroup> _debugGroups;
};

}; //namespace Divide
#endif //_DEBUG_INTERFACE_H_