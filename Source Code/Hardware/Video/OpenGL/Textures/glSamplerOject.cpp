#include "Headers/glSamplerObject.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

glSamplerObject::glSamplerObject() : _samplerID(0)
{
}

glSamplerObject::~glSamplerObject()
{
    if(_samplerID > 0){
        glDeleteSamplers(1,&_samplerID);
    }
}

bool glSamplerObject::Create(const SamplerDescriptor& descriptor) {
    if(!glewIsSupported("ARB_sampler_objects")){
        ERROR_FN(Locale::get("ERROR_NO_SAMPLER_SUPPORT"));
        return false;
    }

    glGenSamplers(1, &_samplerID);

    glSamplerParameteri(_samplerID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(_samplerID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(_samplerID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(_samplerID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(_samplerID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    //glSamplerParameterfv(_samplerID, GL_TEXTURE_BORDER_COLOR, &glm::vec4(0.0f)[0]);
    glSamplerParameterf(_samplerID, GL_TEXTURE_MIN_LOD, -1000.f);
    glSamplerParameterf(_samplerID, GL_TEXTURE_MAX_LOD, 1000.f);
    glSamplerParameterf(_samplerID, GL_TEXTURE_LOD_BIAS, 0.0f);
    glSamplerParameteri(_samplerID, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    glSamplerParameteri(_samplerID, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glSamplerParameterf(_samplerID, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glSamplerParameterf(_samplerID, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.f);
	return true;
}

void glSamplerObject::Bind(GLuint textureUnit){
    glBindSampler(textureUnit, _samplerID);
}