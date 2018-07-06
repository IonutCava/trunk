/*“Copyright 2009-2012 DIVIDE-Studio”*/
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

#ifndef PARAM_H_
#define PARAM_H_

#include "core.h"
#include <boost/any.hpp>

DEFINE_SINGLETON (ParamHandler)
typedef unordered_map<std::string, boost::any> ParamMap;

public:

	template <class T>	
	T getParam(const std::string& name){
		ReadLock r_lock(_mutex);
		ParamMap::iterator it = _params.find(name);
		if(it != _params.end()){
			try	{
				return boost::any_cast<T>(it->second);
			}catch(const boost::bad_any_cast &){
				ERROR_FN(Locale::get("ERROR_PARAM_CAST"),name.c_str(),typeid(T).name());
			}
		}
		return T(); ///integrals will be 0, string will be empty, etc;
	}

	void setParam(const std::string& name, const boost::any& value){
		WriteLock w_lock(_mutex);
		std::pair<ParamMap::iterator, bool> result = _params.insert(std::make_pair(name,value));
		if(!result.second) (result.first)->second = value;
		if (_logState) printOutput(name,value,result.second);

	}

	inline void delParam(const std::string& name){
		WriteLock w_lock(_mutex);
		_params.erase(name); 
		if(_logState) PRINT_FN(Locale::get("PARAM_REMOVE"), name.c_str());
	} 

	inline void setDebugOutput(bool logState) {WriteLock w_lock(_mutex); _logState = logState;}

	inline int getSize(){ReadLock r_lock(_mutex); return _params.size();}

private: 
	void printOutput(const std::string& name, const boost::any& value,bool inserted);

private:
	bool _logState;
	ParamMap _params;
	mutable Lock _mutex;
 
END_SINGLETON

#endif