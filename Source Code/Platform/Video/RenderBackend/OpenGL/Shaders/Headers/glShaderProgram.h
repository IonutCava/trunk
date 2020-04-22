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
                             const Str128& name,
                             const Str128& assetName,
                             const stringImpl& assetLocation,
                             const ShaderProgramDescriptor& descriptor,
                             bool asyncLoad);
    ~glShaderProgram();

    static void initStaticData();
    static void destroyStaticData();
    static void onStartup(GFXDevice& context, ResourceCache* parentCache);
    static void onShutdown();

    template<size_t N>
    static Str<N> decorateFileName(const Str<N>& name) {
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
    /// Check every possible combination of flags to make sure this program can
    /// be used for rendering
    bool isValid() const override;

    /// Get the index of the specified subroutine name for the specified stage.
    /// Not cached!
    U32 GetSubroutineIndex(ShaderType type, const char* name) override;
    U32 GetSubroutineUniformCount(ShaderType type) override;

    void UploadPushConstant(const GFX::PushConstant& constant);
    void UploadPushConstants(const PushConstants& constants);

    static void onAtomChange(const char* atomName, FileUpdateEvent evt);
    static const stringImpl& shaderFileRead(const Str256& filePath, const Str64& atomName, bool recurse, U32 level, vectorEASTL<Str64>& foundAtoms, bool& wasParsed);
    static const stringImpl& shaderFileReadLocked(const Str256& filePath, const Str64& atomName, bool recurse, U32 level, vectorEASTL<Str64>& foundAtoms, bool& wasParsed);

    static void shaderFileRead(const Str256& filePath, const Str128& fileName, stringImpl& sourceCodeOut);
    static void shaderFileWrite(const Str256& filePath, const Str128& fileName, const char* sourceCode);
    static stringImpl preprocessIncludes(const Str128& name,
                                         const stringImpl& source,
                                         GLint level,
                                         vectorEASTL<Str64>& foundAtoms,
                                         bool lock);

    void update(const U64 deltaTimeUS) override;
   protected:

    /// return a list of atom names
       vectorEASTL<Str64> loadSourceCode(ShaderType stage,
                                         const Str64& stageName,
                                         const Str8& extension,
                                         const stringImpl& header,
                                         U32 lineOffset,
                                         bool reloadExisting,
                                         std::pair<bool, stringImpl>& sourceCodeOut);

    bool rebindStages();
    void validatePreBind();
    void validatePostBind();

    bool shouldRecompile() const;
    bool recompileInternal(bool force) override;
    /// Creation of a new shader program. Pass in a shader token and use glsw to
    /// load the corresponding effects
    bool load() override;
    /// This should be called in the loading thread, but some issues are still
    /// present, and it's not recommended (yet)
    void threadedLoad(bool reloadExisting);

    /// Returns true if at least one shader linked succesfully
    bool reloadShaders(bool reloadExisting);

    /// This is used to set all of the subroutine indices for the specified
    /// shader stage for this program
    void SetSubroutines(ShaderType type, const vectorEASTL<U32>& indices) const;
    /// This works exactly like SetSubroutines, but for a single index.
    void SetSubroutine(ShaderType type, U32 index) const;
    /// Bind this shader program
    bool bind(bool& wasBound, bool& wasReady);
    /// Returns true if the shader is currently active
    bool isBound() const;

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

    static GLuint s_shadersUploadedThisFrame;
    //extra entry for "common" location
    static Str256 shaderAtomLocationPrefix[to_base(ShaderType::COUNT) + 1];
    static Str8 shaderAtomExtensionName[to_base(ShaderType::COUNT) + 1];
    static U64 shaderAtomExtensionHash[to_base(ShaderType::COUNT) + 1];
};

namespace Attorney {
    class GLAPIShaderProgram {
    private:
        static void setGlobalLineOffset(U32 offset) {
            glShaderProgram::_lineOffset.fill(offset);
        }
        static void addLineOffset(ShaderType stage, U32 offset) noexcept {
            glShaderProgram::_lineOffset[to_U32(stage)] += offset;
        }
        static bool bind(glShaderProgram& program, bool& wasBound, bool& wasReady) {
            return program.bind(wasBound, wasReady);
        }
        static void queueValidation(glShaderProgram& program) noexcept {
            // After using the shader at least once, validate the shader if needed
            if (!program._validated) {
                program._validationQueued = true;
            }
        }

        static void SetSubroutines(glShaderProgram& program, ShaderType type, const vectorEASTL<U32>& indices) {
            program.SetSubroutines(type, indices);
        }
        static void SetSubroutine(glShaderProgram& program, ShaderType type, U32 index) {
            program.SetSubroutine(type, index);
        }
        friend class Divide::GL_API;
    };
};  // namespace Attorney
};  // namespace Divide

#endif  //_PLATFORM_VIDEO_OPENGLS_PROGRAM_H_

#include "glShaderProgram.inl"
