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
#include <unordered_map>

using namespace std;
using boost::any_cast;

SINGLETON_BEGIN (ParamHandler)

public:

	template <class T>	
	T getParam(string name)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		params::iterator it = _params.find(name);
		if(it != _params.end())
			return it->second;
		else
			return NULL;
	}

	template<>
	F32 getParam<F32>(string name)
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
	D32 getParam<D32>(string name)
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
	U32 getParam<U32>(string name)
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
	int getParam<int>(string name)
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
	bool getParam<bool>(string name)
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
	const char* getParam<const char*>(string name)
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
	string getParam<string>(string name)
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

	void setParam(string name, const boost::any& value)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		_result = _params.insert(pair<string,boost::any>(name,value));
		if(!_result.second) (_result.first)->second = value;
		if (_logState) printOutput(name,value,_result.second);

	}

	void delParam(string name)
	{
		_params.erase(name); 
		if(_logState) cout << "ParamHandler: Removed saved parameter \"" << name << "\"" << endl;
	} 

	inline void setDebugOutput(bool logState) {_logState = logState;}

	int getSize(){return _params.size();}

private: 
	void printOutput(const string& name, const boost::any& value,bool inserted)
	{
		string param = "ParamHandler: Updated param \""+name+"\" : ";
		if(inserted)  param = "ParamHandler: Saved param \""+name+"\" : ";
		cout << param;
		if(value.type() == typeid(F32))				cout << any_cast<F32>(value);
		else if(value.type() == typeid(D32))			cout << any_cast<D32>(value);
		else if(value.type() == typeid(bool))			cout << any_cast<bool>(value);
		else if(value.type() == typeid(int))			cout << any_cast<int>(value);
		else if(value.type() == typeid(U32))	cout << any_cast<U32>(value);
		else if(value.type() == typeid(string))  		cout << any_cast<string>(value);
		else if(value.type() == typeid(const char*))	cout << any_cast<const char*>(value);
		else cout << "unconvertible" << value.type().name();
		cout << endl;
	}
private:
	bool _logState;
	tr1::unordered_map<string, boost::any> _params;
	typedef std::tr1::unordered_map<string, boost::any> params;
	pair<tr1::unordered_map<string, boost::any>::iterator, bool> _result;
	boost::mutex mutex_;
 
SINGLETON_END()

#endif