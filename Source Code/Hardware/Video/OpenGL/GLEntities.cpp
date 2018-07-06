//Creation and management of new OpenGL related entities: GL fonts, FB's, VB's etc
#include "Headers/GLWrapper.h"
#include "Core/Headers/ParamHandler.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Headers/glImmediateModeEmulation.h"

#include "Hardware/Video/OpenGL/Buffers/VertexBuffer/Headers/glVertexArray.h"
#include "Hardware/Video/OpenGL/Buffers/VertexBuffer/Headers/glGenericVertexData.h"
#include "Hardware/Video/OpenGL/Buffers/ShaderBuffer/Headers/glUniformBuffer.h"
#include "Hardware/Video/OpenGL/Buffers/FrameBuffer/Headers/glFrameBuffer.h"
#include "Hardware/Video/OpenGL/Buffers/PixelBuffer/Headers/glPixelBuffer.h"

namespace IMPrimitiveValidation{
    inline bool isValid(glIMPrimitive* const priv){ return (priv && !priv->inUse()); }
}

IMPrimitive* GL_API::createPrimitive(bool allowPrimitiveRecycle) {
    return dynamic_cast<IMPrimitive*>(getOrCreateIMPrimitive(allowPrimitiveRecycle));
}

glIMPrimitive* GL_API::getOrCreateIMPrimitive(bool allowPrimitiveRecycle){
    glIMPrimitive* tempPriv = nullptr;
    //Find a zombified primitive
    vectorImpl<glIMPrimitive* >::iterator it = std::find_if(_glimInterfaces.begin(),_glimInterfaces.end(),IMPrimitiveValidation::isValid);
    if(allowPrimitiveRecycle && it != _glimInterfaces.end()){//If we have one ...
        tempPriv = *it;
        //... resurrect it
        tempPriv->_zombieCounter = 0;
        tempPriv->clear();
    }else{//If we do not have a valid zombie, add a new element
        tempPriv = New glIMPrimitive();
        _glimInterfaces.push_back(tempPriv);
    }
    tempPriv->_setupStates.clear();
    tempPriv->_resetStates.clear();
    tempPriv->_canZombify = allowPrimitiveRecycle;
    return tempPriv;
}

/// Creates a new frame buffer
FrameBuffer* GL_API::newFB(bool multisampled) const {
    // if MSAA is disabled, this will be a simple color / depth buffer
    if (!GFX_DEVICE.MSAAEnabled()){
        multisampled = false;
    }
    // if we requested a MSFB and msaa is enabled, create a resolve buffer
    // and create our FB adding the resolve buffer as it's child
    // else, return a regular frame buffer
     return New glFrameBuffer(multisampled ? New glFrameBuffer() : nullptr);
}

VertexBuffer* GL_API::newVB() const {
    return New glVertexArray();
}

PixelBuffer* GL_API::newPB(const PBType& type) const {
    return New glPixelBuffer(type);
}

GenericVertexData* GL_API::newGVD(const bool persistentMapped) const {
    return New glGenericVertexData(persistentMapped);
}

ShaderBuffer* GL_API::newSB(const bool unbound) const {
    return New glUniformBuffer(unbound);
}