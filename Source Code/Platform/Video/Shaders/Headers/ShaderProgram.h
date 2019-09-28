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
#ifndef _SHADER_PROGRAM_H_
#define _SHADER_PROGRAM_H_

#include "config.h"

#include "Core/Resources/Headers/Resource.h"
#include "Core/Resources/Headers/ResourceDescriptor.h"
#include "Platform/Video/Headers/GraphicsResource.h"
#include "Platform/Video/Headers/RenderAPIEnums.h"

namespace FW {
    class FileWatcher;
};

namespace Divide {

class Kernel;
class Camera;
class Material;
class ShaderBuffer;
class ShaderProgramDescriptor;

struct PushConstants;
struct GenericDrawCommand;

enum class FileUpdateEvent : U8;

FWD_DECLARE_MANAGED_CLASS(ShaderProgram);

namespace Attorney {
    class ShaderProgramKernel;
}

typedef vectorEASTL<std::pair<stringImpl, bool>> ModuleDefines;

class NOINITVTABLE ShaderProgram : public CachedResource,
                                   public GraphicsResource {
    friend class Attorney::ShaderProgramKernel;

   public:
    using ShaderProgramMapEntry = std::pair<ShaderProgram*, size_t>;
    using ShaderProgramMap = ska::bytell_hash_map<I64 /*handle*/, ShaderProgramMapEntry>;
    using AtomMap = ska::bytell_hash_map<U64 /*name hash*/, stringImpl>;
    using ShaderQueue = std::stack<ShaderProgram*, vectorFast<ShaderProgram*> >;

    /// A list of built-in sampler slots. Use these if possible and keep them sorted by how often they are used
    enum class TextureUsage : U8 {
        UNIT0 = 0,
        NORMALMAP = 1,
        HEIGHTMAP = 2,
        SHADOW_LAYERED = 3,
        DEPTH = 4,
        SHADOW_SINGLE = 5,
        REFLECTION_CUBE = 6,
        SHADOW_CUBE = 7,
        OPACITY = 8,
        SPECULAR = 9,
        UNIT1 = 10,
        PROJECTION = 11,
        REFLECTION_PLANAR = 12,
        REFRACTION_PLANAR = 13,
        PREPASS_SHADOWS = 14,
        DEPTH_PREV = 15,
        COUNT,

        GLOSS = SPECULAR,
        ROUGHNESS = GLOSS,
        TERRAIN_SPLAT = OPACITY,
        TERRAIN_ALBEDO_TILE = UNIT0,
        TERRAIN_NORMAL_TILE = UNIT1,
        TERRAIN_EXTRA_TILE = PROJECTION,
        TERRAIN_HELPER_TEXTURES = ROUGHNESS
    };

   public:
    explicit ShaderProgram(GFXDevice& context,
                           size_t descriptorHash,
                           const stringImpl& shaderName,
                           const stringImpl& shaderFileName,
                           const stringImpl& shaderFileLocation,
                           const ShaderProgramDescriptor& descriptor,
                           bool asyncLoad);
    virtual ~ShaderProgram();

    /// Is the shader ready for drawing?
    virtual bool isValid() const = 0;
    virtual bool load() override;
    virtual bool unload() noexcept override;

    /// Subroutine
    virtual U32 GetSubroutineIndex(ShaderType type, const char* name) = 0;
    virtual U32 GetSubroutineUniformCount(ShaderType type) = 0;

    ///  Calling recompile will re-create the marked shaders from source files
    ///  and update them in the ShaderManager if needed
    bool recompile(bool force);

    virtual void update(const U64 deltaTimeUS) = 0;

    /** ------ BEGIN EXPERIMENTAL CODE ----- **/
    inline vec_size getFunctionCount(ShaderType shader) {
        return _functionIndex[to_U32(shader)].size();
    }

    inline void setFunctionCount(ShaderType shader,
                                 vec_size count) {
        _functionIndex[to_U32(shader)].resize(count, 0);
    }

    inline void setFunctionIndex(ShaderType shader,
                                 U32 index,
                                 U32 functionEntry) {
        const U32 shaderTypeValue = to_U32(shader);

        if (_functionIndex[shaderTypeValue].empty()) {
            return;
        }

        DIVIDE_ASSERT(index < _functionIndex[shaderTypeValue].size(),
                      "ShaderProgram error: Invalid function index specified "
                      "for update!");
        if (_availableFunctionIndex[shaderTypeValue].empty()) {
            return;
        }

        DIVIDE_ASSERT(
            functionEntry < _availableFunctionIndex[shaderTypeValue].size(),
            "ShaderProgram error: Specified function entry does not exist!");
        _functionIndex[shaderTypeValue][index] =
            _availableFunctionIndex[shaderTypeValue][functionEntry];
    }

    inline U32 addFunctionIndex(ShaderType shader, U32 index) {
        U32 shaderTypeValue = to_U32(shader);

        _availableFunctionIndex[shaderTypeValue].push_back(index);
        return U32(_availableFunctionIndex[shaderTypeValue].size() - 1);
    }
    /** ------ END EXPERIMENTAL CODE ----- **/

    //==================== static methods ===============================//
    static void idle();
    static void onStartup(GFXDevice& context, ResourceCache& parentCache);
    static void onShutdown();
    static bool updateAll(const U64 deltaTimeUS);
    /// Queue a shaderProgram recompile request
    static bool recompileShaderProgram(const stringImpl& name);
    /// Remove a shaderProgram from the program cache
    static bool unregisterShaderProgram(size_t shaderHash);
    /// Add a shaderProgram to the program cache
    static void registerShaderProgram(ShaderProgram* shaderProgram);
    /// Find a specific shader program by handle. Returns the default shader on failure
    static ShaderProgram& findShaderProgram(I64 shaderHandle, bool& success);
    /// Find a specific shader program by descriptor hash. Returns the default shader on failure;
    static ShaderProgram& findShaderProgram(size_t shaderHash, bool& success);

    /// Return a default shader used for general purpose rendering
    static const ShaderProgram_ptr& defaultShader();

    static const ShaderProgram_ptr& nullShader();

    static void rebuildAllShaders();

    static vector<stringImpl> getAllAtomLocations();

    static bool useShaderTexCache() { return s_useShaderTextCache; }
    static bool useShaderBinaryCache() { return s_useShaderBinaryCache; }

    static size_t definesHash(const ModuleDefines& defines);

    static I32 shaderProgramCount() { return s_shaderCount.load(std::memory_order_relaxed); }

    const char* getResourceTypeName() const override { return "ShaderProgram"; }

   protected:
     virtual bool recompileInternal(bool force) = 0;

     static void useShaderTextCache(bool state) { if (s_useShaderBinaryCache) { state = false; } s_useShaderTextCache = state; }
     static void useShaderBinaryCache(bool state) { s_useShaderBinaryCache = state; if (state) { useShaderTextCache(false); } }

   protected:
    /// Used to render geometry without valid materials.
    /// Should emulate the basic fixed pipeline functions (no lights, just colour and texture)
    static ShaderProgram_ptr s_imShader;
    /// Pointer to a shader that we will perform operations on
    static ShaderProgram_ptr s_nullShader;
    /// Only 1 shader program per frame should be recompiled to avoid a lot of stuttering
    static ShaderQueue s_recompileQueue;
    /// Shader program cache
    static ShaderProgramMap s_shaderPrograms;

    static SharedMutex s_programLock;

    private:
        std::array<vector<U32>, to_base(ShaderType::COUNT)> _functionIndex;
        std::array<vector<U32>, to_base(ShaderType::COUNT)> _availableFunctionIndex;

    protected:
        template <typename T>
        friend class ImplResourceLoader;

        PROPERTY_RW(bool, highPriority, true);

        bool _shouldRecompile;
        bool _asyncLoad;

        static bool s_useShaderTextCache;
        static bool s_useShaderBinaryCache;
        static std::atomic_int s_shaderCount;
};

namespace Attorney {
    class ShaderProgramKernel {
      protected:
        static void useShaderTextCache(bool state) { 
            ShaderProgram::useShaderTextCache(state);
        }

        static void useShaderBinaryCache(bool state) {
            ShaderProgram::useShaderBinaryCache(state);
        }

        friend class Divide::Kernel;
    };
}

struct ShaderModuleDescriptor {
    ModuleDefines _defines;
    stringImpl _variant;
    stringImpl _sourceFile;
    ShaderType _moduleType = ShaderType::COUNT;
    bool _batchSameFile = true;
};

class ShaderProgramDescriptor final : public PropertyDescriptor {
public:
    ShaderProgramDescriptor()
        : PropertyDescriptor(DescriptorType::DESCRIPTOR_SHADER) {

    }

    void clone(std::shared_ptr<PropertyDescriptor>& target) const final {
        target.reset(new ShaderProgramDescriptor(*this));
    }

    inline size_t getHash() const final {
        _hash = PropertyDescriptor::getHash();
        for (const ShaderModuleDescriptor& desc : _modules) {
            Util::Hash_combine(_hash, ShaderProgram::definesHash(desc._defines));
            Util::Hash_combine(_hash, desc._variant);
            Util::Hash_combine(_hash, desc._sourceFile);
            Util::Hash_combine(_hash, desc._moduleType);
            Util::Hash_combine(_hash, desc._batchSameFile);
        }
        return _hash;
    }
    vectorEASTL<ShaderModuleDescriptor> _modules;
    
};

};  // namespace Divide
#endif //_SHADER_PROGRAM_H_
