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
class ResourceCache;
class ShaderProgramDescriptor;

struct PushConstants;
struct Configuration;

enum class FileUpdateEvent : U8;

FWD_DECLARE_MANAGED_CLASS(ShaderProgram);

namespace Attorney {
    class ShaderProgramKernel;
}

using ModuleDefines = vectorEASTL<std::pair<stringImpl, bool>>;


struct ShaderModuleDescriptor {
    ModuleDefines _defines;
    Str64 _sourceFile;
    Str64 _variant;
    ShaderType _moduleType = ShaderType::COUNT;
    bool _batchSameFile = true;
};

void ProcessShadowMappingDefines(const Configuration& config, ModuleDefines& defines);

class ShaderProgramDescriptor final : public PropertyDescriptor {
public:
    ShaderProgramDescriptor() noexcept
        : PropertyDescriptor(DescriptorType::DESCRIPTOR_SHADER) {

    }

    size_t getHash() const noexcept override;

    vectorEASTL<ShaderModuleDescriptor> _modules;
};

class NOINITVTABLE ShaderProgram : public CachedResource,
                                   public GraphicsResource {
    friend class Attorney::ShaderProgramKernel;

   public:
    using ShaderProgramMapEntry = std::pair<ShaderProgram*, size_t>;
    using ShaderProgramMap = ska::bytell_hash_map<I64 /*handle*/, ShaderProgramMapEntry>;
    using AtomMap = ska::bytell_hash_map<U64 /*name hash*/, stringImpl>;
    //using AtomInclusionMap = ska::bytell_hash_map<U64 /*name hash*/, vectorEASTL<ResourcePath>>;
    using AtomInclusionMap = hashMap<U64 /*name hash*/, vectorEASTL<ResourcePath>>;
    using ShaderQueue = eastl::stack<ShaderProgram*, vectorEASTLFast<ShaderProgram*> >;


   public:
    explicit ShaderProgram(GFXDevice& context,
                           size_t descriptorHash,
                           const Str256& shaderName,
                           const Str256& shaderFileName,
                           const ResourcePath& shaderFileLocation,
                           ShaderProgramDescriptor descriptor,
                           bool asyncLoad);
    virtual ~ShaderProgram();

    /// Is the shader ready for drawing?
    virtual bool isValid() const = 0;
    bool load() override;
    bool unload() override;

    virtual bool recompile(bool force);

    /** ------ BEGIN EXPERIMENTAL CODE ----- **/
    size_t getFunctionCount(const ShaderType shader) noexcept {
        return _functionIndex[to_U32(shader)].size();
    }

    void setFunctionCount(const ShaderType shader, const size_t count) {
        _functionIndex[to_U32(shader)].resize(count, 0);
    }

    void setFunctionIndex(const ShaderType shader, const U32 index, const U32 functionEntry) {
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
        _functionIndex[shaderTypeValue][index] = _availableFunctionIndex[shaderTypeValue][functionEntry];
    }

    U32 addFunctionIndex(const ShaderType shader, const U32 index) {
        const U32 shaderTypeValue = to_U32(shader);

        _availableFunctionIndex[shaderTypeValue].push_back(index);
        return to_U32(_availableFunctionIndex[shaderTypeValue].size() - 1);
    }
    /** ------ END EXPERIMENTAL CODE ----- **/

    //==================== static methods ===============================//
    static void idle();
    static void onStartup(ResourceCache* parentCache);
    static void onShutdown();
    static bool updateAll();
    /// Queue a shaderProgram recompile request
    static bool recompileShaderProgram(const Str256& name);
    /// Remove a shaderProgram from the program cache
    static bool unregisterShaderProgram(size_t shaderHash);
    /// Add a shaderProgram to the program cache
    static void registerShaderProgram(ShaderProgram* shaderProgram);
    /// Find a specific shader program by handle.
    static ShaderProgram* findShaderProgram(I64 shaderHandle);
    /// Find a specific shader program by descriptor hash.
    static ShaderProgram* findShaderProgram(size_t shaderHash);

    /// Return a default shader used for general purpose rendering
    static const ShaderProgram_ptr& defaultShader();

    static const ShaderProgram_ptr& nullShader();

    static void rebuildAllShaders();

    static vectorEASTL<ResourcePath> getAllAtomLocations();

    static bool useShaderTexCache() noexcept { return s_useShaderTextCache; }
    static bool useShaderBinaryCache() noexcept { return s_useShaderBinaryCache; }

    static size_t definesHash(const ModuleDefines& defines);

    static I32 shaderProgramCount() noexcept { return s_shaderCount.load(std::memory_order_relaxed); }

    const ShaderProgramDescriptor& descriptor() const noexcept { return _descriptor; }

    [[nodiscard]] const char* getResourceTypeName() const noexcept override { return "ShaderProgram"; }

    PROPERTY_RW(bool, highPriority, true);

   protected:
     static void useShaderTextCache(bool state) noexcept { if (s_useShaderBinaryCache) { state = false; } s_useShaderTextCache = state; }
     static void useShaderBinaryCache(const bool state) noexcept { s_useShaderBinaryCache = state; if (state) { useShaderTextCache(false); } }

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
        std::array<vectorEASTL<U32>, to_base(ShaderType::COUNT)> _functionIndex;
        std::array<vectorEASTL<U32>, to_base(ShaderType::COUNT)> _availableFunctionIndex;

    protected:
        template <typename T>
        friend class ImplResourceLoader;

        const ShaderProgramDescriptor _descriptor;

        bool _asyncLoad;

        static bool s_useShaderTextCache;
        static bool s_useShaderBinaryCache;
        static std::atomic_int s_shaderCount;
};

namespace Attorney {
    class ShaderProgramKernel {
      protected:
        static void useShaderTextCache(const bool state) noexcept {
            ShaderProgram::useShaderTextCache(state);
        }

        static void useShaderBinaryCache(const bool state) {
            ShaderProgram::useShaderBinaryCache(state);
        }

        friend class Kernel;
    };
}

};  // namespace Divide
#endif //_SHADER_PROGRAM_H_
