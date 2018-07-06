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

#ifndef _SHADER_HANDLER_H_
#define _SHADER_HANDLER_H_

#include "Utility/Headers/Vector.h"
#include "Core/Resources/Headers/HardwareResource.h"
#include "Hardware/Video/Headers/RenderAPIEnums.h"

class Shader;
enum ShaderType;
enum MATRIX_MODE;

class ShaderProgram : public HardwareResource {
public:
    virtual ~ShaderProgram();

    virtual void bind();
    virtual void unbind(bool resetActiveProgram = true);
    virtual U8   update(const U64 deltaTime);

    ///Attributes
    inline void Attribute(const std::string& ext, D32 value) { Attribute(cachedLoc(ext,false), value); }
    inline void Attribute(const std::string& ext, F32 value) { Attribute(cachedLoc(ext, false), value); }
    inline void Attribute(const std::string& ext, const vec2<F32>& value) { Attribute(cachedLoc(ext, false), value); }
    inline void Attribute(const std::string& ext, const vec3<F32>& value) { Attribute(cachedLoc(ext, false), value); }
    inline void Attribute(const std::string& ext, const vec4<F32>& value) { Attribute(cachedLoc(ext, false), value); }
    ///Uniforms (update constant buffer for D3D. Use index as location in buffer)
    inline void Uniform(const std::string& ext, U32 value) { Uniform(cachedLoc(ext), value); }
    inline void Uniform(const std::string& ext, I32 value) { Uniform(cachedLoc(ext), value); }
    inline void Uniform(const std::string& ext, F32 value) { Uniform(cachedLoc(ext), value); }
    inline void Uniform(const std::string& ext, bool value) { Uniform(cachedLoc(ext), value); }
    inline void Uniform(const std::string& ext, const vec2<F32>& value) { Uniform(cachedLoc(ext), value); }
    inline void Uniform(const std::string& ext, const vec2<I32>& value) { Uniform(cachedLoc(ext), value); }
    inline void Uniform(const std::string& ext, const vec2<U16>& value) { Uniform(cachedLoc(ext), value); }
    inline void Uniform(const std::string& ext, const vec3<F32>& value) { Uniform(cachedLoc(ext), value); }
    inline void Uniform(const std::string& ext, const vec4<F32>& value) { Uniform(cachedLoc(ext), value); }
    inline void Uniform(const std::string& ext, const mat3<F32>& value, bool rowMajor = false) { Uniform(cachedLoc(ext), value, rowMajor); }
    inline void Uniform(const std::string& ext, const mat4<F32>& value, bool rowMajor = false) { Uniform(cachedLoc(ext), value, rowMajor); }
    inline void Uniform(const std::string& ext, const vectorImpl<I32 >& values) { Uniform(cachedLoc(ext), values); }
    inline void Uniform(const std::string& ext, const vectorImpl<F32 >& values) { Uniform(cachedLoc(ext), values); }
    inline void Uniform(const std::string& ext, const vectorImpl<vec2<F32> >& values) { Uniform(cachedLoc(ext), values); }
    inline void Uniform(const std::string& ext, const vectorImpl<vec3<F32> >& values) { Uniform(cachedLoc(ext), values); }
    inline void Uniform(const std::string& ext, const vectorImpl<vec4<F32> >& values) { Uniform(cachedLoc(ext), values); }
    inline void Uniform(const std::string& ext, const vectorImpl<mat4<F32> >& values, bool rowMajor = false) { Uniform(cachedLoc(ext), values, rowMajor); }
    ///Uniform Texture
    inline void UniformTexture(const std::string& ext, U16 slot) { UniformTexture(cachedLoc(ext), slot); }

    ///Attribute+Uniform+UniformTexture implementation
    virtual void Attribute(I32 location, D32 value) const = 0;
    virtual void Attribute(I32 location, F32 value) const = 0;
    virtual void Attribute(I32 location, const vec2<F32>& value) const = 0;
    virtual void Attribute(I32 location, const vec3<F32>& value) const = 0;
    virtual void Attribute(I32 location, const vec4<F32>& value) const = 0;
    virtual void Uniform(I32 location, U32 value) const = 0;
    virtual void Uniform(I32 location, I32 value) const = 0;
    virtual void Uniform(I32 location, F32 value) const = 0;
    virtual void Uniform(I32 location, const vec2<F32>& value) const = 0;
    virtual void Uniform(I32 location, const vec2<I32>& value) const = 0;
    virtual void Uniform(I32 location, const vec2<U16>& value) const = 0;
    virtual void Uniform(I32 location, const vec3<F32>& value) const = 0;
    virtual void Uniform(I32 location, const vec4<F32>& value) const = 0;
    virtual void Uniform(I32 location, const mat3<F32>& value, bool rowMajor = false) const = 0;
    virtual void Uniform(I32 location, const mat4<F32>& value, bool rowMajor = false) const = 0;
    virtual void Uniform(I32 location, const vectorImpl<I32 >& values) const = 0;
    virtual void Uniform(I32 location, const vectorImpl<F32 >& values) const = 0;
    virtual void Uniform(I32 location, const vectorImpl<vec2<F32> >& values) const = 0;
    virtual void Uniform(I32 location, const vectorImpl<vec3<F32> >& values) const = 0;
    virtual void Uniform(I32 location, const vectorImpl<vec4<F32> >& values) const = 0;
    virtual void Uniform(I32 location, const vectorImpl<mat4<F32> >& values, bool rowMajor = false) const = 0;
    virtual void UniformTexture(I32, U16 slot) const = 0;

    inline void Uniform(I32 location, bool value) const { Uniform(location, value ? 1 : 0); }

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
    inline void setMatricesDirty() { _extendedMatricesDirty = true; }
    I32 operator==(const ShaderProgram &_v) { return this->getGUID() == _v.getGUID(); }
    I32 operator!=(const ShaderProgram &_v) { return !(*this == _v); }

protected:

    ShaderProgram(const bool optimise = false);
    void threadedLoad(const std::string& name);

    virtual I32 cachedLoc(const std::string& name, const bool uniform = true) = 0;
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
    U64 _elapsedTime;
    F32 _elapsedTimeMS;
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

    Unordered_map<EXTENDED_MATRIX, I32  > _extendedMatrixEntry;
    bool _extendedMatricesDirty;

    ///Various uniform/attribute locations
    I32 _timeLoc;
    I32 _cameraLocationLoc;
    I32 _clipPlanesLoc;
    I32 _clipPlaneCountLoc;
    I32 _clipPlanesActiveLoc;
    I32 _enableFogLoc;
    I32 _lightAmbientLoc;
    I32 _zPlanesLoc;
    I32 _screenDimensionLoc;
    I32 _texDepthMapFromLightArrayLoc;
    I32 _texDepthMapFromLightCubeLoc;
    I32 _texNormalMapLoc;
    I32 _texOpacityMapLoc;
    I32 _texSpecularLoc;
    I32 _fogColorLoc;
    I32 _fogDensityLoc;
    I32 _fogStartLoc;
    I32 _fogEndLoc;
    I32 _fogModeLoc;
    I32 _fogDetailLevelLoc;
};

#endif
