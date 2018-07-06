#include "Headers/ParamHandler.h"	
using boost::any_cast;

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