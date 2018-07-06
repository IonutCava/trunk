#include "stdafx.h"

#include "Headers/glSamplerObject.h"

#include "Platform/Video/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/Textures/Headers/TextureDescriptor.h"

namespace Divide {

GLuint glSamplerObject::construct(const SamplerDescriptor& descriptor) {
    GLuint samplerID = 0;

    glCreateSamplers(1, &samplerID);
    glSamplerParameterf(samplerID, GL_TEXTURE_LOD_BIAS, descriptor.biasLOD());
    glSamplerParameterf(samplerID, GL_TEXTURE_MIN_LOD, descriptor.minLOD());
    glSamplerParameterf(samplerID, GL_TEXTURE_MAX_LOD, descriptor.maxLOD());
    glSamplerParameteri(samplerID, GL_TEXTURE_MIN_FILTER, to_U32(GLUtil::glTextureFilterTable[to_U32(descriptor.minFilter())]));
    glSamplerParameteri(samplerID, GL_TEXTURE_MAG_FILTER, to_U32(GLUtil::glTextureFilterTable[to_U32(descriptor.magFilter())]));
    glSamplerParameteri(samplerID, GL_TEXTURE_WRAP_S, to_U32(GLUtil::glWrapTable[to_U32(descriptor.wrapU())]));
    glSamplerParameteri(samplerID, GL_TEXTURE_WRAP_T, to_U32(GLUtil::glWrapTable[to_U32(descriptor.wrapV())]));
    glSamplerParameteri(samplerID, GL_TEXTURE_WRAP_R, to_U32(GLUtil::glWrapTable[to_U32(descriptor.wrapW())]));

    if (descriptor.wrapU() == TextureWrap::CLAMP_TO_BORDER ||
        descriptor.wrapV() == TextureWrap::CLAMP_TO_BORDER ||
        descriptor.wrapW() == TextureWrap::CLAMP_TO_BORDER)
    {
        glSamplerParameterfv(samplerID, GL_TEXTURE_BORDER_COLOR, descriptor.borderColour());
    }

    if (descriptor._useRefCompare) {
        glSamplerParameteri(samplerID, GL_TEXTURE_COMPARE_FUNC, to_U32(GLUtil::glCompareFuncTable[to_U32(descriptor._cmpFunc)]));
        glSamplerParameteri(samplerID, GL_TEXTURE_COMPARE_MODE, to_base(GL_COMPARE_REF_TO_TEXTURE));
    } else {
        glSamplerParameteri(samplerID, GL_TEXTURE_COMPARE_MODE, to_base(GL_NONE));
    }

    if (descriptor.anisotropyLevel() > 1 && descriptor.generateMipMaps()) {
        glSamplerParameterf(samplerID,
                            GL_API::s_opengl46Supported ? gl::GL_TEXTURE_MAX_ANISOTROPY
                                                        : gl::GL_TEXTURE_MAX_ANISOTROPY_EXT,
                            std::min<GLfloat>(to_F32(descriptor.anisotropyLevel()),
                                              to_F32(GL_API::s_anisoLevel)));
    }

    return samplerID;
}

void glSamplerObject::destruct(GLuint& handle) {
    if (handle > 0) {
        glDeleteSamplers(1, &handle);
        handle = 0;
    }
}

};