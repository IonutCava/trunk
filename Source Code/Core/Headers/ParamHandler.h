/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _CORE_PARAM_HANDLER_H_
#define _CORE_PARAM_HANDLER_H_

#include "Console.h"
#include "cdigginsAny.h"
#include "Utility/Headers/HashMap.h"
#include "Utility/Headers/Localization.h"
#include "Platform/Threading/Headers/SharedMutex.h"
#include "Platform/Platform/Headers/PlatformDefines.h"

namespace Divide {

DEFINE_SINGLETON(ParamHandler)

    typedef hashMapImpl<stringImpl, cdiggins::any> ParamMap;
    /// A special map for string types (small perf. optimization for add/retrieve)
    typedef hashMapImpl<stringImpl, stringImpl> ParamStringMap;
    /// A special map for boolean types (small perf. optimization for add/retrieve)
    /// Used a lot as option toggles
    typedef hashMapImpl<stringImpl, bool> ParamBoolMap;
    /// Floats are also used often
    typedef hashMapImpl<stringImpl, F32> ParamFloatMap;

  public:
    void setDebugOutput(bool logState);

    template <typename T>
    T getParam(const stringImpl& name, T defaultValue = T()) const;

    template <typename T>
    void setParam(const stringImpl& name, const T& value);

    template <typename T>
    void delParam(const stringImpl& name);

    template <typename T>
    bool isParam(const stringImpl& param) const;

    template <>
    stringImpl getParam(const stringImpl& name, stringImpl defaultValue) const;

    template <>
    void setParam(const stringImpl& name, const stringImpl& value);

    #if defined(STRING_IMP) && STRING_IMP != 1
    template <>
    std::string getParam(const stringImpl& name, std::string defaultValue) const;

    template <>
    void setParam(const stringImpl& name, const std::string& value);

    template <>
    void delParam<std::string>(const stringImpl& name);
    #endif

    template <>
    void delParam<stringImpl>(const stringImpl& name);

    template <>
    bool isParam<stringImpl>(const stringImpl& param) const;

    template <>
    bool getParam(const stringImpl& name, bool defaultValue) const;

    template <>
    void setParam(const stringImpl& name, const bool& value);

    template <>
    void delParam<bool>(const stringImpl& name);

    template <>
    bool isParam<bool>(const stringImpl& param) const;

    template <>
    F32 getParam(const stringImpl& name, F32 defaultValue) const;

    template <>
    void setParam(const stringImpl& name, const F32& value);

    template <>
    void delParam<F32>(const stringImpl& name);

    template <>
    bool isParam<F32>(const stringImpl& param) const;

  private:
    ParamMap _params;
    ParamBoolMap _paramBool;
    ParamStringMap _paramsStr;
    ParamFloatMap _paramsFloat;
    mutable SharedLock _mutex;
    std::atomic_bool _logState;

END_SINGLETON

};  // namespace Divide

#endif  //_CORE_PARAM_HANDLER_H_

#include "ParamHandler.inl"