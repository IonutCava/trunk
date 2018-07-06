//Creation and management of new OpenGL related entities: GL fonts, FBO's, VBO's etc
#include "Headers/GLWrapper.h"
#include "Headers/glRenderStateBlock.h"
#include "Core/Headers/ParamHandler.h"
#include "Headers/glImmediateModeEmulation.h"

#include "Hardware/Video/OpenGL/Buffers/VertexBufferObject/Headers/glVertexArrayObject.h"
#include "Hardware/Video/OpenGL/Buffers/VertexBufferObject/Headers/glGenericVertexData.h"
#include "Hardware/Video/OpenGL/Buffers/FrameBufferObject/Headers/glFrameBufferObject.h"
#include "Hardware/Video/OpenGL/Buffers/PixelBufferObject/Headers/glPixelBufferObject.h"

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
    assert(tempPriv != nullptr);
    tempPriv->_setupStates.clear();
    tempPriv->_resetStates.clear();
    tempPriv->_canZombify = allowPrimitiveRecycle;
    return tempPriv;
}

RenderStateBlock* GL_API::newRenderStateBlock(const RenderStateBlockDescriptor& descriptor){
    return New glRenderStateBlock(descriptor);
}

FrameBufferObject* GL_API::newFBO(const FBOType& type)  {
    return New glFrameBufferObject(type);
}

VertexBufferObject* GL_API::newVBO(const PrimitiveType& type) {
    return New glVertexArrayObject(type);
}

PixelBufferObject* GL_API::newPBO(const PBOType& type) {
    return New glPixelBufferObject(type);
}

GenericVertexData* GL_API::newGVD() {
    return New glGenericVertexData();
}