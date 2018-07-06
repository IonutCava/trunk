/*
   Copyright (c) 2013 DIVIDE-Studio
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
#include <typeinfo>
#include "cdigginsAny.h"
#include "Utility/Headers/UnorderedMap.h"
#include "Utility/Headers/Localization.h"
#include "Hardware/Platform/Headers/SharedMutex.h"
#include "Hardware/Platform/Headers/PlatformDefines.h"

DEFINE_SINGLETON (ParamHandler)
typedef Unordered_map<std::string, cdiggins::any> ParamMap;
typedef Unordered_map<std::string, const char* >  ParamTypeMap;
typedef cdiggins::anyimpl::bad_any_cast BadAnyCast;
public:

	template <class T>
	T getParam(const std::string& name, const T& defaultValue = T()) {
		ReadLock r_lock(_mutex);
		ParamMap::iterator it = _params.find(name);
#ifdef _DEBUG
		if(it != _params.end()){
			try	{
				return it->second.cast<T>();
			}catch(BadAnyCast){
				ERROR_FN(Locale::get("ERROR_PARAM_CAST"),name.c_str(),typeid(T).name());
			}
		}
#else
	if(it != _params.end()){
		return it->second.cast<T>();
	}
#endif
		return defaultValue; //integrals will be 0, string will be empty, etc;
	}

	void setParam(const std::string& name, const cdiggins::any& value){
		WriteLock w_lock(_mutex);
		std::pair<ParamMap::iterator, bool> result = _params.insert(std::make_pair(name,value));
		if(!result.second) (result.first)->second = value;
	}

	inline void delParam(const std::string& name){
		if(isParam(name)){
			WriteLock w_lock(_mutex);
			_params.erase(name);
			if(_logState) PRINT_FN(Locale::get("PARAM_REMOVE"), name.c_str());
		}else{
			ERROR_FN(Locale::get("ERROR_PARAM_REMOVE"),name.c_str());
		}
	}

	inline void setDebugOutput(const bool logState) {WriteLock w_lock(_mutex); _logState = logState;}

	inline U32 getSize() const {ReadLock r_lock(_mutex); return _params.size();}

	inline bool isParam(const std::string& param) const {ReadLock r_lock(_mutex); return _params.find(param) != _params.end();}

private:
	bool _logState;
	ParamMap _params;
	mutable SharedLock _mutex;

END_SINGLETON

#endif