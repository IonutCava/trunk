/*“Copyright 2009-2011 DIVIDE-Studio”*/
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
#include "Utility/Headers/Singleton.h"
#include <boost/any.hpp>
#include <boost/thread/thread.hpp>

using boost::any_cast;

DEFINE_SINGLETON (ParamHandler)
	typedef std::tr1::unordered_map<std::string, boost::any> ParamMap;
public:

	template <class T>	
	T getParam(const std::string& name)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		params::iterator it = _params.find(name);
		if(it != _params.end())
			return it->second;
		else
			return NULL;
	}

	template<>
	F32 getParam<F32>(const std::string& name)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		ParamMap::iterator it = _params.find(name);
		if(it != _params.end())
		{
			try
			{
				F32 temp = any_cast<F32>(it->second);
				return temp;
			}
			catch(const boost::bad_any_cast &)
			{
				return getParam<D32>(name); //Float and double are interchangable;
				
			}
		}
		else return 0;
	}

	template<>
	D32 getParam<D32>(const std::string& name)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		ParamMap::iterator it = _params.find(name);
		if(it != _params.end())
		{
			try
			{
				return any_cast<D32>(it->second);
			}
			catch(const boost::bad_any_cast &)
			{
				return getParam<F32>(name);
			}
		}
		else return 0;
	}

	template<>
	U8 getParam<U8>(const std::string& name)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		ParamMap::iterator it = _params.find(name);
		if(it != _params.end())
		{
			try
			{
				return any_cast<U8>(it->second);
			}
			catch(const boost::bad_any_cast &)
			{
				Console::getInstance().printfn("ParamHandler: error casting [ %s ] to U8",name.c_str());
				return 0;
			}
		}
		else return 0;
	}

	template<>
	U16 getParam<U16>(const std::string& name)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		ParamMap::iterator it = _params.find(name);
		if(it != _params.end())
		{
			try
			{
				return any_cast<U16>(it->second);
			}
			catch(const boost::bad_any_cast &)
			{
				Console::getInstance().printfn("ParamHandler: error casting [ %s ] to U16",name.c_str());
				return 0;
			}
		}
		else return 0;
	}

	template<>
	U32 getParam<U32>(const std::string& name)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		ParamMap::iterator it = _params.find(name);
		if(it != _params.end())
		{
			try
			{
				return any_cast<U32>(it->second);
			}
			catch(const boost::bad_any_cast &)
			{
				Console::getInstance().printfn("ParamHandler: error casting [ %s ] to U32",name.c_str());
				return 0;
			}
		}
		else return 0;
	}

		template<>
	I8 getParam<I8>(const std::string& name)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		ParamMap::iterator it = _params.find(name);
		if(it != _params.end())
		{
			try
			{
				return any_cast<I8>(it->second);
			}
			catch(const boost::bad_any_cast &)
			{
				return 0;
			}
		}
		else return 0;
	}

	template<>
	I16 getParam<I16>(const std::string& name)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		ParamMap::iterator it = _params.find(name);
		if(it != _params.end())
		{
			try
			{
				return any_cast<I16>(it->second);
			}
			catch(const boost::bad_any_cast &)
			{
				return 0;
			}
		}
		else return 0;
	}

	template<>
	I32 getParam<I32>(const std::string& name)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		ParamMap::iterator it = _params.find(name);
		if(it != _params.end())
		{
			try
			{
				return any_cast<I32>(it->second);
			}
			catch(const boost::bad_any_cast &)
			{
				return 0;
			}
		}
		else return 0;
	}

	template<>
	bool getParam<bool>(const std::string& name)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		ParamMap::iterator it = _params.find(name);
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
	const char* getParam<const char*>(const std::string& name)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		ParamMap::iterator it = _params.find(name);
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
	std::string getParam<std::string>(const std::string& name)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		ParamMap::iterator it = _params.find(name);
		if(it != _params.end())
		{
			try
			{
				return any_cast<std::string>(it->second);
			}
			catch(const boost::bad_any_cast &)
			{
				return "ParamHandler: Error! not found";
			}
		}
		else return "ParamHandler: Error! not found";
	}

	void setParam(const std::string& name, const boost::any& value)
	{
		boost::mutex::scoped_lock  lock(mutex_);
		std::pair<ParamMap::iterator, bool> result = _params.insert(make_pair(name,value));
		if(!result.second) (result.first)->second = value;
		if (_logState) printOutput(name,value,result.second);

	}

	void delParam(const std::string& name)
	{
		_params.erase(name); 
		if(_logState) Console::getInstance().printfn("ParamHandler: Removed saved parameter [ %s ]", name.c_str());
	} 

	inline void setDebugOutput(bool logState) {_logState = logState;}

	int getSize(){return _params.size();}

private: 
	void printOutput(const std::string& name, const boost::any& value,bool inserted)
	{
		std::string param = "ParamHandler: Updated param \""+name+"\" : ";
		if(inserted)  param = "ParamHandler: Saved param \""+name+"\" : ";
		Console::getInstance().printf("%s",param.c_str());
		if(value.type() == typeid(F32))				    Console::getInstance().printf("%f",any_cast<F32>(value));
		else if(value.type() == typeid(D32))			Console::getInstance().printf("%f",any_cast<D32>(value));
		else if(value.type() == typeid(bool))			Console::getInstance().printf("%s",any_cast<bool>(value)? "true" : "false");
		else if(value.type() == typeid(int))			Console::getInstance().printf("%d",any_cast<I32>(value));
		else if(value.type() == typeid(U8))		     	Console::getInstance().printf("%d",any_cast<U8>(value));
		else if(value.type() == typeid(U16))			Console::getInstance().printf("%d",any_cast<U16>(value));
		else if(value.type() == typeid(U32))			Console::getInstance().printf("%d",any_cast<U32>(value));
		else if(value.type() == typeid(std::string)) 	Console::getInstance().printf("%s",any_cast<std::string>(value).c_str());
		else if(value.type() == typeid(const char*))	Console::getInstance().printf("%s",any_cast<const char*>(value));
		else Console::getInstance().printf("unconvertible %s",value.type().name());
		Console::getInstance().printf("\n");
	}
private:
	bool _logState;
	ParamMap _params;
	boost::mutex mutex_;
 
END_SINGLETON

#endif