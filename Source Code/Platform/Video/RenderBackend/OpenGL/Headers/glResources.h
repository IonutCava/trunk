/*
   Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _GL_RESOURCES_H_
#define _GL_RESOURCES_H_

#include "Platform/Headers/PlatformDefines.h"
#ifndef GLBINDING_STATIC
#define GLBINDING_STATIC
#endif

#include "Platform/Video/Headers/RenderAPIWrapper.h"

#include <glbinding/gl46/gl.h>
using namespace gl46;

#include <glbinding/Binding.h>

struct SDL_Window;
typedef void *SDL_GLContext;

namespace NS_GLIM {
    enum class GLIM_ENUM : int;
}; //namespace NS_GLIM

namespace Divide {


struct ImageBindSettings {
    GLuint _texture = 0;
    GLint  _level = 0;
    GLboolean _layered = GL_FALSE;
    GLint _layer = 0;
    GLenum _access = GL_NONE;
    GLenum _format = GL_NONE;

    inline void reset() {
        _texture = 0;
         _level = 0;
        _layered = GL_FALSE;
        _layer = 0;
        _access = GL_NONE;
        _format = GL_NONE;
    }

    inline bool operator==(const ImageBindSettings& other) const {
        return _texture == other._texture &&
               _level == other._level &&
               _layered == other._layered &&
               _layer == other._layer &&
               _access == other._access &&
               _format == other._format;
    }

    inline bool operator!=(const ImageBindSettings& other) const {
        return !(*this == other);
    }
};

class VAOBindings {
public:
    using BufferBindingParams = std::tuple<GLuint, GLintptr, GLsizei>;

    VAOBindings() noexcept;
    ~VAOBindings();
    void init(U32 maxBindings);

    const BufferBindingParams& bindingParams(GLuint vao, GLuint index);
    void bindingParams(GLuint vao, GLuint index, const BufferBindingParams& newParams);

private:
    using VAOBufferData = vector<BufferBindingParams>;

    mutable VAOBufferData* _cachedData = nullptr;
    mutable GLuint _cachedVao = 0;

    hashMap<GLuint /*vao ID*/, VAOBufferData> _bindings;
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

// Not thread-safe!
class glTexturePool {
private:
    enum class State : U8 {
        USED = 0,
        FREE,
        CLEAN
    };

    struct poolImpl {
        vectorEASTL<AtomicWrapper<State>>  _usageMap;

        vectorEASTL<U32>    _lifeLeft;
        vectorEASTL<GLuint> _handles;
        vectorEASTL<GLuint> _tempBuffer;
        GLenum _type = GL_NONE;
    };
public:
    void onFrameEnd();
    void init(const vectorEASTL<std::pair<GLenum, U32>>& poolSizes);
    void destroy();
    inline bool typeSupported(GLenum type) const;
    //Use GL_NONE for textures created with glGen instead of glCreate (e.g. for texture views)
    GLuint allocate(GLenum type = GL_NONE, bool retry = false);
    
    void deallocate(GLuint& handle, GLenum type = GL_NONE, U32 frameDelay = 1);

protected:
    void onFrameEndInternal(poolImpl& impl);

private:
    vectorEASTL<GLenum> _types;
    hashMap<GLenum, poolImpl> _pools;
};
/// Wrapper for glGetIntegerv
template<typename T = GLint>
void getGLValue(GLenum param, T& value, GLint index = -1);

template<typename T = GLint>
T getGLValue(GLenum param);

template<typename T = GLint>
T getGLValueIndexed(GLenum param, GLint index = -1);

/// Check the current operation for errors
void DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                   GLsizei length, const GLchar* message, const void* userParam);

/// Invalid object value. Used to compare handles and determine if they were
/// properly created
extern GLuint k_invalidObjectID;
extern GLuint s_lastQueryResult;
extern const DisplayWindow* s_glMainRenderWindow;
extern thread_local SDL_GLContext s_glSecondaryContext;

extern std::mutex s_driverLock;
extern std::mutex s_glSecondaryContextMutex;

void submitRenderCommand(const GenericDrawCommand& drawCommand,
                         bool drawIndexed,
                         bool useIndirectBuffer,
                         GLenum internalFormat,
                         GLsizei* countData = nullptr,
                         bufferPtr indexData = nullptr);

/// Populate enumeration tables with appropriate API values
void fillEnumTables();

GLenum internalFormat(GFXImageFormat baseFormat, GFXDataFormat dataType, bool srgb);

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
extern std::array<UseProgramStageMask, to_base(ShaderType::COUNT) + 1> glProgramStageMask;
extern std::array<stringImpl, to_base(ShaderType::COUNT)> glShaderStageNameTable;
extern std::array<GLenum, to_base(QueryType::COUNT)> glQueryTypeTable;

};  // namespace GLUtil
};  // namespace Divide

#endif

#include "glResources.inl"
