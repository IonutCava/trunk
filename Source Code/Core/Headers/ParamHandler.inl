/*
   Copyright (c) 2018 DIVIDE-Studio
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

#ifndef _CORE_PARAM_HANDLER_INL_
#define _CORE_PARAM_HANDLER_INL_

namespace Divide {

inline void ParamHandler::setDebugOutput(bool logState) {
    _logState = logState;
}

template <typename T>
inline bool ParamHandler::isParam(U64 paramID) const {
    ReadLock r_lock(_mutex);
    return _params.find(paramID) != std::cend(_params);
}

template <typename T>
inline bool ParamHandler::isParam(const char* param) const {
    return isParam<T>(_ID_RT(param));
}

template <typename T>
inline T ParamHandler::getParam(const char* name, T defaultValue) const {
    STUBBED("Please don't use strings directly for indexing with the ParamHandler");
    return getParam<T>(_ID_RT(name));
}

template <typename T>
inline T ParamHandler::getParam(U64 nameID, T defaultValue) const {
    ReadLock r_lock(_mutex);
    ParamMap::const_iterator it = _params.find(nameID);
    if (it != std::cend(_params)) {
        bool success = false;
#       if !defined(CPP_17_SUPPORT)
        const T& ret = it->second.constant_cast<T>(success);
#       else
        const T& ret = std::any_cast<T>(it->second);
        success = true;
#       endif
        if (Config::Build::IS_DEBUG_BUILD) {
            if (!success) {
                Console::errorfn(Locale::get(_ID("ERROR_PARAM_CAST")), nameID);
                DIVIDE_ASSERT(success,
                              "ParamHandler error: Can't cast requested param to "
                              "specified type!");
            }
        }

        return ret;
    }

    Console::errorfn(Locale::get(_ID("ERROR_PARAM_GET")), nameID);
    return defaultValue;  // integers will be 0, string will be empty, etc;
}

template <typename T>
inline void ParamHandler::setParam(const char* name, const T& value) {
    STUBBED("Please don't use strings directly for indexing with the ParamHandler");
    setParam<T>(_ID_RT(name), value);
}

template <typename T>
inline void ParamHandler::setParam(U64 nameID, const T& value) {
    WriteLock w_lock(_mutex);
    ParamMap::iterator it = _params.find(nameID);
    if (it == std::end(_params)) {
        bool result = hashAlg::emplace(_params, nameID, value).second;
        DIVIDE_ASSERT(result,"ParamHandler error: can't add specified value to map!");
    } else {
            it->second = AnyParam(value);
    }
}

template <typename T>
inline void ParamHandler::delParam(const char* name) {
    delParam<T>(_ID_RT(name));
}

template <typename T>
inline void ParamHandler::delParam(U64 nameID) {
    if (isParam<T>(nameID)) {
        WriteLock w_lock(_mutex);
        _params.erase(nameID);
        if (_logState) {
            Console::printfn(Locale::get(_ID("PARAM_REMOVE")), nameID);
        }
    } else {
        Console::errorfn(Locale::get(_ID("ERROR_PARAM_REMOVE")), nameID);
    }
}

template <>
inline stringImpl ParamHandler::getParam(U64 paramID, stringImpl defaultValue) const {
    ReadLock r_lock(_mutex);
    ParamStringMap::const_iterator it = _paramsStr.find(paramID);
    if (it != std::cend(_paramsStr)) {
        return it->second;
    }

    Console::errorfn(Locale::get(_ID("ERROR_PARAM_GET")), paramID);
    return defaultValue;
}

template <>
inline void ParamHandler::setParam(U64 paramID, const stringImpl& value) {
    WriteLock w_lock(_mutex);
    ParamStringMap::iterator it = _paramsStr.find(paramID);
    if (it == std::end(_paramsStr)) {
        DIVIDE_ASSERT(hashAlg::emplace(_paramsStr, paramID, value).second,
                      "ParamHandler error: can't add specified value to map!");
    } else {
        it->second = value;
    }
}

#if defined(USE_CUSTOM_MEMORY_ALLOCATORS)
template <>
inline std::string ParamHandler::getParam(U64 paramID, std::string defaultValue) const {
    return getParam<stringImpl>(paramID, defaultValue.c_str()).c_str();
}
template <>
inline void ParamHandler::setParam(U64 paramID, const std::string& value) {
    setParam<stringImpl>(paramID, value.c_str());
}

#endif

template <>
inline bool ParamHandler::isParam<stringImpl>(U64 paramID) const {
    ReadLock r_lock(_mutex);
    return _paramsStr.find(paramID) != std::cend(_paramsStr);
}


template <>
inline void ParamHandler::delParam<stringImpl>(U64 paramID) {
    if (isParam<stringImpl>(paramID)) {
        WriteLock w_lock(_mutex);
        _paramsStr.erase(paramID);
        if (_logState) {
            Console::printfn(Locale::get(_ID("PARAM_REMOVE")), paramID);
        }
    } else {
        Console::errorfn(Locale::get(_ID("ERROR_PARAM_REMOVE")), paramID);
    }
}

template <>
inline bool ParamHandler::getParam(U64 paramID, bool defaultValue) const {
    ReadLock r_lock(_mutex);
    ParamBoolMap::const_iterator it = _paramBool.find(paramID);
    if (it != std::cend(_paramBool)) {
        return it->second;
    }

    Console::errorfn(Locale::get(_ID("ERROR_PARAM_GET")), paramID);
    return defaultValue;
}

template <>
inline void ParamHandler::setParam(U64 paramID, const bool& value) {
    WriteLock w_lock(_mutex);
    ParamBoolMap::iterator it = _paramBool.find(paramID);
    if (it == std::end(_paramBool)) {
        DIVIDE_ASSERT(hashAlg::emplace(_paramBool, paramID, value).second,
                      "ParamHandler error: can't add specified value to map!");
    } else {
        it->second = value;
    }
}

template <>
inline bool ParamHandler::isParam<bool>(U64 paramID) const {
    ReadLock r_lock(_mutex);
    return _paramBool.find(paramID) != std::cend(_paramBool);
}

template <>
inline void ParamHandler::delParam<bool>(U64 paramID) {
    if (isParam<bool>(paramID)) {
        WriteLock w_lock(_mutex);
        _paramBool.erase(paramID);
        if (_logState) {
            Console::printfn(Locale::get(_ID("PARAM_REMOVE")), paramID);
        }
    } else {
        Console::errorfn(Locale::get(_ID("ERROR_PARAM_REMOVE")), paramID);
    }
}

template <>
inline F32 ParamHandler::getParam(U64 paramID, F32 defaultValue) const {
    ReadLock r_lock(_mutex);
    ParamFloatMap::const_iterator it = _paramsFloat.find(paramID);
    if (it != std::cend(_paramsFloat)) {
        return it->second;
    }

    Console::errorfn(Locale::get(_ID("ERROR_PARAM_GET")), paramID);
    return defaultValue;
}

template <>
inline void ParamHandler::setParam(U64 paramID, const F32& value) {
    WriteLock w_lock(_mutex);
    ParamFloatMap::iterator it = _paramsFloat.find(paramID);
    if (it == std::end(_paramsFloat)) {
        DIVIDE_ASSERT(hashAlg::emplace(_paramsFloat, paramID, value).second,
                      "ParamHandler error: can't add specified value to map!");
    } else {
        it->second = value;
    }
}

template <>
inline bool ParamHandler::isParam<F32>(U64 paramID) const {
    ReadLock r_lock(_mutex);
    return _paramsFloat.find(paramID) != std::cend(_paramsFloat);
}

template <>
inline void ParamHandler::delParam<F32>(U64 paramID) {
    if (isParam<F32>(paramID)) {
        WriteLock w_lock(_mutex);
        _paramsFloat.erase(paramID);
        if (_logState) {
            Console::printfn(Locale::get(_ID("PARAM_REMOVE")), paramID);
        }
    } else {
        Console::errorfn(Locale::get(_ID("ERROR_PARAM_REMOVE")), paramID);
    }
}

};  // namespace Divide

#endif  //_CORE_PARAM_HANDLER_INL_