#include "Headers/glSamplerObject.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/ParamHandler.h"
#include "Utility/Headers/Localization.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"
#include "Platform/Video/Textures/Headers/TextureDescriptor.h"

namespace Divide {

glSamplerObject::glSamplerObject(const SamplerDescriptor& descriptor) {
    DIVIDE_ASSERT((glewIsSupported("GL_ARB_sampler_objects") == GL_TRUE),
                  Locale::get("ERROR_NO_SAMPLER_SUPPORT"));

    glGenSamplers(1, &_samplerID);

    glSamplerParameterf(_samplerID, GL_TEXTURE_LOD_BIAS, descriptor.biasLOD());
    glSamplerParameterf(_samplerID, GL_TEXTURE_MIN_LOD, descriptor.minLOD());
    glSamplerParameterf(_samplerID, GL_TEXTURE_MAX_LOD, descriptor.maxLOD());
    glSamplerParameteri(
        _samplerID, GL_TEXTURE_MIN_FILTER,
        GLUtil::GL_ENUM_TABLE::glTextureFilterTable[descriptor.minFilter()]);
    glSamplerParameteri(
        _samplerID, GL_TEXTURE_MAG_FILTER,
        GLUtil::GL_ENUM_TABLE::glTextureFilterTable[descriptor.magFilter()]);
    glSamplerParameteri(_samplerID, GL_TEXTURE_WRAP_S,
                        GLUtil::GL_ENUM_TABLE::glWrapTable[descriptor.wrapU()]);
    glSamplerParameteri(_samplerID, GL_TEXTURE_WRAP_T,
                        GLUtil::GL_ENUM_TABLE::glWrapTable[descriptor.wrapV()]);
    glSamplerParameteri(_samplerID, GL_TEXTURE_WRAP_R,
                        GLUtil::GL_ENUM_TABLE::glWrapTable[descriptor.wrapW()]);
    if (descriptor.wrapU() == TEXTURE_CLAMP_TO_BORDER ||
        descriptor.wrapV() == TEXTURE_CLAMP_TO_BORDER ||
        descriptor.wrapW() == TEXTURE_CLAMP_TO_BORDER) {
        glSamplerParameterfv(_samplerID, GL_TEXTURE_BORDER_COLOR,
                             descriptor.borderColor());
    }

    if (descriptor._useRefCompare) {
        glSamplerParameteri(_samplerID, GL_TEXTURE_COMPARE_MODE,
                            GL_COMPARE_R_TO_TEXTURE);
        glSamplerParameteri(
            _samplerID, GL_TEXTURE_COMPARE_FUNC,
            GLUtil::GL_ENUM_TABLE::glCompareFuncTable[descriptor._cmpFunc]);
    }

    if (descriptor.anisotropyLevel() > 1 && descriptor.generateMipMaps()) {
        GLint anisoLevel =
            std::min<I32>((GLint)descriptor.anisotropyLevel(),
                          ParamHandler::getInstance().getParam<GLint>(
                              "rendering.anisotropicFilteringLevel"));
        glSamplerParameterf(_samplerID, GL_TEXTURE_MAX_ANISOTROPY_EXT,
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