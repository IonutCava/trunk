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

#ifndef _CORE_PARAM_HANDLER_H_
#define _CORE_PARAM_HANDLER_H_

#include "Console.h"
#include "cdigginsAny.h"
#include "Utility/Headers/Localization.h"
#include "Platform/Threading/Headers/SharedMutex.h"
#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

DEFINE_SINGLETON(ParamHandler)

    typedef hashMapImpl<ULL, cdiggins::any> ParamMap;
    /// A special map for string types (small perf. optimization for add/retrieve)
    typedef hashMapImpl<ULL, stringImpl> ParamStringMap;
    /// A special map for boolean types (small perf. optimization for add/retrieve)
    /// Used a lot as option toggles
    typedef hashMapImpl<ULL, bool> ParamBoolMap;
    /// Floats are also used often
    typedef hashMapImpl<ULL, F32> ParamFloatMap;

  public:
    void setDebugOutput(bool logState);

    template <typename T>
    T getParam(ULL paramID, T defaultValue = T()) const;

    template <typename T>
    T getParam(const char* name, T defaultValue = T()) const;

    template <typename T>
    void setParam(ULL paramID, const T& value);

    template <typename T>
    void setParam(const char* name, const T& value);

    template <typename T>
    void delParam(ULL paramID);

    template <typename T>
    void delParam(const char* name);

    template <typename T>
    bool isParam(ULL paramID) const;

    template <typename T>
    bool isParam(const char* param) const;

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
