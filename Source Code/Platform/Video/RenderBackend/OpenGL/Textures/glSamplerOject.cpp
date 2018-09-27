#include "stdafx.h"

#include "Headers/glSamplerObject.h"

#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/Textures/Headers/TextureDescriptor.h"

namespace Divide {

GLuint glSamplerObject::construct(const SamplerDescriptor& descriptor) {
    GLuint samplerID = 0;

    glCreateSamplers(1, &samplerID);
    glSamplerParameterf(samplerID, GL_TEXTURE_LOD_BIAS, descriptor._biasLOD);
    glSamplerParameterf(samplerID, GL_TEXTURE_MIN_LOD, descriptor._minLOD);
    glSamplerParameterf(samplerID, GL_TEXTURE_MAX_LOD, descriptor._maxLOD);
    glSamplerParameteri(samplerID, GL_TEXTURE_MIN_FILTER, to_U32(GLUtil::glTextureFilterTable[to_U32(descriptor._minFilter)]));
    glSamplerParameteri(samplerID, GL_TEXTURE_MAG_FILTER, to_U32(GLUtil::glTextureFilterTable[to_U32(descriptor._magFilter)]));
    glSamplerParameteri(samplerID, GL_TEXTURE_WRAP_S, to_U32(GLUtil::glWrapTable[to_U32(descriptor._wrapU)]));
    glSamplerParameteri(samplerID, GL_TEXTURE_WRAP_T, to_U32(GLUtil::glWrapTable[to_U32(descriptor._wrapV)]));
    glSamplerParameteri(samplerID, GL_TEXTURE_WRAP_R, to_U32(GLUtil::glWrapTable[to_U32(descriptor._wrapW)]));

    if (descriptor._wrapU == TextureWrap::CLAMP_TO_BORDER ||
        descriptor._wrapV == TextureWrap::CLAMP_TO_BORDER ||
        descriptor._wrapW == TextureWrap::CLAMP_TO_BORDER)
    {
        glSamplerParameterfv(samplerID, GL_TEXTURE_BORDER_COLOR, descriptor._borderColour._v);
    }

    if (descriptor._useRefCompare) {
        glSamplerParameteri(samplerID, GL_TEXTURE_COMPARE_FUNC, to_U32(GLUtil::glCompareFuncTable[to_U32(descriptor._cmpFunc)]));
        glSamplerParameteri(samplerID, GL_TEXTURE_COMPARE_MODE, to_base(GL_COMPARE_REF_TO_TEXTURE));
    } else {
        glSamplerParameteri(samplerID, GL_TEXTURE_COMPARE_MODE, to_base(GL_NONE));
    }

    if (descriptor._anisotropyLevel > 1 && descriptor.generateMipMaps()) {
        glSamplerParameterf(samplerID,
                            GL_API::s_opengl46Supported ? gl::GL_TEXTURE_MAX_ANISOTROPY
                                                        : gl::GL_TEXTURE_MAX_ANISOTROPY_EXT,
                            std::min<GLfloat>(to_F32(descriptor._anisotropyLevel),
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