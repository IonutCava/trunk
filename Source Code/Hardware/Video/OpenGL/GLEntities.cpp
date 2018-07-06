//Creation and management of new OpenGL related entities: GL fonts, FB's, VB's etc
#include "Headers/GLWrapper.h"
#include "Headers/glRenderStateBlock.h"
#include "Core/Headers/ParamHandler.h"
#include "Headers/glImmediateModeEmulation.h"

#include "Hardware/Video/OpenGL/Buffers/VertexBuffer/Headers/glVertexArray.h"
#include "Hardware/Video/OpenGL/Buffers/VertexBuffer/Headers/glGenericVertexData.h"
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
    assert(tempPriv != nullptr);
    tempPriv->_setupStates.clear();
    tempPriv->_resetStates.clear();
    tempPriv->_canZombify = allowPrimitiveRecycle;
    return tempPriv;
}

RenderStateBlock* GL_API::newRenderStateBlock(const RenderStateBlockDescriptor& descriptor){
    return New glRenderStateBlock(descriptor);
}

FrameBuffer* GL_API::newFB(const FBType& type)  {
    return New glFrameBuffer(type);
}

VertexBuffer* GL_API::newVB(const PrimitiveType& type) {
    return New glVertexArray(type);
}

PixelBuffer* GL_API::newPB(const PBType& type) {
    return New glPixelBuffer(type);
}

GenericVertexData* GL_API::newGVD() {
    return New glGenericVertexData();
}