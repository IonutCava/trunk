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

#ifndef _SHADER_PROGRAM_H_
#define _SHADER_PROGRAM_H_

#include "config.h"

#include "Core/Resources/Headers/Resource.h"
#include "Platform/Video/Headers/GraphicsResource.h"
#include "Platform/Video/Headers/RenderAPIEnums.h"

#include <stack>

namespace FW {
    class FileWatcher;
};

namespace Divide {

class Camera;
class Material;
class ShaderBuffer;
class GenericDrawCommand;
enum class FileUpdateEvent : U8;

FWD_DECLARE_MANAGED_CLASS(ShaderProgram);

class NOINITVTABLE ShaderProgram : public Resource, 
                                   public GraphicsResource {
   public:
    typedef hashMapImpl<U64, ShaderProgram_ptr> ShaderProgramMap;
    typedef hashMapImpl<U64, stringImpl> AtomMap;
    typedef std::stack<ShaderProgram_ptr, vectorImpl<ShaderProgram_ptr> > ShaderQueue;
    /// A list of built-in sampler slots. Use these if possible
    enum class TextureUsage : U32 {
        UNIT0 = 0,
        UNIT1 = 1,
        NORMALMAP = 2,
        OPACITY = 3,
        SPECULAR = 4,
        PROJECTION = 5,
        DEPTH = 6,
        DEPTH_PREV = 7,
        REFLECTION_PLANAR = 8,
        REFRACTION_PLANAR = 9,
        REFLECTION_CUBE = 10,
        REFRACTION_CUBE = 11,
        SHADOW_CUBE = 12,
        SHADOW_LAYERED = 13,
        SHADOW_SINGLE = 14,
        COUNT,

        GLOSS = SPECULAR,
        ROUGHNESS = GLOSS
    };

    enum class MemoryBarrierType : U32 {
        BUFFER = 0,
        TEXTURE = 1,
        RENDER_TARGET = 2,
        TRANSFORM_FEEDBACK = 3,
        COUNTER = 4,
        QUERY = 5,
        SHADER_BUFFER = 6,
        ALL = 7,
        COUNT
    };

    explicit ShaderProgram(GFXDevice& context,
                           const stringImpl& name,
                           const stringImpl& resourceName,
                           const stringImpl& resourceLocation,
                           bool asyncLoad);
    virtual ~ShaderProgram();

    virtual bool bind() = 0;
    /// Currently active
    virtual bool isBound() const = 0;
    /// Is the shader ready for drawing?
    virtual bool isValid() const = 0;
    virtual bool update(const U64 deltaTime);
    virtual bool load(const DELEGATE_CBK<void, Resource_wptr>& onLoadCallback) override;
    virtual bool unload() override;

    /// Uniforms (update constant buffer for D3D. Use index as location in
    /// buffer)
    virtual void Uniform(const char* ext, U32 value) = 0;
    virtual void Uniform(const char* ext, I32 value) = 0;
    virtual void Uniform(const char* ext, F32 value) = 0;
    virtual void Uniform(const char* ext, const vec2<F32>& value) = 0;
    virtual void Uniform(const char* ext, const vec2<I32>& value) = 0;
    virtual void Uniform(const char* ext, const vec3<F32>& value) = 0;
    virtual void Uniform(const char* ext, const vec3<I32>& value) = 0;
    virtual void Uniform(const char* ext, const vec4<F32>& value) = 0;
    virtual void Uniform(const char* ext, const vec4<I32>& value) = 0;
    virtual void Uniform(const char* ext, const mat3<F32>& value, bool rowMajor = false) = 0;
    virtual void Uniform(const char* ext, const mat4<F32>& value, bool rowMajor = false) = 0;
    virtual void Uniform(const char* ext, const vectorImpl<I32>& values) = 0;
    virtual void Uniform(const char* ext, const vectorImpl<F32>& values) = 0;
    virtual void Uniform(const char* ext, const vectorImpl<vec2<F32> >& values) = 0;
    virtual void Uniform(const char* ext, const vectorImpl<vec3<F32> >& values) = 0;
    virtual void Uniform(const char* ext, const vectorImplBest<vec4<F32> >& values) = 0;
    virtual void Uniform(const char* ext, const vectorImpl<mat3<F32> >& values, bool rowMajor = false) = 0;
    virtual void Uniform(const char* ext, const vectorImplBest<mat4<F32> >& values, bool rowMajor = false) = 0;

    inline void Uniform(const char* ext, TextureUsage slot) {
        Uniform(ext, to_int(slot));
    }
        
    inline void Uniform(const char* ext, bool value) {
        Uniform(ext, value ? 1 : 0);
    }

    virtual I32 getUniformLocation(const char* name) = 0;
    /// Subroutine
    virtual void SetSubroutines(ShaderType type, const vectorImpl<U32>& indices) const = 0;
    virtual void SetSubroutine(ShaderType type, U32 index) const = 0;
    virtual U32 GetSubroutineIndex(ShaderType type, const char* name) const = 0;
    virtual U32 GetSubroutineUniformLocation(ShaderType type, const char* name) const = 0;
    virtual U32 GetSubroutineUniformCount(ShaderType type) const = 0;
    /// Uniform+UniformTexture implementation
    virtual void Uniform(I32 location, U32 value) = 0;
    virtual void Uniform(I32 location, I32 value) = 0;
    virtual void Uniform(I32 location, F32 value) = 0;
    virtual void Uniform(I32 location, const vec2<F32>& value) = 0;
    virtual void Uniform(I32 location, const vec2<I32>& value) = 0;
    virtual void Uniform(I32 location, const vec3<F32>& value) = 0;
    virtual void Uniform(I32 location, const vec3<I32>& value) = 0;
    virtual void Uniform(I32 location, const vec4<F32>& value) = 0;
    virtual void Uniform(I32 location, const vec4<I32>& value) = 0;
    virtual void Uniform(I32 location,
                         const mat3<F32>& value,
                         bool rowMajor = false) = 0;
    virtual void Uniform(I32 location,
                         const mat4<F32>& value,
                         bool rowMajor = false) = 0;
    virtual void Uniform(I32 location, const vectorImpl<I32>& values) = 0;
    virtual void Uniform(I32 location, const vectorImpl<F32>& values) = 0;
    virtual void Uniform(I32 location,
                         const vectorImpl<vec2<F32> >& values) = 0;
    virtual void Uniform(I32 location,
                         const vectorImpl<vec3<F32> >& values) = 0;
    virtual void Uniform(I32 location,
                         const vectorImplBest<vec4<F32> >& values) = 0;
    virtual void Uniform(I32 location,
                         const vectorImpl<mat3<F32> >& values,
                         bool rowMajor = false) = 0;
    virtual void Uniform(I32 location,
                         const vectorImplBest<mat4<F32> >& values,
                         bool rowMajor = false) = 0;

    virtual void DispatchCompute(U32 xGroups, U32 yGroups, U32 zGroups) = 0;

    virtual void SetMemoryBarrier(MemoryBarrierType type) = 0;

    inline void Uniform(I32 location, TextureUsage slot) {
        Uniform(location, to_ubyte(slot));
    }

    inline void Uniform(I32 location, bool value) {
        Uniform(location, value ? 1 : 0);
    }

    /// ShaderProgram object id (i.e.: for OGL _shaderProgramID =
    /// glCreateProgram())
    inline U32 getID() const { return _shaderProgramID; }
    ///  Calling recompile will re-create the marked shaders from source files
    ///  and update them in the ShaderManager if needed
    bool recompile();
    /// Add a define to the shader. The defined must not have been added
    /// previously
    void addShaderDefine(const stringImpl& define);
    /// Remove a define from the shader. The defined must have been added
    /// previously
    void removeShaderDefine(const stringImpl& define);

    /** ------ BEGIN EXPERIMENTAL CODE ----- **/
    inline vectorAlg::vecSize getFunctionCount(ShaderType shader, U8 LoD) {
        return _functionIndex[to_uint(shader)][LoD].size();
    }

    inline void setFunctionCount(ShaderType shader,
                                 U8 LoD,
                                 vectorAlg::vecSize count) {
        _functionIndex[to_uint(shader)][LoD].resize(count, 0);
    }

    inline void setFunctionCount(ShaderType shader, vectorAlg::vecSize count) {
        for (U8 i = 0; i < Config::SCENE_NODE_LOD; ++i) {
            setFunctionCount(shader, i, count);
        }
    }

    inline void setFunctionIndex(ShaderType shader,
                                 U8 LoD,
                                 U32 index,
                                 U32 functionEntry) {
        U32 shaderTypeValue = to_uint(shader);

        if (_functionIndex[shaderTypeValue][LoD].empty()) {
            return;
        }

        DIVIDE_ASSERT(index < _functionIndex[shaderTypeValue][LoD].size(),
                      "ShaderProgram error: Invalid function index specified "
                      "for update!");
        if (_availableFunctionIndex[shaderTypeValue].empty()) {
            return;
        }

        DIVIDE_ASSERT(
            functionEntry < _availableFunctionIndex[shaderTypeValue].size(),
            "ShaderProgram error: Specified function entry does not exist!");
        _functionIndex[shaderTypeValue][LoD][index] =
            _availableFunctionIndex[shaderTypeValue][functionEntry];
    }

    inline U32 addFunctionIndex(ShaderType shader, U32 index) {
        U32 shaderTypeValue = to_uint(shader);

        _availableFunctionIndex[shaderTypeValue].push_back(index);
        return U32(_availableFunctionIndex[shaderTypeValue].size() - 1);
    }
    /** ------ END EXPERIMENTAL CODE ----- **/

    //==================== static methods ===============================//
    static void idle();
    static void onStartup(ResourceCache& parentCache);
    static void onShutdown();
    static bool updateAll(const U64 deltaTime);
    /// Queue a shaderProgram recompile request
    static bool recompileShaderProgram(const stringImpl& name);
    static const stringImpl& shaderFileRead(const stringImpl& atomName, const stringImpl& location);
    static void shaderFileRead(const stringImpl& filePath, bool buildVariant, stringImpl& sourceCodeOut);
    static void shaderFileWrite(const stringImpl& atomName, const char* sourceCode);
    /// Remove a shaderProgram from the program cache
    static bool unregisterShaderProgram(const stringImpl& name);
    /// Add a shaderProgram to the program cache
    static void registerShaderProgram(const stringImpl& name, const ShaderProgram_ptr& shaderProgram);
    /// Bind the null shader
    static bool unbind();
    /// Return a default shader used for general purpose rendering
    static const ShaderProgram_ptr& defaultShader();

    static void onAtomChange(const char* atomName, FileUpdateEvent evt);

    static void rebuildAllShaders();

    static vectorImpl<stringImpl> getAllAtomLocations();

   protected:
       virtual bool recompileInternal() = 0;

       void registerAtomFile(const stringImpl& atomFile);
   protected:
    /// Shaders loaded from files are kept as atoms
    static AtomMap _atoms;
    /// Used to render geometry without valid materials.
    /// Should emulate the basic fixed pipeline functions (no lights, just colour and texture)
    static ShaderProgram_ptr _imShader;
    /// Pointer to a shader that we will perform operations on
    static ShaderProgram_ptr _nullShader;
    /// Only 1 shader program per frame should be recompiled to avoid a lot of stuttering
    static ShaderQueue _recompileQueue;
    /// Shader program cache
    static ShaderProgramMap _shaderPrograms;

    static SharedLock _programLock;

   protected:
    template <typename T>
    friend class ImplResourceLoader;

    bool _asyncLoad;
    std::atomic_bool _linked;
    U32 _shaderProgramID;  //<not thread-safe. Make sure assignment is protected
    // with a mutex or something
    /// A list of preprocessor defines
    vectorImpl<stringImpl> _definesList;
    /// A list of atoms used by this program. (All stages are added toghether)
    vectorImpl<stringImpl> _usedAtoms;

   private:
    std::array<std::array<vectorImpl<U32>, Config::SCENE_NODE_LOD>, to_const_uint(ShaderType::COUNT)> _functionIndex;
    std::array<vectorImpl<U32>, to_const_uint(ShaderType::COUNT)>  _availableFunctionIndex;

    static std::unique_ptr<FW::FileWatcher> s_shaderFileWatcher;
};

};  // namespace Divide
#endif //_SHADER_PROGRAM_H_
