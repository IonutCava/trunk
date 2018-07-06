#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Managers/Headers/ShaderManager.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/Application.h"
#include <boost/algorithm/string.hpp>

ShaderProgram* ImplResourceLoader<ShaderProgram>::operator()(){
    ParamHandler& par = ParamHandler::getInstance();
    ShaderProgram* ptr = GFX_DEVICE.newShaderProgram();

    ptr->setState(RES_LOADING);
    ptr->enableThreadedLoading(_descriptor.getThreaded() && Application::getInstance().mainLoopActive());
    if(_descriptor.getResourceLocation().compare("default") == 0){
        ptr->setResourceLocation(par.getParam<std::string>("assetsLocation") + "/" +
                                 par.getParam<std::string>("shaderLocation") + "/" );
    }else{
        ptr->setResourceLocation(_descriptor.getResourceLocation());
    }

    //get all of the preprocessor defines
    if(!_descriptor.getPropertyListString().empty()){
        vectorImpl<std::string> defines;
        boost::split(defines, _descriptor.getPropertyListString(), boost::is_any_of(","), boost::token_compress_on);
        for(U8 i = 0; i < defines.size(); i++){
            if(!defines[i].empty())ptr->addShaderDefine(defines[i]);
        }
    }

    if(!load(ptr,_descriptor.getName())){
        SAFE_DELETE(ptr);
    }else{
        ShaderManager::getInstance().registerShaderProgram(ptr->getName(), ptr);
    }

    return ptr;
}

DEFAULT_HW_LOADER_IMPL(ShaderProgram)