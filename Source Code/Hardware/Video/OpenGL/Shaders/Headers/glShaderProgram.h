/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef GLSL_H_
#define GLSL_H_

#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "Hardware/Video/Shaders/Headers/ShaderProgram.h"

/// OpenGL implementation of the Shader entity
class glShaderProgram : public ShaderProgram {
public:
    glShaderProgram(const bool optimise = false);
    ~glShaderProgram();

    /// Make sure this program is ready for deletion
    inline bool unload() {
        unbind();
        return ShaderProgram::unload();
    }
    /// Bind this shader program
    bool bind();
    /// Unbinding this program, unless forced, just clears the _bound flag
    void unbind(bool resetActiveProgram = true);
    /// Check every possible combination of flags to make sure this program can be used for rendering
    bool isValid() const;
    /// Called once per frame. Used to update internal state
    U8 update(const U64 deltaTime);
    /// Add a new shader stage to this program
    void attachShader(Shader* const shader,const bool refresh = false);
    /// Remove a shader stage from this program
    void detachShader(Shader* const shader);
    /// This is used to set all of the subroutine indices for the specified shader stage for this program
    void SetSubroutines(ShaderType type, const vectorImpl<U32>& indices) const;
    /// This works exactly like SetSubroutines, but for a single index. 
    void SetSubroutine(ShaderType type, U32 index) const;
    /// Get the index of the specified subroutine name for the specified stage. Not cached!
    U32  GetSubroutineIndex(ShaderType type, const std::string& name) const;
    /// Get the uniform location of the specified subroutine uniform for the specified stage. Not cached!
    U32  GetSubroutineUniformLocation(ShaderType type, const std::string& name) const;
    U32  GetSubroutineUniformCount(ShaderType type) const;
    /// Set an attribute value
    void Attribute(GLint location, GLdouble value) const;
    void Attribute(GLint location, GLfloat value) const;
    void Attribute(GLint location, const vec2<GLfloat>& value) const;
    void Attribute(GLint location, const vec3<GLfloat>& value) const;
    void Attribute(GLint location, const vec4<GLfloat>& value) const;
    /// Set an uniform value
    void Uniform(GLint location, U32 value) const;
    void Uniform(GLint location, I32 value) const;
    void Uniform(GLint location, F32 value) const;
    void Uniform(GLint location, const vec2<F32>& value) const;
    void Uniform(GLint location, const vec2<I32>& value) const;
    void Uniform(GLint location, const vec2<U16>& value) const;
    void Uniform(GLint location, const vec3<F32>& value) const;
    void Uniform(GLint location, const vec4<F32>& value) const;
    void Uniform(GLint location, const mat3<F32>& value, bool rowMajor = false) const;
    void Uniform(GLint location, const mat4<F32>& value, bool rowMajor = false) const;
    void Uniform(GLint location, const vectorImpl<I32 >& values) const;
    void Uniform(GLint location, const vectorImpl<F32 >& values) const;
    void Uniform(GLint location, const vectorImpl<vec2<F32> >& values) const;
    void Uniform(GLint location, const vectorImpl<vec3<F32> >& values) const;
    void Uniform(GLint location, const vectorImpl<vec4<F32> >& values) const;
    void Uniform(GLint location, const vectorImpl<mat3<F32> >& values, bool rowMajor = false) const;
    void Uniform(GLint location, const vectorImpl<mat4<F32> >& values, bool rowMajor = false) const;
    /// Bind a sampler to specific texture unit, checking double bind to avoid programming errors in debug
    void UniformTexture(GLint location, GLushort slot);
    /// Clear location cache if needed (e.g. after a refresh)
    inline void flushLocCache() { _shaderVars.clear();}

protected:
    /// Creation of a new shader program. Pass in a shader token and use glsw to load the corresponding effects
    bool generateHWResource(const std::string& name);
    /// Linking a shader program also sets up all pre-link properties for the shader (varying locations, attrib bindings, etc)
    void link();
    /// This should be called in the loading thread, but some issues are still present, and it's not recommended (yet)
    void threadedLoad(const std::string& name);
    /// Cache uniform/attribute locations for shader programs
    GLint cachedLoc(const std::string& name, const bool uniform = true);
    /// Basic OpenGL shader program validation (both in debug and in release)
    void validateInternal();
    /// Retrieve the program's validation log if we need it
    std::string getLog() const;
    /// Prevent binding multiple textures to the same slot.
    bool checkSlotUsage(GLint location, GLushort slot);
 
private:
    typedef Unordered_map<std::string, GLint > ShaderVarMap;
    typedef Unordered_map<GLushort, GLint >    TextureSlotMap;

    ShaderVarMap _shaderVars;
    TextureSlotMap _textureSlots;
    boost::atomic_bool _validationQueued;
    GLenum  _binaryFormat;
    bool    _validated;
    bool    _loadedFromBinary;
    Shader* _shaderStage[ShaderType_PLACEHOLDER];
    GLuint  _shaderProgramIDTemp;
    GLenum  _shaderStageTable[6];
};

#endif
