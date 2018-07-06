/*
Copyright (c) 2016 DIVIDE-Studio
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
#include "Utility/Headers/GUIDWrapper.h"
#include "Platform/Threading/Headers/SharedMutex.h"

namespace Divide {

namespace Attorney {
    class DebugInterfaceGUI;
};

DEFINE_SINGLETON(DebugInterface)
    friend class Attorney::DebugInterfaceGUI;

public:
    class DebugVar : public GUIDWrapper {
       public:
        DebugVar();
        void* _varPointer;
        bool  _locked;
        CallbackParam _type;
        stringImpl _locationFile;
        U16  _locationLine;
        I64  _group;
    };

    class DebugGroup : public GUIDWrapper {
        public:
           DebugGroup();
           stringImpl _name;
           I64 _parentGroup;
           vectorImpl<I64> _childGroupsGUID;
    };

public:
    I64 addDebugVar(const char* file, I32 line, void* variable, CallbackParam type, I64 debugGroup, bool locked);
    void onDebugVarTrigger(I64 guid, bool invert);

    I64 addDebugGroup(const char* name, I64 parentGUID);
    I64 getDebugGroup(const char* name);

    inline size_t debugVarCount() const  {
        ReadLock lock(_varMutex);
        return _debugVariables.size();
    }
protected:
    inline hashMapImpl<I64, DebugVar>& getDebugVariables() {
        return _debugVariables;
    }
    inline hashMapImpl<I64, DebugGroup>& getDebugGroups() {
        return _debugGroups;
    }
protected:
    mutable SharedLock _varMutex;
    mutable SharedLock _groupMutex;
    hashMapImpl<I64, DebugVar> _debugVariables;
    hashMapImpl<I64, DebugGroup> _debugGroups;

END_SINGLETON

#define ADD_DEBUG_GROUP_CHILD(name, parentGUID) \
    DebugInterface::instance().addDebugGroup(name, parentGUID);

#define ADD_DEBUG_GROUP(name) \
    ADD_DEBUG_GROUP_CHILD(name, -1);

#define ADD_FILE_DEBUG_GROUP_CHILD(parentGUID) \
    ADD_DEBUG_GROUP_CHILD(__FILE__, parentGUID);

#define ADD_FILE_DEBUG_GROUP() \
    ADD_DEBUG_GROUP(__FILE__);

#define ADD_DEBUG_VAR(pointer, type, group, locked) \
    DebugInterface::instance().addDebugVar(__FILE__, __LINE__, pointer, type, group, locked);

#define ADD_DEBUG_VAR_FILE(pointer, type, locked) \
    ADD_DEBUG_VAR(pointer, type, DebugInterface::instance().getDebugGroup(__FILE__), locked);

namespace Attorney {
    class DebugInterfaceGUI {
    private:
        static hashMapImpl<I64, DebugInterface::DebugGroup>& getDebugGroups() {
            return DebugInterface::instance().getDebugGroups();
        }

        static hashMapImpl<I64, DebugInterface::DebugVar>& getDebugVariables() {
            return DebugInterface::instance().getDebugVariables();
        }

        static void lockVars(bool write) {
            if (write) {
                DebugInterface::instance()._varMutex.lock();
            } else {
                DebugInterface::instance()._varMutex.lock_shared();
            }
        }

        static void unlockVars() {
            DebugInterface::instance()._varMutex.unlock();
        }

        static void lockGroups(bool write) {
            if (write) {
                DebugInterface::instance()._groupMutex.lock();
            }
            else {
                DebugInterface::instance()._groupMutex.lock_shared();
            }
        }

        static void unlockGroups() {
            DebugInterface::instance()._groupMutex.unlock();
        }

        friend class GUI;
    };
};  // namespace Attorney

}; //namespace Divide
#endif //_DEBUG_INTERFACE_H_