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

#include "Hardware/Video/Shaders/Headers/ShaderProgram.h"

enum  UBO_NAME;
class glUniformBufferObject;
class glShaderProgram : public ShaderProgram {
public:
    glShaderProgram(const bool optimise = false);
    ~glShaderProgram();

    bool unload(){unbind(); return true;}
    void bind();
    void unbind(bool resetActiveProgram = true);
    U8   update(const U64 deltaTime);
    void attachShader(Shader* const shader,const bool refresh = false);
    void detachShader(Shader* const shader);
    //Attributes
    void Attribute(GLint location, GLdouble value) const;
    void Attribute(GLint location, GLfloat value) const;
    void Attribute(GLint location, const vec2<GLfloat>& value) const;
    void Attribute(GLint location, const vec3<GLfloat>& value) const;
    void Attribute(GLint location, const vec4<GLfloat>& value) const;
    //Uniforms (no redundant 'if(location == -1) return' checks as the driver already handles that)
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
    void Uniform(GLint location, const vectorImpl<mat4<F32> >& values, bool rowMajor = false) const;
    //Uniform Texture
    void UniformTexture(GLint location, GLushort slot) const;

    inline void  flushLocCache()                               { _shaderVars.clear();}

protected:
    void threadedLoad(const std::string& name);
    GLint cachedLoc(const std::string& name, const bool uniform = true);
    void validateInternal();
    std::string getLog() const;

private:
    typedef Unordered_map<std::string, GLint > ShaderVarMap;
    ShaderVarMap _shaderVars;
    vectorImpl<GLint> _UBOLocation;
    vectorImpl<glUniformBufferObject* > _uniformBufferObjects;
    boost::atomic_bool _validationQueued;
    U32 _shaderProgramIdTemp;
    GLenum _binaryFormat;
    bool   _loadedFromBinary;

protected:
    bool generateHWResource(const std::string& name);
    inline void validate() {_validationQueued = true;}
    void link();

protected:
    friend class glUniformBufferObject;
    inline GLuint getUBOLocation(const UBO_NAME& ubo) {assert((GLuint)ubo < _UBOLocation.size()); return _UBOLocation[ubo];}
    void initUBO();
};

#endif
