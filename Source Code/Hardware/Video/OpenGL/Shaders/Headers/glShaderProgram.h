/*
   Copyright (c) 2013 DIVIDE-Studio
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
    U8   update(const D32 deltaTime);
    void attachShader(Shader* const shader,const bool refresh = false);
    void detachShader(Shader* const shader);
    //Attributes
    void Attribute(const std::string& ext, GLdouble value);
    void Attribute(const std::string& ext, GLfloat value);
    void Attribute(const std::string& ext, const vec2<GLfloat>& value);
    void Attribute(const std::string& ext, const vec3<GLfloat>& value);
    void Attribute(const std::string& ext, const vec4<GLfloat>& value);
    //Uniforms
    void Uniform(const std::string& ext, GLuint value);
    void Uniform(const std::string& ext, GLint value);
    void Uniform(const std::string& ext, GLfloat value);
    void Uniform(const std::string& ext, const vec2<GLfloat>& value);
    void Uniform(const std::string& ext, const vec2<GLint>& value);
    void Uniform(const std::string& ext, const vec2<GLushort>& value);
    void Uniform(const std::string& ext, const vec3<GLfloat>& value);
    void Uniform(const std::string& ext, const vec4<GLfloat>& value);
    void Uniform(const std::string& ext, const mat3<GLfloat>& value, bool rowMajor = false);
    void Uniform(const std::string& ext, const mat4<GLfloat>& value, bool rowMajor = false);
    void Uniform(const std::string& ext, const vectorImpl<GLint >& values);
    void Uniform(const std::string& ext, const vectorImpl<GLfloat >& values);
    void Uniform(const std::string& ext, const vectorImpl<vec2<GLfloat> >& values);
    void Uniform(const std::string& ext, const vectorImpl<vec3<GLfloat> >& values);
    void Uniform(const std::string& ext, const vectorImpl<vec4<GLfloat> >& values);
    void Uniform(const std::string& ext, const vectorImpl<mat4<GLfloat> >& values, bool rowMajor = false);
    //Uniform Texture
    void UniformTexture(const std::string& ext, GLushort slot);

    inline GLint getAttributeLocation(const std::string& name) { return cachedLoc(name,false); }
    inline GLint getUniformLocation(const std::string& name)   { return cachedLoc(name,true);  }
    inline void  flushLocCache()                               { _shaderVars.clear();}

private:
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
