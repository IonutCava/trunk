/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
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