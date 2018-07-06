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

#include "resource.h"
#include <boost/any.hpp>
#include <boost/thread/thread.hpp>

using boost::any_cast;

DEFINE_SINGLETON (ParamHandler)
typedef unordered_map<std::string, boost::any> ParamMap;

public:

	template <class T>	
	T getParam(const std::string& name);

	void setParam(const std::string& name, const boost::any& value){
		_mutex.lock();
		std::pair<ParamMap::iterator, bool> result = _params.insert(make_pair(name,value));
		if(!result.second) (result.first)->second = value;
		if (_logState) printOutput(name,value,result.second);
		_mutex.unlock();

	}

	void delParam(const std::string& name){
		_params.erase(name); 
		if(_logState) Console::getInstance().printfn("ParamHandler: Removed saved parameter [ %s ]", name.c_str());
	} 

	inline void setDebugOutput(bool logState) {_logState = logState;}

	int getSize(){return _params.size();}

private: 
	void printOutput(const std::string& name, const boost::any& value,bool inserted);

private:
	bool _logState;
	ParamMap _params;
	boost::mutex _mutex;
 
END_SINGLETON

#endif