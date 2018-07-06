#include "Headers/ParamHandler.h"	

template <class T>	
T ParamHandler::getParam(const std::string& name){
	boost::mutex::scoped_lock  lock(mutex_);
	params::iterator it = _params.find(name);
	if(it != _params.end())
		return it->second;
	else
		return NULL;
}

template<>
F32 ParamHandler::getParam<F32>(const std::string& name){
	boost::mutex::scoped_lock  lock(_mutex);
	ParamMap::iterator it = _params.find(name);
	if(it != _params.end()){
		try	{
			F32 temp = any_cast<F32>(it->second);
			return temp;
		}catch(const boost::bad_any_cast &){
			try{
				return any_cast<D32>(it->second); //Float and double are interchangable;
			}catch(const boost::bad_any_cast &){
				return 0;
			}
			
		}
	}else return 0;
}

template<>
U8 ParamHandler::getParam<U8>(const std::string& name){
	boost::mutex::scoped_lock  lock(_mutex);
	ParamMap::iterator it = _params.find(name);
	if(it != _params.end())	{
		try
		{
			return any_cast<U8>(it->second);
		}
		catch(const boost::bad_any_cast &)
		{
			PRINT_FN("ParamHandler: error casting [ %s ] to U8",name.c_str());
			return 0;
		}
	}else return 0;
}

template<>
U16 ParamHandler::getParam<U16>(const std::string& name){
	boost::mutex::scoped_lock  lock(_mutex);
	ParamMap::iterator it = _params.find(name);
	if(it != _params.end())	{
		try
		{
			return any_cast<U16>(it->second);
		}
		catch(const boost::bad_any_cast &)
		{
			PRINT_FN("ParamHandler: error casting [ %s ] to U16",name.c_str());
			return 0;
		}
	}else return 0;
}

template<>
U32 ParamHandler::getParam<U32>(const std::string& name){
	boost::mutex::scoped_lock  lock(_mutex);
	ParamMap::iterator it = _params.find(name);
	if(it != _params.end())	{
		try
		{
			return any_cast<U32>(it->second);
		}
		catch(const boost::bad_any_cast &)
		{
			PRINT_FN("ParamHandler: error casting [ %s ] to U32",name.c_str());
			return 0;
		}
	}else return 0;
}

	template<>
I8 ParamHandler::getParam<I8>(const std::string& name){
	boost::mutex::scoped_lock  lock(_mutex);
	ParamMap::iterator it = _params.find(name);
	if(it != _params.end())	{
		try
		{
			return any_cast<I8>(it->second);
		}
		catch(const boost::bad_any_cast &)
		{
			return 0;
		}
	}else return 0;
}

template<>
I16 ParamHandler::getParam<I16>(const std::string& name){
	boost::mutex::scoped_lock  lock(_mutex);
	ParamMap::iterator it = _params.find(name);
	if(it != _params.end())	{
		try
		{
			return any_cast<I16>(it->second);
		}
		catch(const boost::bad_any_cast &)
		{
			return 0;
		}
	}	else return 0;
}

template<>
I32 ParamHandler::getParam<I32>(const std::string& name){
	boost::mutex::scoped_lock  lock(_mutex);
	ParamMap::iterator it = _params.find(name);
	if(it != _params.end())	{
		try
		{
			return any_cast<I32>(it->second);
		}
		catch(const boost::bad_any_cast &)
		{
			return 0;
		}
	}else return 0;
}

template<>
bool ParamHandler::getParam<bool>(const std::string& name){
	boost::mutex::scoped_lock  lock(_mutex);
	ParamMap::iterator it = _params.find(name);
	if(it != _params.end())	{
		try
		{
			return any_cast<bool>(it->second);
		}
		catch(const boost::bad_any_cast &)
		{
			return false;
		}
	}else return false;
}

template<>
const char* ParamHandler::getParam<const char*>(const std::string& name){
	boost::mutex::scoped_lock  lock(_mutex);
	ParamMap::iterator it = _params.find(name);
	if(it != _params.end())	{
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
std::string ParamHandler::getParam<std::string>(const std::string& name){
	_mutex.lock();
	ParamMap::iterator it = _params.find(name);
	_mutex.unlock();
	if(it != _params.end())	{
		try
		{
			return any_cast<std::string>(it->second);
		}
		catch(const boost::bad_any_cast &)
		{
			return "ParamHandler: Error! not found";
		}
	}else return "ParamHandler: Error! not found";
}


void ParamHandler::printOutput(const std::string& name, const boost::any& value,bool inserted){
	std::string param = "ParamHandler: Updated param \""+name+"\" : ";
	if(inserted)  param = "ParamHandler: Saved param \""+name+"\" : ";
	PRINT_F("%s",param.c_str());
	if(value.type() == typeid(F32))				    PRINT_F("%f",any_cast<F32>(value))
	else if(value.type() == typeid(D32))			PRINT_F("%f",any_cast<D32>(value))
	else if(value.type() == typeid(bool))			PRINT_F("%s",any_cast<bool>(value)? "true" : "false")
	else if(value.type() == typeid(int))			PRINT_F("%d",any_cast<I32>(value))
	else if(value.type() == typeid(U8))		     	PRINT_F("%d",any_cast<U8>(value))
	else if(value.type() == typeid(U16))			PRINT_F("%d",any_cast<U16>(value))
	else if(value.type() == typeid(U32))			PRINT_F("%d",any_cast<U32>(value))
	else if(value.type() == typeid(std::string)) 	PRINT_F("%s",any_cast<std::string>(value).c_str())
	else if(value.type() == typeid(const char*))	PRINT_F("%s",any_cast<const char*>(value))
	else PRINT_F("unconvertible %s",value.type().name())
	PRINT_F("\n");
}