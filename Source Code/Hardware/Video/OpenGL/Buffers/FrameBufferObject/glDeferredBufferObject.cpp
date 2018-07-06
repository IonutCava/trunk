#include "Headers/glFrameBufferObject.h"

#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "core.h"
#include "Headers/glFrameBufferObject.h"
#include "Hardware/Video/Headers/GFXDevice.h"

bool glFrameBufferObject::CreateDeferred(){
    D_PRINT_FN(Locale::get("GL_FBO_GEN_DEFERRED"),_width,_height);
    Destroy();
    _useDepthBuffer = true;

    GLCheck(glGenFramebuffers(1, &_frameBufferHandle));

    GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferHandle));

    TextureDescriptor albedo   = _attachement[TextureDescriptor::Color0];
    TextureDescriptor position = _attachement[TextureDescriptor::Color1];
    TextureDescriptor normals  = _attachement[TextureDescriptor::Color2];
    TextureDescriptor blend    = _attachement[TextureDescriptor::Color3];

    GLCheck(glGenTextures(5, _textureId));

    //Depth Buffer
    GLCheck(glBindTexture(_textureType, _textureId[TextureDescriptor::Depth]));
    GLCheck(glTexParameteri(_textureType, GL_TEXTURE_COMPARE_FUNC, GL_NONE)); //should change if needed
    GLCheck(glTexParameteri (_textureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GLCheck(glTexParameteri (_textureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GLCheck(glTexParameteri (_textureType, GL_TEXTURE_WRAP_S, GL_CLAMP));
    GLCheck(glTexParameteri (_textureType, GL_TEXTURE_WRAP_T, GL_CLAMP));
    GLCheck(glTexImage2D(_textureType, 0, GL_DEPTH_COMPONENT24, _width, _height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0));

    //Albedo
    GLCheck(glBindTexture(_textureType, _textureId[TextureDescriptor::Color0]));
    GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, glTextureFilterTable[albedo.getSampler().magFilter()]));
    GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, glTextureFilterTable[albedo.getSampler().minFilter()]));
    GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_S, glWrapTable[albedo.getSampler().wrapU()]));
    GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, glWrapTable[albedo.getSampler().wrapV()]));

    GLCheck(glTexImage2D(_textureType,  0, glImageFormatTable[albedo._internalFormat],
                         _width, _height, 0, glImageFormatTable[albedo._format],
                         glDataFormat[albedo._dataType], NULL));

    //Positions
    GLCheck(glBindTexture(_textureType, _textureId[TextureDescriptor::Color1]));

    GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, glTextureFilterTable[position.getSampler().magFilter()]));
    GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, glTextureFilterTable[position.getSampler().minFilter()]));
    GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_S, glWrapTable[position.getSampler().wrapU()]));
    GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, glWrapTable[position.getSampler().wrapV()]));

    GLCheck(glTexImage2D(_textureType,  0, glImageFormatTable[position._internalFormat],
                         _width, _height, 0, glImageFormatTable[position._format],
                         glDataFormat[position._dataType], NULL));

    //Normals
    GLCheck(glBindTexture(_textureType, _textureId[TextureDescriptor::Color2]));
    GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, glTextureFilterTable[normals.getSampler().magFilter()]));
    GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, glTextureFilterTable[normals.getSampler().minFilter()]));
    GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_S, glWrapTable[normals.getSampler().wrapU()]));
    GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, glWrapTable[normals.getSampler().wrapV()]));

    GLCheck(glTexImage2D(_textureType,  0, glImageFormatTable[normals._internalFormat],
                         _width, _height, 0, glImageFormatTable[normals._format],
                         glDataFormat[normals._dataType], NULL));

    //Blend
    GLCheck(glBindTexture(_textureType, _textureId[TextureDescriptor::Color3]));
    GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MAG_FILTER, glTextureFilterTable[blend.getSampler().magFilter()]));
    GLCheck(glTexParameterf(_textureType, GL_TEXTURE_MIN_FILTER, glTextureFilterTable[blend.getSampler().minFilter()]));
    GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_S, glWrapTable[blend.getSampler().wrapU()]));
    GLCheck(glTexParameterf(_textureType, GL_TEXTURE_WRAP_T, glWrapTable[blend.getSampler().wrapV()]));

    GLCheck(glTexImage2D(_textureType,  0, glImageFormatTable[blend._internalFormat],
                         _width, _height, 0, glImageFormatTable[blend._format],
                         glDataFormat[blend._dataType], NULL));

    GLCheck(glBindTexture(_textureType,0));

    for(U8 i = 0; i < 4; i++){
        GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, _textureType, _textureId[i], 0));
    }

    GLCheck(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _textureType, _textureId[TextureDescriptor::Depth], 0)); 

    checkStatus();
    
    if(_clearColorState)
        GL_API::clearColor( _clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a );

    GLCheck(glClear(_clearBufferMask));
    
    GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    GLCheck(glDrawBuffers(4, buffers));

    GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    return true;
}