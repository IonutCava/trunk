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

#ifndef HLSL_H_
#define HLSL_H_

#include "Hardware/Video/Shaders/Headers/ShaderProgram.h"

class d3dShaderProgram : public ShaderProgram {
public:
    d3dShaderProgram(const bool optimise = false) : ShaderProgram(optimise) {};
    ~d3dShaderProgram(){};

    bool unload(){unbind(); return true;}
    void bind(){}
    void unbind(bool resetActiveProgram = true){}
    void attachShader(Shader* const shader,const bool refresh = false){}
    void detachShader(Shader* const shader) {}
    //Attributes
    void Attribute(const std::string& ext, D32 value){}
    void Attribute(const std::string& ext, F32 value){}
    void Attribute(const std::string& ext, const vec2<F32>& value){}
    void Attribute(const std::string& ext, const vec3<F32>& value){}
    void Attribute(const std::string& ext, const vec4<F32>& value){}
    //Uniforms
    void Uniform(const std::string& ext, U32 value){}
    void Uniform(const std::string& ext, I32 value){}
    void Uniform(const std::string& ext, F32 value){}
    void Uniform(const std::string& ext, const vec2<F32>& value){}
    void Uniform(const std::string& ext, const vec2<I32>& value){}
    void Uniform(const std::string& ext, const vec2<U16>& value){}
    void Uniform(const std::string& ext, const vec3<F32>& value){}
    void Uniform(const std::string& ext, const vec4<F32>& value){}
    void Uniform(const std::string& ext, const mat3<F32>& value, bool rowMajor = false){}
    void Uniform(const std::string& ext, const mat4<F32>& value, bool rowMajor = false){}
    void Uniform(const std::string& ext, const vectorImpl<I32 >& values) {}
    void Uniform(const std::string& ext, const vectorImpl<F32 >& values) {}
    void Uniform(const std::string& ext, const vectorImpl<vec2<F32> >& values) {}
    void Uniform(const std::string& ext, const vectorImpl<vec3<F32> >& values) {}
    void Uniform(const std::string& ext, const vectorImpl<vec4<F32> >& values) {}
    void Uniform(const std::string& ext, const vectorImpl<mat4<F32> >& values, bool rowMajor = false){}
    //Uniform Texture
    void UniformTexture(const std::string& ext, U16 slot){}

    I32 getAttributeLocation(const std::string& name);
    I32 getUniformLocation(const std::string& name);

private:
    I32   cachedLoc(const std::string& name,bool uniform = true){return 0;}
    bool flushLocCache(){return true;}

private:
    Unordered_map<std::string, I32 > _shaderVars;

protected:
    inline bool generateHWResource(const std::string& name){return ShaderProgram::generateHWResource(name);}
    void validate(){}
    void link(){}
};

#endif
