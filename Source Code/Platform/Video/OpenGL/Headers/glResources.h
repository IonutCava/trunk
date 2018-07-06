/*
   Copyright (c) 2017 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _GL_RESOURCES_H_
#define _GL_RESOURCES_H_

#include "Platform/Headers/PlatformDefines.h"
#ifndef GLBINDING_STATIC
#define GLBINDING_STATIC
#endif

#include <glbinding/gl45/gl.h>
using namespace gl45;

#include <glbinding/Binding.h>

#include "Platform/Video/Headers/RenderAPIWrapper.h"

struct SDL_Window;
typedef void *SDL_GLContext;

namespace NS_GLIM {
    enum class GLIM_ENUM : int;
}; //namespace NS_GLIM

namespace Divide {


struct ImageBindSettings {
    GLuint _texture;
    GLint  _level;
    GLboolean _layered;
    GLint _layer;
    GLenum _access;
    GLenum _format;

    bool operator==(const ImageBindSettings& other) const {
        return _texture == other._texture &&
            _level == other._level &&
            _layered == other._layered &&
            _layer == other._layer &&
            _access == other._access &&
            _format == other._format;
    }

    bool operator!=(const ImageBindSettings& other) const {
        return !(*this == other);
    }
};

class VAOBindings {
public:
    typedef std::tuple<GLuint, GLintptr, GLsizei> BufferBindingParams;

    VAOBindings();
    ~VAOBindings();
    void init(U32 maxBindings);

    const BufferBindingParams& bindingParams(GLuint vao, GLuint index);
    void bindingParams(GLuint vao, GLuint index, const BufferBindingParams& newParams);

private:
    typedef vectorImpl<BufferBindingParams> VAOBufferData;
    hashMapImpl<GLuint /*vao ID*/, VAOBufferData> _bindings;
    U32 _maxBindings;
};

enum class glObjectType : U8 {
    TYPE_BUFFER = 0,
    TYPE_TEXTURE,
    TYPE_SHADER,
    TYPE_SHADER_PROGRAM,
    TYPE_FRAMEBUFFER,
    TYPE_QUERY,
    COUNT
};

class glObject {
public:
    explicit glObject(glObjectType type, GFXDevice& context);

    inline glObjectType type() const { return _type;  }

private:
   const glObjectType _type;
};

namespace GLUtil {

/// Wrapper for glGetIntegerv
GLint getIntegerv(GLenum param);

/// Check the current operation for errors
void DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                   GLsizei length, const GLchar* message, const void* userParam);

/// Invalid object value. Used to compare handles and determine if they were
/// properly created
extern GLuint _invalidObjectID;
extern GLuint _lastQueryResult;
extern SDL_GLContext _glRenderContext;
extern thread_local SDL_GLContext _glSecondaryContext;
extern SharedLock _glSecondaryContextMutex;

void submitRenderCommand(const GenericDrawCommand& drawCommand,
                         bool useIndirectBuffer,
                         GLenum internalFormat,
                         GLuint indexBuffer = 0);

/// Populate enumeration tables with appropriate API values
void fillEnumTables();

extern std::array<GLenum, to_base(BlendProperty::COUNT)> glBlendTable;
extern std::array<GLenum, to_base(BlendOperation::COUNT)> glBlendOpTable;
extern std::array<GLenum, to_base(ComparisonFunction::COUNT)> glCompareFuncTable;
extern std::array<GLenum, to_base(StencilOperation::COUNT)> glStencilOpTable;
extern std::array<GLenum, to_base(CullMode::COUNT)> glCullModeTable;
extern std::array<GLenum, to_base(FillMode::COUNT)> glFillModeTable;
extern std::array<GLenum, to_base(TextureType::COUNT)> glTextureTypeTable;
extern std::array<GLenum, to_base(GFXImageFormat::COUNT)> glImageFormatTable;
extern std::array<GLenum, to_base(PrimitiveType::COUNT)> glPrimitiveTypeTable;
extern std::array<GLenum, to_base(GFXDataFormat::COUNT)> glDataFormat;
extern std::array<GLenum, to_base(TextureWrap::COUNT)> glWrapTable;
extern std::array<GLenum, to_base(TextureFilter::COUNT)> glTextureFilterTable;
extern std::array<NS_GLIM::GLIM_ENUM, to_base(PrimitiveType::COUNT)> glimPrimitiveType;
extern std::array<GLenum, to_base(ShaderType::COUNT)> glShaderStageTable;
extern std::array<stringImpl, to_base(ShaderType::COUNT)> glShaderStageNameTable;
extern std::array<GLenum, to_base(QueryType::COUNT)> glQueryTypeTable;

};  // namespace GLUtil
};  // namespace Divide

#endif

#include "glResources.inl"
