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

#ifndef _SHADER_HANDLER_H_
#define _SHADER_HANDLER_H_

#include "Utility/Headers/Vector.h"
#include "Core/Resources/Headers/HardwareResource.h"

class Shader;
enum ShaderType;
enum MATRIX_MODE;

class ShaderProgram : public HardwareResource {
public:
    virtual ~ShaderProgram();

    virtual void bind();
    virtual void unbind(bool resetActiveProgram = true);
    virtual U8   update(const D32 deltaTime);

    ///Attributes
    virtual void Attribute(const std::string& ext, D32 value) = 0;
    virtual void Attribute(const std::string& ext, F32 value) = 0 ;
    virtual void Attribute(const std::string& ext, const vec2<F32>& value) = 0;
    virtual void Attribute(const std::string& ext, const vec3<F32>& value) = 0;
    virtual void Attribute(const std::string& ext, const vec4<F32>& value) = 0;
    ///Uniforms
    virtual void Uniform(const std::string& ext, U32 value) = 0;
    virtual void Uniform(const std::string& ext, I32 value) = 0;
    virtual void Uniform(const std::string& ext, F32 value) = 0 ;
    virtual void Uniform(const std::string& ext, const vec2<F32>& value) = 0;
    virtual void Uniform(const std::string& ext, const vec2<I32>& value) = 0;
    virtual void Uniform(const std::string& ext, const vec2<U16>& value) = 0;
    virtual void Uniform(const std::string& ext, const vec3<F32>& value) = 0;
    virtual void Uniform(const std::string& ext, const vec4<F32>& value) = 0;
    virtual void Uniform(const std::string& ext, const mat3<F32>& value, bool rowMajor = false) = 0;
    virtual void Uniform(const std::string& ext, const mat4<F32>& value, bool rowMajor = false) = 0;
    virtual void Uniform(const std::string& ext, const vectorImpl<I32 >& values) = 0;
    virtual void Uniform(const std::string& ext, const vectorImpl<F32 >& values) = 0;
    virtual void Uniform(const std::string& ext, const vectorImpl<vec2<F32> >& values) = 0;
    virtual void Uniform(const std::string& ext, const vectorImpl<vec3<F32> >& values) = 0;
    virtual void Uniform(const std::string& ext, const vectorImpl<vec4<F32> >& values) = 0;
    virtual void Uniform(const std::string& ext, const vectorImpl<mat4<F32> >& values, bool rowMajor = false) = 0;
    ///Uniform Texture
    virtual void UniformTexture(const std::string& ext, U16 slot) = 0;
    virtual I32  getAttributeLocation(const std::string& name) = 0;
    virtual I32  getUniformLocation(const std::string& name) = 0;
    virtual void attachShader(Shader* const shader,const bool refresh = false) = 0;
    virtual void detachShader(Shader* const shader) = 0;
    ///ShaderProgram object id (i.e.: for OGL _shaderProgramId = glCreateProgram())
    inline U32  getId()   const { return _shaderProgramId; }
    ///Currently active
    inline bool isBound() const {return _bound;}

    //calling recompile will re-create the marked shaders from source files and update them in the ShaderManager if needed
           void recompile(const bool vertex, const bool fragment, const bool geometry = false, const bool tessellation = false);
    //calling refresh will force an update on default shader uniforms
           void refresh() {_dirty = true;}
    //add global shader defines
    inline void addShaderDefine(const std::string& define) {_definesList.push_back(define);}
           void removeShaderDefine(const std::string& define);
    //add either fragment or vertex uniforms (without the "uniform" word. e.g. addShaderUniform("vec3 eyePos", VERTEX_SHADER);)
           void addShaderUniform(const std::string& uniform, const ShaderType& type);
           void removeUniform(const std::string& uniform, const ShaderType& type);

    inline void setShaderMask(const P32 mask) {
        _useVertex       = (mask.b.b0 == 1);
        _useFragment     = (mask.b.b1 == 1);
        _useGeometry     = (mask.b.b2 == 1);
        _useTessellation = (mask.b.b3 == 1);
        _compiled = false;
    }

    void uploadNodeMatrices();

protected:
    friend class ShaderManager;
    vectorImpl<Shader* > getShaders(const ShaderType& type) const;

protected:

    ShaderProgram(const bool optimise = false);
    void threadedLoad(const std::string& name);

    virtual void validate() = 0;
    virtual void link() = 0;
    template<typename T>
    friend class ImplResourceLoader;
    virtual bool generateHWResource(const std::string& name);
    void updateMatrices();

protected:
    bool _useTessellation;
    bool _useGeometry;
    bool _useFragment;
    bool _useVertex;
    bool _refreshVert;
    bool _refreshFrag;
    bool _refreshGeom;
    bool _refreshTess;
    bool _optimise;
    bool _dirty;
    bool _wasBound;
    boost::atomic_bool _bound;
    boost::atomic_bool _compiled;
    U32 _shaderProgramId; //<not thread-safe. Make sure assignment is protected with a mutex or something
    I32 _maxCombinedTextureUnits;
    ///A list of preprocessor defines
    vectorImpl<std::string > _definesList;
    ///A list of vertex shader uniforms
    vectorImpl<std::string > _vertUniforms;
    ///A list of fragment shader uniforms
    vectorImpl<std::string > _fragUniforms;
    ///ID<->shaders pair
    typedef Unordered_map<U32, Shader* > ShaderIdMap;
    ShaderIdMap _shaderIdMap;
    ///cached clipping planes
    vectorImpl<vec4<F32> > _clipPlanes;
    ///cached clippin planes' states
    vectorImpl<I32 >       _clipPlanesStates;
private:
    ///Small hack to avoid stack allocations
    mat4<F32> _cachedMatrix;
    mat3<F32> _cachedNormalMatrix;
};

#endif
