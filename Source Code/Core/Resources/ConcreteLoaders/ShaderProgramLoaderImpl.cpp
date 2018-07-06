#include "Core/Resources/Headers/ResourceLoader.h"
#include "Hardware/Video/GFXDevice.h"
#include "Core/Headers/ParamHandler.h"

ShaderProgram* ImplResourceLoader<ShaderProgram>::operator()(){
	
	ParamHandler& par = ParamHandler::getInstance();
	ShaderProgram* ptr = GFX_DEVICE.newShaderProgram();

	if(_descriptor.getResourceLocation().compare("default") == 0)
		ptr->setResourceLocation(par.getParam<std::string>("assetsLocation") + "/" + 
							     par.getParam<std::string>("shaderLocation") + "/" );
	else
		ptr->setResourceLocation(_descriptor.getResourceLocation());

	if(!ptr) return NULL;
	if(!ptr->load(_descriptor.getName())) return NULL;

	return ptr;
}