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

inline void ParamHandler::setDebugOutput(bool logState) noexcept {
    _logState = logState;
}

template <typename T>
inline bool ParamHandler::isParam(HashType paramID) const {
    SharedLock r_lock(_mutex);
    return _params.find(paramID) != std::cend(_params);
}

template <typename T>
inline T ParamHandler::getParam(HashType nameID, T defaultValue) const {
    SharedLock r_lock(_mutex);
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
    return defaultValue;
}

template <typename T>
inline void ParamHandler::setParam(HashType nameID, const T& value) {
    UniqueLockShared w_lock(_mutex);
    ParamMap::iterator it = _params.find(nameID);
    if (it == std::end(_params)) {
        bool result = hashAlg::emplace(_params, nameID, value).second;
        DIVIDE_ASSERT(result,"ParamHandler error: can't add specified value to map!");
    } else {
            it->second = AnyParam(value);
    }
}

template <typename T>
inline void ParamHandler::delParam(HashType nameID) {
    if (isParam<T>(nameID)) {
        UniqueLockShared w_lock(_mutex);
        _params.erase(nameID);
        if (_logState) {
            Console::printfn(Locale::get(_ID("PARAM_REMOVE")), nameID);
        }
    } else {
        Console::errorfn(Locale::get(_ID("ERROR_PARAM_REMOVE")), nameID);
    }
}

template <>
inline stringImpl ParamHandler::getParam(HashType paramID, stringImpl defaultValue) const {
    SharedLock r_lock(_mutex);
    const ParamStringMap::const_iterator it = _paramsStr.find(paramID);
    if (it != std::cend(_paramsStr)) {
        return it->second;
    }

    Console::errorfn(Locale::get(_ID("ERROR_PARAM_GET")), paramID);
    return defaultValue;
}

template <>
inline void ParamHandler::setParam(HashType paramID, const stringImpl& value) {
    UniqueLockShared w_lock(_mutex);
    const ParamStringMap::iterator it = _paramsStr.find(paramID);
    if (it == std::end(_paramsStr)) {
        bool result = hashAlg::insert(_paramsStr, paramID, value).second;
        DIVIDE_ASSERT(result, "ParamHandler error: can't add specified value to map!");
    } else {
        it->second = value;
    }
}

template <>
inline bool ParamHandler::isParam<stringImpl>(HashType paramID) const {
    SharedLock r_lock(_mutex);
    return _paramsStr.find(paramID) != std::cend(_paramsStr);
}


template <>
inline void ParamHandler::delParam<stringImpl>(HashType paramID) {
    if (isParam<stringImpl>(paramID)) {
        if (_logState) {
            Console::printfn(Locale::get(_ID("PARAM_REMOVE")), paramID);
        }
        UniqueLockShared w_lock(_mutex);
        _paramsStr.erase(paramID);
    } else {
        Console::errorfn(Locale::get(_ID("ERROR_PARAM_REMOVE")), paramID);
    }
}

template <>
inline bool ParamHandler::getParam(HashType paramID, bool defaultValue) const {
    SharedLock r_lock(_mutex);
    const ParamBoolMap::const_iterator it = _paramBool.find(paramID);
    if (it != std::cend(_paramBool)) {
        return it->second;
    }

    Console::errorfn(Locale::get(_ID("ERROR_PARAM_GET")), paramID);
    return defaultValue;
}

template <>
inline void ParamHandler::setParam(HashType paramID, const bool& value) {
    UniqueLockShared w_lock(_mutex);
    const ParamBoolMap::iterator it = _paramBool.find(paramID);
    if (it == std::end(_paramBool)) {
        bool result = hashAlg::emplace(_paramBool, paramID, value).second;
        DIVIDE_ASSERT(result, "ParamHandler error: can't add specified value to map!");
    } else {
        it->second = value;
    }
}

template <>
inline bool ParamHandler::isParam<bool>(HashType paramID) const {
    SharedLock r_lock(_mutex);
    return _paramBool.find(paramID) != std::cend(_paramBool);
}

template <>
inline void ParamHandler::delParam<bool>(HashType paramID) {
    if (isParam<bool>(paramID)) {
        UniqueLockShared w_lock(_mutex);
        _paramBool.erase(paramID);
        if (_logState) {
            Console::printfn(Locale::get(_ID("PARAM_REMOVE")), paramID);
        }
    } else {
        Console::errorfn(Locale::get(_ID("ERROR_PARAM_REMOVE")), paramID);
    }
}

template <>
inline F32 ParamHandler::getParam(HashType paramID, F32 defaultValue) const {
    SharedLock r_lock(_mutex);
    const ParamFloatMap::const_iterator it = _paramsFloat.find(paramID);
    if (it != std::cend(_paramsFloat)) {
        return it->second;
    }

    Console::errorfn(Locale::get(_ID("ERROR_PARAM_GET")), paramID);
    return defaultValue;
}

template <>
inline void ParamHandler::setParam(HashType paramID, const F32& value) {
    UniqueLockShared w_lock(_mutex);
    const ParamFloatMap::iterator it = _paramsFloat.find(paramID);
    if (it == std::end(_paramsFloat)) {
        bool result = hashAlg::emplace(_paramsFloat, paramID, value).second;
        DIVIDE_ASSERT(result, "ParamHandler error: can't add specified value to map!");
    } else {
        it->second = value;
    }
}

template <>
inline bool ParamHandler::isParam<F32>(HashType paramID) const {
    SharedLock r_lock(_mutex);
    return _paramsFloat.find(paramID) != std::cend(_paramsFloat);
}

template <>
inline void ParamHandler::delParam<F32>(HashType paramID) {
    if (isParam<F32>(paramID)) {
        UniqueLockShared w_lock(_mutex);
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