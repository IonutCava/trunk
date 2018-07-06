#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/Application.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Shaders/Headers/ShaderManager.h"

#include <EASTL/string.h>

namespace Divide {


ShaderProgram* ImplResourceLoader<ShaderProgram>::operator()(){
    ParamHandler& par = ParamHandler::getInstance();
    ShaderProgram* ptr = GFX_DEVICE.newShaderProgram();

    ptr->setState(RES_LOADING);
    ptr->enableThreadedLoading(_descriptor.getThreaded() && Application::getInstance().mainLoopActive());
    if ( _descriptor.getResourceLocation().compare( "default" ) == 0 ) {
        ptr->setResourceLocation( par.getParam<stringImpl>( "assetsLocation" ) + "/" +
                                  par.getParam<stringImpl>( "shaderLocation" ) + "/" );
    } else {
        ptr->setResourceLocation( _descriptor.getResourceLocation() );
    }

    if ( _descriptor.getId() > 0 ) {
        ptr->SetOutputCount( (U8)_descriptor.getId() );
    }
    //get all of the preprocessor defines
    if ( !_descriptor.getPropertyListString().empty() ) {
        vectorImpl<stringImpl> defines;
        Util::split( _descriptor.getPropertyListString(), ",", defines );
        for ( U8 i = 0; i < defines.size(); i++ ) {
            if ( !defines[i].empty() )ptr->addShaderDefine( defines[i] );
        }
    }

    if ( !load( ptr, _descriptor.getName() ) ) {
        MemoryManager::SAFE_DELETE( ptr );
    } else {
        ShaderManager::getInstance().registerShaderProgram( ptr->getName(), ptr );
    }

    return ptr;
}

DEFAULT_HW_LOADER_IMPL(ShaderProgram)

};