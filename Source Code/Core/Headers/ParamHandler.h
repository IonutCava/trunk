/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _PARAM_HANDLER_H_
#define _PARAM_HANDLER_H_

#include "Console.h"
#include "cdigginsAny.h"
#include "Utility/Headers/HashMap.h"
#include "Utility/Headers/Localization.h"
#include "Hardware/Platform/Headers/SharedMutex.h"
#include "Hardware/Platform/Headers/PlatformDefines.h"

namespace Divide {

DEFINE_SINGLETON (ParamHandler)
typedef hashMapImpl<stringImpl, cdiggins::any> ParamMap;
/// A special map for string types (small perf. optimization for add/retrieve)
typedef hashMapImpl<stringImpl, stringImpl >   ParamStringMap;
/// A special map for boolean types (small perf. optimization for add/retrieve. Used a lot as option toggles)
typedef hashMapImpl<stringImpl, bool >         ParamBoolMap;

public:
	inline void setDebugOutput(bool logState) {
		_logState = logState;
	}

	template <typename T>
	inline T getParam(const stringImpl& name, T defaultValue = T()) const {
		ReadLock r_lock(_mutex);
		ParamMap::const_iterator it = _params.find(name);
		if(it != _params.end()) {
            bool success = false;
		    const T& ret = it->second.constant_cast<T>(success);
#           ifdef _DEBUG		
		        if (!success) {
			        ERROR_FN(Locale::get("ERROR_PARAM_CAST"),name.c_str());
					DIVIDE_ASSERT(success, "ParamHandler error: Can't cast requested param to specified type!");
		        } 
#           endif

		    return ret;
		} 
		
		ERROR_FN(Locale::get("ERROR_PARAM_GET"), name.c_str());
	    return defaultValue; //integers will be 0, string will be empty, etc;
	}

    template<typename T>
	void setParam(const stringImpl& name, const T& value) {
		WriteLock w_lock(_mutex);
		ParamMap::iterator it = _params.find(name); 
        if (it == _params.end()) {
			DIVIDE_ASSERT(emplace(_params, name, cdiggins::any(value)).second, "ParamHandler error: can't add specified value to map!");
        } else {
			it->second = cdiggins::any(value);
        }
	}

	template<typename T>
	inline void delParam(const stringImpl& name) {
		if (isParam(name)) {
			WriteLock w_lock(_mutex);
			_params.erase(name);
			if (_logState) {
                PRINT_FN(Locale::get("PARAM_REMOVE"), name.c_str());
            }
		} else {
			ERROR_FN(Locale::get("ERROR_PARAM_REMOVE"),name.c_str());
		}
	}

	template<typename T>
	inline bool isParam(const stringImpl& param) const {
		ReadLock r_lock(_mutex);
		return _params.find(param) != _params.end();
	}

	template<>
	inline stringImpl getParam(const stringImpl& name, stringImpl defaultValue) const {
		ReadLock r_lock(_mutex);
		ParamStringMap::const_iterator it = _paramsStr.find(name);
		if (it != _paramsStr.end()) {
			return it->second;
		}

		ERROR_FN(Locale::get("ERROR_PARAM_GET"), name.c_str());
		return defaultValue;
	}

	template<>
	void setParam(const stringImpl& name, const stringImpl& value) {
		WriteLock w_lock(_mutex);
		ParamStringMap::iterator it = _paramsStr.find(name);
		if (it == _paramsStr.end()) {
			DIVIDE_ASSERT(emplace(_paramsStr, name, value).second, "ParamHandler error: can't add specified value to map!");
		} else {
			it->second = value;
		}
	}

#if defined(STRING_IMP) && STRING_IMP != 1
	template<>
	inline std::string getParam(const stringImpl& name, std::string defaultValue) const {
		return stringAlg::fromBase(getParam<stringImpl>(name, stringAlg::toBase(defaultValue)));
	}

	template<>
	inline void setParam(const stringImpl& name, const std::string& value) {
		setParam(name, stringImpl(value.c_str()));
	}
	
	template<>
	inline void delParam<std::string>(const stringImpl& name) {
		delParam<stringImpl>(name);
	}
#endif

	template<>
	inline void delParam<stringImpl>(const stringImpl& name) {
		if (isParam<stringImpl>(name)) {
			WriteLock w_lock(_mutex);
			_paramsStr.erase(name);
			if (_logState) {
				PRINT_FN(Locale::get("PARAM_REMOVE"), name.c_str());
			}
		} else {
			ERROR_FN(Locale::get("ERROR_PARAM_REMOVE"), name.c_str());
		}
	}

	template<>
	inline bool isParam<stringImpl>(const stringImpl& param) const {
		ReadLock r_lock(_mutex);
		return _paramsStr.find(param) != _paramsStr.end();
	}

	template<>
	inline bool getParam(const stringImpl& name, bool defaultValue) const {
		ReadLock r_lock(_mutex);
		ParamBoolMap::const_iterator it = _paramBool.find(name);
		if (it != _paramBool.end()) {
			return it->second;
		}

		ERROR_FN(Locale::get("ERROR_PARAM_GET"), name.c_str());
		return defaultValue;
	}

	template<>
	void setParam(const stringImpl& name, const bool& value) {
		WriteLock w_lock(_mutex);
		ParamBoolMap::iterator it = _paramBool.find(name);
		if (it == _paramBool.end()) {
			DIVIDE_ASSERT(emplace(_paramBool, name, value).second, "ParamHandler error: can't add specified value to map!");
		} else {
			it->second = value;
		}
	}

	template<>
	inline void delParam<bool>(const stringImpl& name) {
		if (isParam<stringImpl>(name)) {
			WriteLock w_lock(_mutex);
			_paramBool.erase(name);
			if (_logState) {
				PRINT_FN(Locale::get("PARAM_REMOVE"), name.c_str());
			}
		} else {
			ERROR_FN(Locale::get("ERROR_PARAM_REMOVE"), name.c_str());
		}
	}

	template<>
	inline bool isParam<bool>(const stringImpl& param) const {
		ReadLock r_lock(_mutex);
		return _paramBool.find(param) != _paramBool.end();
	}

private:
	ParamMap _params;
	ParamBoolMap _paramBool;
	ParamStringMap _paramsStr;
	mutable SharedLock _mutex;
	std::atomic_bool _logState;

END_SINGLETON

}; //namespace Divide

#endif