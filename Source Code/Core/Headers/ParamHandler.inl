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

#ifndef _CORE_PARAM_HANDLER_INL_
#define _CORE_PARAM_HANDLER_INL_

namespace Divide {

inline void ParamHandler::setDebugOutput(bool logState) {
    _logState = logState;
}

template <typename T>
T ParamHandler::getParam(const stringImpl& name, T defaultValue) const {
    ReadLock r_lock(_mutex);
    ParamMap::const_iterator it = _params.find(name);
    if (it != std::end(_params)) {
        bool success = false;
        const T& ret = it->second.constant_cast<T>(success);
#ifdef _DEBUG
        if (!success) {
            Console::errorfn(Locale::get("ERROR_PARAM_CAST"), name.c_str());
            DIVIDE_ASSERT(success,
                          "ParamHandler error: Can't cast requested param to "
                          "specified type!");
        }
#endif

        return ret;
    }

    Console::errorfn(Locale::get("ERROR_PARAM_GET"), name.c_str());
    return defaultValue;  // integers will be 0, string will be empty, etc;
}

template <typename T>
void ParamHandler::setParam(const stringImpl& name, const T& value) {
    WriteLock w_lock(_mutex);
    ParamMap::iterator it = _params.find(name);
    if (it == std::end(_params)) {
        DIVIDE_ASSERT(emplace(_params, name, cdiggins::any(value)).second,
                      "ParamHandler error: can't add specified value to map!");
    } else {
        it->second = cdiggins::any(value);
    }
}

template <typename T>
void ParamHandler::delParam(const stringImpl& name) {
    if (isParam(name)) {
        WriteLock w_lock(_mutex);
        _params.erase(name);
        if (_logState) {
            Console::printfn(Locale::get("PARAM_REMOVE"), name.c_str());
        }
    } else {
        Console::errorfn(Locale::get("ERROR_PARAM_REMOVE"), name.c_str());
    }
}

template <typename T>
bool ParamHandler::isParam(const stringImpl& param) const {
    ReadLock r_lock(_mutex);
    return _params.find(param) != std::end(_params);
}

template <>
stringImpl ParamHandler::getParam(const stringImpl& name,
                                  stringImpl defaultValue) const {
    ReadLock r_lock(_mutex);
    ParamStringMap::const_iterator it = _paramsStr.find(name);
    if (it != std::end(_paramsStr)) {
        return it->second;
    }

    Console::errorfn(Locale::get("ERROR_PARAM_GET"), name.c_str());
    return defaultValue;
}

template <>
void ParamHandler::setParam(const stringImpl& name, const stringImpl& value) {
    WriteLock w_lock(_mutex);
    ParamStringMap::iterator it = _paramsStr.find(name);
    if (it == std::end(_paramsStr)) {
        DIVIDE_ASSERT(emplace(_paramsStr, name, value).second,
                      "ParamHandler error: can't add specified value to map!");
    } else {
        it->second = value;
    }
}

#if defined(STRING_IMP) && STRING_IMP != 1
template <>
std::string ParamHandler::getParam(const stringImpl& name,
                                   std::string defaultValue) const {
    return stringAlg::fromBase(
        getParam<stringImpl>(name, stringAlg::toBase(defaultValue)));
}

template <>
void ParamHandler::setParam(const stringImpl& name, const std::string& value) {
    setParam(name, stringImpl(value.c_str()));
}

template <>
void ParamHandler::delParam<std::string>(const stringImpl& name) {
    delParam<stringImpl>(name);
}
#endif

template <>
void ParamHandler::delParam<stringImpl>(const stringImpl& name) {
    if (isParam<stringImpl>(name)) {
        WriteLock w_lock(_mutex);
        _paramsStr.erase(name);
        if (_logState) {
            Console::printfn(Locale::get("PARAM_REMOVE"), name.c_str());
        }
    } else {
        Console::errorfn(Locale::get("ERROR_PARAM_REMOVE"), name.c_str());
    }
}

template <>
bool ParamHandler::isParam<stringImpl>(const stringImpl& param) const {
    ReadLock r_lock(_mutex);
    return _paramsStr.find(param) != std::end(_paramsStr);
}

template <>
bool ParamHandler::getParam(const stringImpl& name, bool defaultValue) const {
    ReadLock r_lock(_mutex);
    ParamBoolMap::const_iterator it = _paramBool.find(name);
    if (it != std::end(_paramBool)) {
        return it->second;
    }

    Console::errorfn(Locale::get("ERROR_PARAM_GET"), name.c_str());
    return defaultValue;
}

template <>
void ParamHandler::setParam(const stringImpl& name, const bool& value) {
    WriteLock w_lock(_mutex);
    ParamBoolMap::iterator it = _paramBool.find(name);
    if (it == std::end(_paramBool)) {
        DIVIDE_ASSERT(emplace(_paramBool, name, value).second,
                      "ParamHandler error: can't add specified value to map!");
    } else {
        it->second = value;
    }
}

template <>
inline void ParamHandler::delParam<bool>(const stringImpl& name) {
    if (isParam<stringImpl>(name)) {
        WriteLock w_lock(_mutex);
        _paramBool.erase(name);
        if (_logState) {
            Console::printfn(Locale::get("PARAM_REMOVE"), name.c_str());
        }
    } else {
        Console::errorfn(Locale::get("ERROR_PARAM_REMOVE"), name.c_str());
    }
}

template <>
bool ParamHandler::isParam<bool>(const stringImpl& param) const {
    ReadLock r_lock(_mutex);
    return _paramBool.find(param) != std::end(_paramBool);
}

};  // namespace Divide

#endif  //_CORE_PARAM_HANDLER_INL_