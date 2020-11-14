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
#ifndef _PLATFORM_VIDEO_OPENGLS_PROGRAM_H_
#define _PLATFORM_VIDEO_OPENGLS_PROGRAM_H_

#include "glShaderProgram.h"
#include "AI/ActionInterface/Headers/AIProcessor.h"
#include "GLIM/Declarations.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/glResources.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {
class GL_API;
class glShader;
class glLockManager;
namespace Attorney {
    class GLAPIShaderProgram;
};
/// OpenGL implementation of the ShaderProgram entity
class glShaderProgram final : public ShaderProgram, public glObject {
    friend class Attorney::GLAPIShaderProgram;
   public:
    explicit glShaderProgram(GFXDevice& context,
                             size_t descriptorHash,
                             const Str256& name,
                             const Str256& assetName,
                             const ResourcePath& assetLocation,
                             const ShaderProgramDescriptor& descriptor,
                             bool asyncLoad);
    ~glShaderProgram();

    static void initStaticData();
    static void destroyStaticData();
    static void onStartup(GFXDevice& context, ResourceCache* parentCache);
    static void onShutdown();

    template<typename StringType> 
    static StringType decorateFileName(const StringType& name) {
        if_constexpr(Config::Build::IS_DEBUG_BUILD) {
            return "DEBUG." + name;
        } else if_constexpr(Config::Build::IS_PROFILE_BUILD) {
            return "PROFILE." + name;
        } else {
            return "RELEASE." + name;
        }
    }

    /// Make sure this program is ready for deletion
    bool unload() override;
    /// Check every possible combination of flags to make sure this program can be used for rendering
    bool isValid() const override;

    void UploadPushConstant(const GFX::PushConstant& constant);
    void UploadPushConstants(const PushConstants& constants);

    static void onAtomChange(std::string_view atomName, FileUpdateEvent evt);
    static const stringImpl& ShaderFileRead(const ResourcePath& filePath, const ResourcePath& atomName, bool recurse, vectorEASTL<ResourcePath>& foundAtoms, bool& wasParsed);
    static const stringImpl& ShaderFileReadLocked(const ResourcePath& filePath, const ResourcePath& atomName, bool recurse, vectorEASTL<ResourcePath>& foundAtoms, bool& wasParsed);

    static void shaderFileRead(const ResourcePath& filePath, const ResourcePath& fileName, stringImpl& sourceCodeOut);
    static void shaderFileWrite(const ResourcePath& filePath, const ResourcePath& fileName, const char* sourceCode);
    static stringImpl preprocessIncludes(const ResourcePath& name,
                                         const stringImpl& source,
                                         GLint level,
                                         vectorEASTL<ResourcePath>& foundAtoms,
                                         bool lock);
   protected:
    /// return a list of atom names
    vectorEASTL<ResourcePath> loadSourceCode(
        const Str128& stageName,
        const Str8& extension,
        const stringImpl& header,
        size_t definesHash,
        U32 lineOffset,
        bool reloadExisting,
        std::pair<bool, stringImpl>& sourceCodeOut) const;

    bool rebindStages();
    void validatePreBind();
    void validatePostBind();

    bool shouldRecompile() const;
    bool recompile(bool force) override;
    /// Creation of a new shader program. Pass in a shader token and use glsw to
    /// load the corresponding effects
    bool load() override;
    /// This should be called in the loading thread, but some issues are still
    /// present, and it's not recommended (yet)
    void threadedLoad(bool reloadExisting);

    /// Returns true if at least one shader linked successfully
    bool reloadShaders(bool reloadExisting);

    /// Bind this shader program (returns false if the program was ready/failed validation)
    std::pair<bool/*success*/, bool/*was bound*/> bind();
    /// Returns true if the shader is currently active
    bool isBound() const noexcept;

   private:
    GLuint _handle;
    bool _validationQueued;
    bool _validated;

    UseProgramStageMask _stageMask = UseProgramStageMask::GL_NONE_BIT;
    vectorEASTL<glShader*> _shaderStage;
    static std::array<U32, to_base(ShaderType::COUNT)> _lineOffset;

    static I64 s_shaderFileWatcherID;

    /// Shaders loaded from files are kept as atoms
    static SharedMutex s_atomLock;
    static AtomMap s_atoms;
    static AtomInclusionMap s_atomIncludes;

    //extra entry for "common" location
    static ResourcePath shaderAtomLocationPrefix[to_base(ShaderType::COUNT) + 1];
    static Str8 shaderAtomExtensionName[to_base(ShaderType::COUNT) + 1];
    static U64 shaderAtomExtensionHash[to_base(ShaderType::COUNT) + 1];
};

namespace Attorney {
    class GLAPIShaderProgram {
        static void setGlobalLineOffset(U32 offset) {
            glShaderProgram::_lineOffset.fill(offset);
        }
        static void addLineOffset(ShaderType stage, U32 offset) noexcept {
            glShaderProgram::_lineOffset[to_U32(stage)] += offset;
        }
        static std::pair<bool, bool> bind(glShaderProgram& program) {
            return program.bind();
        }
        static void queueValidation(glShaderProgram& program) noexcept {
            // After using the shader at least once, validate the shader if needed
            if (!program._validated) {
                program._validationQueued = true;
            }
        }

        friend class GL_API;
    };
};  // namespace Attorney
};  // namespace Divide

#endif  //_PLATFORM_VIDEO_OPENGLS_PROGRAM_H_

#include "glShaderProgram.inl"
