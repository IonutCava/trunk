#include "Headers/glSamplerObject.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/ParamHandler.h"
#include "Utility/Headers/Localization.h"
#include "Hardware/Video/OpenGL/Headers/GLWrapper.h"
#include "Hardware/Video/Textures/Headers/TextureDescriptor.h"

glSamplerObject::glSamplerObject() : _samplerID(Divide::GL::_invalidObjectID)
{
    bool availableSamplerObj = (glewIsSupported("GL_ARB_sampler_objects") == GL_TRUE);
    if(!availableSamplerObj){
        ERROR_FN(Locale::get("ERROR_NO_SAMPLER_SUPPORT"));
        assert(availableSamplerObj);
    }
}

glSamplerObject::~glSamplerObject()
{
    Destroy();
}

bool glSamplerObject::Destroy() {
    if(_samplerID > 0 && _samplerID != Divide::GL::_invalidObjectID){
        GLCheck(glDeleteSamplers(1, &_samplerID));
        _samplerID = 0;
    }
    return (_samplerID == 0);
}

bool glSamplerObject::Create(const SamplerDescriptor& descriptor) {
    if(_samplerID == Divide::GL::_invalidObjectID) GLCheck(glGenSamplers(1, &_samplerID));

    GLCheck(glSamplerParameterf(_samplerID, GL_TEXTURE_LOD_BIAS, descriptor.biasLOD()));
    GLCheck(glSamplerParameterf(_samplerID, GL_TEXTURE_MIN_LOD, descriptor.minLOD()));
    GLCheck(glSamplerParameterf(_samplerID, GL_TEXTURE_MAX_LOD, descriptor.maxLOD()));
    GLCheck(glSamplerParameteri(_samplerID, GL_TEXTURE_MIN_FILTER, glTextureFilterTable[descriptor.minFilter()]));
    GLCheck(glSamplerParameteri(_samplerID, GL_TEXTURE_MAG_FILTER, glTextureFilterTable[descriptor.magFilter()]));
    GLCheck(glSamplerParameteri(_samplerID, GL_TEXTURE_WRAP_S, glWrapTable[descriptor.wrapU()]));
    GLCheck(glSamplerParameteri(_samplerID, GL_TEXTURE_WRAP_T, glWrapTable[descriptor.wrapV()]));
    GLCheck(glSamplerParameteri(_samplerID, GL_TEXTURE_WRAP_R, glWrapTable[descriptor.wrapW()]));

    if(descriptor._useRefCompare){
        GLCheck(glSamplerParameteri(_samplerID, GL_TEXTURE_COMPARE_MODE,  GL_COMPARE_R_TO_TEXTURE));
        GLCheck(glSamplerParameteri(_samplerID, GL_TEXTURE_COMPARE_FUNC,  glCompareFuncTable[descriptor._cmpFunc]));
        GLCheck(glSamplerParameteri(_samplerID, GL_DEPTH_TEXTURE_MODE,    glImageFormatTable[descriptor._depthCompareMode]));
    }

    if (descriptor.anisotrophyLevel() > 1 && descriptor.generateMipMaps()) {
        if(!glewIsSupported("GL_EXT_texture_filter_anisotropic")){
            ERROR_FN(Locale::get("ERROR_NO_ANISO_SUPPORT"));
        }else{
            U8 anisoLevel = std::min<I32>((I32)descriptor.anisotrophyLevel(), ParamHandler::getInstance().getParam<U8>("rendering.anisotropicFilteringLevel"));
            GLCheck(glSamplerParameterf(_samplerID, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisoLevel));
        }
    }
    //GLCheck(glSamplerParameterfv(_samplerID, GL_TEXTURE_BORDER_COLOR, &glm::vec4(0.0f)[0]));
    return true;
}

void glSamplerObject::Bind(GLuint textureUnit) const{
    GLCheck(glBindSampler(textureUnit, _samplerID));
}

void glSamplerObject::Unbind(GLuint textureUnit) const{
    GLCheck(glBindSampler(textureUnit, 0));
}

void glSamplerObject::Bind() const{
    GLCheck(glBindSampler(GL_API::getActiveTextureUnit(), _samplerID));
}

void glSamplerObject::Unbind() const{
    GLCheck(glBindSampler(GL_API::getActiveTextureUnit(), 0));
}