#include "Headers/glSamplerObject.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/ParamHandler.h"
#include "Utility/Headers/Localization.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/Textures/Headers/TextureDescriptor.h"

namespace Divide {
    
glSamplerObject::glSamplerObject(const SamplerDescriptor& descriptor) {
    DIVIDE_ASSERT(glfwExtensionSupported("GL_ARB_sampler_objects") == 1,
                  Locale::get("ERROR_NO_SAMPLER_SUPPORT"));

    glGenSamplers(1, &_samplerID);

    glSamplerParameterf(_samplerID, GL_TEXTURE_LOD_BIAS, descriptor.biasLOD());
    glSamplerParameterf(_samplerID, GL_TEXTURE_MIN_LOD, descriptor.minLOD());
    glSamplerParameterf(_samplerID, GL_TEXTURE_MAX_LOD, descriptor.maxLOD());
    glSamplerParameteri(_samplerID, GL_TEXTURE_MIN_FILTER,
                        to_uint(GLUtil::GL_ENUM_TABLE::glTextureFilterTable[to_uint(
                            descriptor.minFilter())]));
    glSamplerParameteri(_samplerID, GL_TEXTURE_MAG_FILTER,
                        to_uint(GLUtil::GL_ENUM_TABLE::glTextureFilterTable[to_uint(
                            descriptor.magFilter())]));
    glSamplerParameteri(
        _samplerID, GL_TEXTURE_WRAP_S, to_uint(GLUtil::GL_ENUM_TABLE::glWrapTable[to_uint(descriptor.wrapU())]));
    glSamplerParameteri(
        _samplerID, GL_TEXTURE_WRAP_T, to_uint(GLUtil::GL_ENUM_TABLE::glWrapTable[to_uint(descriptor.wrapV())]));
    glSamplerParameteri(
        _samplerID, GL_TEXTURE_WRAP_R, to_uint(GLUtil::GL_ENUM_TABLE::glWrapTable[to_uint(descriptor.wrapW())]));
    if (descriptor.wrapU() == TextureWrap::TEXTURE_CLAMP_TO_BORDER ||
        descriptor.wrapV() == TextureWrap::TEXTURE_CLAMP_TO_BORDER ||
        descriptor.wrapW() == TextureWrap::TEXTURE_CLAMP_TO_BORDER) {
        glSamplerParameterfv(_samplerID, GL_TEXTURE_BORDER_COLOR,
                             descriptor.borderColor());
    }

    if (descriptor._useRefCompare) {
        glSamplerParameteri(_samplerID, GL_TEXTURE_COMPARE_FUNC,
                            to_uint(GLUtil::GL_ENUM_TABLE::glCompareFuncTable[to_uint(
                                descriptor._cmpFunc)]));

        glSamplerParameteri(_samplerID, GL_TEXTURE_COMPARE_MODE, to_uint(GL_COMPARE_REF_TO_TEXTURE));
    }

    if (descriptor.anisotropyLevel() > 1 && descriptor.generateMipMaps()) {
        GLint anisoLevel =
            std::min<I32>((GLint)descriptor.anisotropyLevel(),
                          ParamHandler::getInstance().getParam<GLint>(
                              "rendering.anisotropicFilteringLevel"));
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