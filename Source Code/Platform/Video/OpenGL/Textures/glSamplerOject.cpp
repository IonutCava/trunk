#include "Headers/glSamplerObject.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/ParamHandler.h"
#include "Utility/Headers/Localization.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/Textures/Headers/TextureDescriptor.h"

namespace Divide {

glSamplerObject::glSamplerObject(const SamplerDescriptor& descriptor) {
    glCreateSamplers(1, &_samplerID);
    glSamplerParameterf(_samplerID, GL_TEXTURE_LOD_BIAS, descriptor.biasLOD());
    glSamplerParameterf(_samplerID, GL_TEXTURE_MIN_LOD, descriptor.minLOD());
    glSamplerParameterf(_samplerID, GL_TEXTURE_MAX_LOD, descriptor.maxLOD());
    glSamplerParameteri(
        _samplerID, GL_TEXTURE_MIN_FILTER,
        to_uint(GLUtil::glTextureFilterTable[to_uint(descriptor.minFilter())]));
    glSamplerParameteri(
        _samplerID, GL_TEXTURE_MAG_FILTER,
        to_uint(GLUtil::glTextureFilterTable[to_uint(descriptor.magFilter())]));
    glSamplerParameteri(
        _samplerID, GL_TEXTURE_WRAP_S,
        to_uint(GLUtil::glWrapTable[to_uint(descriptor.wrapU())]));
    glSamplerParameteri(
        _samplerID, GL_TEXTURE_WRAP_T,
        to_uint(GLUtil::glWrapTable[to_uint(descriptor.wrapV())]));
    glSamplerParameteri(
        _samplerID, GL_TEXTURE_WRAP_R,
        to_uint(GLUtil::glWrapTable[to_uint(descriptor.wrapW())]));
    if (descriptor.wrapU() == TextureWrap::CLAMP_TO_BORDER ||
        descriptor.wrapV() == TextureWrap::CLAMP_TO_BORDER ||
        descriptor.wrapW() == TextureWrap::CLAMP_TO_BORDER) {
        glSamplerParameterfv(_samplerID, GL_TEXTURE_BORDER_COLOR,
                             descriptor.borderColor());
    }

    if (descriptor._useRefCompare) {
        glSamplerParameteri(
            _samplerID, GL_TEXTURE_COMPARE_FUNC,
            to_uint(GLUtil::glCompareFuncTable[to_uint(descriptor._cmpFunc)]));

        glSamplerParameteri(_samplerID, GL_TEXTURE_COMPARE_MODE,
                            to_uint(GL_COMPARE_REF_TO_TEXTURE));
    }

    if (descriptor.anisotropyLevel() > 1 && descriptor.generateMipMaps()) {
        GLfloat anisoLevel =
            std::min<GLfloat>(to_float(descriptor.anisotropyLevel()),
                              to_float(ParamHandler::getInstance().getParam<GLint>(
                                  _ID("rendering.anisotropicFilteringLevel"))));

        glSamplerParameterf(_samplerID, gl::GL_TEXTURE_MAX_ANISOTROPY_EXT,
                            anisoLevel);
    }
    // glSamplerParameterfv(_samplerID, GL_TEXTURE_BORDER_COLOR,
    // &vec4<F32>(0.0f).r));
}

glSamplerObject::~glSamplerObject() {
    if (_samplerID > 0 && _samplerID != GLUtil::_invalidObjectID) {
        glDeleteSamplers(1, &_samplerID);
        _samplerID = 0;
    }
}
};