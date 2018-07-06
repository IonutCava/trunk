#ifndef PARAM_H_
#define PARAM_H_
/******************************************************************************
 *   <ParamHandler: Adds a parameter database for the entire application>     *
 *   Copyright (C) <2010>  <Cava Ionut - Divide Studio>                       *
 *                                                                            *
 *   This program is free software: you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License as published by     *
 *   the Free Software Foundation, either version 3 of the License, or        *
 *   (at your option) any later version.                                      *
 *                                                                            *
 *   This program is distributed in the hope that it will be useful,          *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU General Public License for more details.                             *
 *                                                                            *
 *   You should have received a copy of the GNU General Public License        *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                            *
 ******************************************************************************/
#include "resource.h"
#include "Utility/Headers/Singleton.h"
#include <boost/any.hpp>
#include <boost/thread/thread.hpp>

using boost::any_cast;

SINGLETON_BEGIN (ParamHandler)

public:

	template <class T>	
	T getParam(const string& name)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		params::iterator it = _params.find(name);
		if(it != _params.end())
			return it->second;
		else
			return NULL;
	}

	template<>
	F32 getParam<F32>(const string& name)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		params::iterator it = _params.find(name);
		if(it != _params.end())
		{
			try
			{
				return any_cast<F32>(it->second);
			}
			catch(const boost::bad_any_cast &)
			{
				return 0;
			}
		}
		else return 0;
	}

	template<>
	D32 getParam<D32>(const string& name)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		params::iterator it = _params.find(name);
		if(it != _params.end())
		{
			try
			{
				return any_cast<D32>(it->second);
			}
			catch(const boost::bad_any_cast &)
			{
				return 0;
			}
		}
		else return 0;
	}

	template<>
	U32 getParam<U32>(const string& name)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		params::iterator it = _params.find(name);
		if(it != _params.end())
		{
			try
			{
				return any_cast<U32>(it->second);
			}
			catch(const boost::bad_any_cast &)
			{
				cout << "ParamHandler: error casting [ " << name << " ] to U32" << endl;
				return 0;
			}
		}
		else return 0;
	}

	template<>
	int getParam<int>(const string& name)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		params::iterator it = _params.find(name);
		if(it != _params.end())
		{
			try
			{
				return any_cast<int>(it->second);
			}
			catch(const boost::bad_any_cast &)
			{
				return 0;
			}
		}
		else return 0;
	}

	template<>
	bool getParam<bool>(const string& name)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		params::iterator it = _params.find(name);
		if(it != _params.end())
		{
			try
			{
				return any_cast<bool>(it->second);
			}
			catch(const boost::bad_any_cast &)
			{
				return false;
			}
		}
		else return false;
	}

	template<>
	const char* getParam<const char*>(const string& name)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		params::iterator it = _params.find(name);
		if(it != _params.end())
		{
			try
			{
				return any_cast<const char*>(it->second);
			}
			catch(const boost::bad_any_cast &)
			{
				return "ParamHandler: Error! not found";
			}
		}
		else return "ParamHandler: Error! not found";
	}

	template<>
	string getParam<string>(const string& name)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		params::iterator it = _params.find(name);
		if(it != _params.end())
		{
			try
			{
				return any_cast<string>(it->second);
			}
			catch(const boost::bad_any_cast &)
			{
				return "ParamHandler: Error! not found";
			}
		}
		else return "ParamHandler: Error! not found";
	}

	void setParam(const string& name, const boost::any& value)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		_result = _params.insert(pair<string,boost::any>(name,value));
		if(!_result.second) (_result.first)->second = value;
		if (_logState) printOutput(name,value,_result.second);

	}

	void delParam(const string& name)
	{
		_params.erase(name); 
		if(_logState) Con::getInstance().printfn("ParamHandler: Removed saved parameter [ %s ]", name.c_str());
	} 

	inline void setDebugOutput(bool logState) {_logState = logState;}

	int getSize(){return _params.size();}

private: 
	void printOutput(const string& name, const boost::any& value,bool inserted)
	{
		string param = "ParamHandler: Updated param \""+name+"\" : ";
		if(inserted)  param = "ParamHandler: Saved param \""+name+"\" : ";
		Con::getInstance().printf("%s",param.c_str());
		if(value.type() == typeid(F32))				    Con::getInstance().printf("%f",any_cast<F32>(value));
		else if(value.type() == typeid(D32))			Con::getInstance().printf("%f",any_cast<D32>(value));
		else if(value.type() == typeid(bool))			Con::getInstance().printf("%s",any_cast<bool>(value)? "true" : "false");
		else if(value.type() == typeid(int))			Con::getInstance().printf("%d",any_cast<I32>(value));
		else if(value.type() == typeid(U32))			Con::getInstance().printf("%d",any_cast<U32>(value));
		else if(value.type() == typeid(string))  		Con::getInstance().printf("%s",any_cast<string>(value).c_str());
		else if(value.type() == typeid(const char*))	Con::getInstance().printf("%s",any_cast<const char*>(value));
		else Con::getInstance().printf("unconvertible %s",value.type().name());
		Con::getInstance().printf("\n");
	}
private:
	bool _logState;
	tr1::unordered_map<string, boost::any> _params;
	typedef std::tr1::unordered_map<string, boost::any> params;
	pair<tr1::unordered_map<string, boost::any>::iterator, bool> _result;
	boost::mutex mutex_;
 
SINGLETON_END()

#endif