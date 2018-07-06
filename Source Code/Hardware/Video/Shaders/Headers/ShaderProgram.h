/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
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
	virtual U8   tick(const U32 deltaTime);

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

	inline void addShaderDefine(const std::string& define) {_definesList.push_back(define);}
           void removeShaderDefine(const std::string& define);

    inline void setShaderMask(const P32 mask) {
        _useVertex       = (mask.b.b0 == 1);
        _useFragment     = (mask.b.b1 == 1);
        _useGeometry     = (mask.b.b2 == 1);
        _useTessellation = (mask.b.b3 == 1);
        _compiled = false;
    }

	void uploadModelMatrices();

    void setMatrixMask(const bool uploadNormals = true,
                       const bool uploadModel = true,
                       const bool uploadModelView = true,
                       const bool uploadModelViewProjection = true);

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
	U32 _invalidShaderProgramId;
	I32 _maxCombinedTextureUnits;
    /**Skip or include certain matrices. Can change at any time:
    /*b0 = normal_matrix
    /*b1 = model_matrix
    /*b2 = model_view_matrix
    /*b3 = model_view_projection*/
    P32 _matrixMask;
	///A list of preprocessor defines
	vectorImpl<std::string > _definesList;
	///ID<->shaders pair
	typedef Unordered_map<U32, Shader* > ShaderIdMap;
	ShaderIdMap _shaderIdMap;
};

#endif
