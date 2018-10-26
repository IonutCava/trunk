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
#ifndef _GL_WRAPPER_H_
#define _GL_WRAPPER_H_

#include "config.h"

#include "GLStateTracker.h"
#include "glHardwareQueryPool.h"
#include "Rendering/Camera/Headers/Frustum.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/RenderBackend/OpenGL/Shaders/Headers/glShaderProgram.h"
#include "Platform/Video/RenderBackend/OpenGL/Shaders/Headers/glShader.h"
#include "Platform/Video/RenderBackend/OpenGL/Textures/Headers/glSamplerObject.h"
#include "Platform/Video/RenderBackend/OpenGL/Textures/Headers/glTexture.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/PixelBuffer/Headers/glPixelBuffer.h"
#include "Platform/Video/RenderBackend/OpenGL/Buffers/VertexBuffer/Headers/glVAOPool.h"

#include <unordered_set>

struct glslopt_ctx;
struct FONScontext;
struct ImDrawData;

namespace CEGUI {
    class OpenGL3Renderer;
};

namespace Divide {

namespace Time {
    class ProfileTimer;
};

enum class WindowType : U8;

class DisplayWindow;
class PlatformContext;
class RenderStateBlock;
class GenericVertexData;
class glHardwareQueryRing;
class glBufferLockManager;

namespace GLUtil {
    class glVAOCache;
};

struct GLConfig {
    bool _glES = false;
};


/// OpenGL implementation of the RenderAPIWrapper
class GL_API final : public RenderAPIWrapper {
    friend class glShader;
    friend class glTexture;
    friend class GLUtil::glVAOCache;
    friend class glIMPrimitive;
    friend class glFramebuffer;
    friend class glVertexArray;
    friend class glShaderProgram;
    friend class glSamplerObject;
    friend class glGenericVertexData;

    friend struct GLStateTracker;
public:
    GL_API(GFXDevice& context, const bool glES);
    ~GL_API();

protected:
    /// Try and create a valid OpenGL context taking in account the specified
    /// command line arguments
    ErrorCode initRenderingAPI(I32 argc, char** argv, Configuration& config) override;
    /// Clear everything that was setup in initRenderingAPI()
    void closeRenderingAPI() override;
    /// Prepare the GPU for rendering a frame
    void beginFrame(DisplayWindow& window, bool global = false) override;
    /// Finish rendering the current frame
    void endFrame(DisplayWindow& window, bool global = false) override;

    /// Verify if we have a sampler object created and available for the given
    /// descriptor
    static U32 getOrCreateSamplerObject(const SamplerDescriptor& descriptor);

    /// Return the OpenGL sampler object's handle for the given hash value
    static GLuint getSamplerHandle(size_t samplerHash);
    /// Clipping planes are only enabled/disabled if they differ from the current
    /// state
    void updateClipPlanes(const FrustumClipPlanes& list) override;
    /// Text rendering is handled exclusively by Mikko Mononen's FontStash library
    /// (https://github.com/memononen/fontstash)
    /// with his OpenGL frontend adapted for core context profiles
    void drawText(const TextElementBatch& batch);
    void drawIMGUI(ImDrawData* data, I64 windowGUID);

    bool draw(const GenericDrawCommand& cmd);

    /// Sets the current state block to the one passed as a param
    size_t setStateBlock(size_t stateBlockHash) override;

    void flushCommand(const GFX::CommandBuffer::CommandEntry& entry, const GFX::CommandBuffer& commandBuffer) override;
    void postFlushCommandBuffer(const GFX::CommandBuffer& commandBuffer);

    /// Return the time it took to render a single frame (in nanoseconds). Only
    /// works in GPU validation builds
    F32 getFrameDurationGPU() const override;

    /// Return the size in pixels that we can render to. This differs from the window size on Retina displays
    vec2<U16> getDrawableSize(const DisplayWindow& window) const override;

    U32 getHandleFromCEGUITexture(const CEGUI::Texture& textureIn) const override;

    void onThreadCreated(const std::thread::id& threadID) override;

    /// Try to find the requested font in the font cache. Load on cache miss.
    I32 getFont(const stringImpl& fontName);

    /// Reset as much of the GL default state as possible within the limitations given
    void clearStates(const DisplayWindow& window, GLStateTracker& stateTracker, bool global);

    bool makeTexturesResident(const TextureDataContainer& textureData);
    bool makeTextureResident(const TextureData& textureData, U8 binding);

    bool setViewport(const Rect<I32>& viewport) override;

public:
    static GLStateTracker& getStateTracker();

    /// Makes sure that the calling thread has a valid GL context. If not, a new one is created.
    static void createOrValidateContextForCurrentThread(GFXDevice& context);

    /// Queue a mipmap recalculation
    static void queueComputeMipMap(PlatformContext& context, GLuint textureHandle, bool threaded);

    static void pushDebugMessage(const char* message, I32 id);
    static void popDebugMessage();

    static bool deleteTextures(GLuint count, GLuint* textures);
    static bool deleteSamplers(GLuint count, GLuint* samplers);
    static bool deleteBuffers(GLuint count, GLuint* buffers);
    static bool deleteVAOs(GLuint count, GLuint* vaos);
    static bool deleteFramebuffers(GLuint count, GLuint* framebuffers);
    static bool deleteShaderPrograms(GLuint count, GLuint* programs);

    static void registerBufferBind(const BufferWriteData& data);

private:
    /// Prepare our shader loading system
    static bool initShaders();
    /// Revert everything that was set up in "initShaders()"
    static bool deInitShaders();

    static bool initGLSW();
    static bool deInitGLSW();

    bool bindPipeline(const Pipeline& pipeline);
    void sendPushConstants(const PushConstants& pushConstants);
    void dispatchCompute(const ComputeParams& computeParams);

    ErrorCode createGLContext(const DisplayWindow& window);
    ErrorCode destroyGLContext();
    /// FontStash library initialization
    bool createFonsContext();
    /// FontStash library deinitialization
    void deleteFonsContext();
    /// Use GLSW to append tokens to shaders. Use ShaderType::COUNT to append to
    /// all stages
    typedef std::array<GLint, to_base(ShaderType::COUNT) + 1> ShaderOffsetArray;
    static void appendToShaderHeader(ShaderType type, const stringImpl& entry, ShaderOffsetArray& inOutOffset);

    GenericVertexData* getOrCreateIMGUIBuffer(I64 windowGUID);

protected:
    /// Number of available texture units
    static GLint s_maxTextureUnits;
    /// Number of available attribute binding indices
    static GLint s_maxAttribBindings;
    /// Max number of texture attachments to an FBO
    static GLint s_maxFBOAttachments;

public:
    /// Shader block data
    static GLuint s_UBOffsetAlignment;
    static GLuint s_UBMaxSize;
    static GLuint s_SSBOffsetAlignment;
    static GLuint s_SSBMaxSize;
    static glHardwareQueryPool* s_hardwareQueryPool;
    static GLConfig s_glConfig;

private:
    GFXDevice& _context;
    /// The previous Text3D node's font face size
    GLfloat _prevSizeNode;
    /// The previous plain text string's font face size
    GLfloat _prevSizeString;
    /// The previous Text3D node's line width
    GLfloat _prevWidthNode;
    /// The previous plain text string's line width
    GLfloat _prevWidthString;
    /// Line width limit (hardware upper limit)
    static GLint s_lineWidthLimit;
    /// Used to render points (e.g. to render full screen quads with geometry shaders)
    static GLuint s_dummyVAO;
    /// Preferred anisotropic filtering level
    static GLuint s_anisoLevel;
    /// A cache of all fonts used
    typedef hashMap<U64, I32> FontCache;
    FontCache _fonts;
    hashAlg::pair<stringImpl, I32> _fontCache;

    /// Current state of all available clipping planes
    std::array<bool, to_base(Frustum::FrustPlane::COUNT)> _activeClipPlanes;
    /// Hardware query objects used for performance measurements
    std::shared_ptr<glHardwareQueryRing> _elapsedTimeQuery;

    /// Duration in milliseconds to render a frame
    F32 FRAME_DURATION_GPU;
    /// FontStash's context
    FONScontext* _fonsContext;

    static std::mutex s_mipmapQueueSetLock;
    static hashMap<GLuint, GLsync> GL_API::s_mipmapQueueSync;
    /// The main VAO pool. We use a pool to avoid multithreading issues with VAO states
    static GLUtil::glVAOPool s_vaoPool;

    /// /*sampler hash value*/ /*sampler object*/
    typedef hashMap<size_t, GLuint> samplerObjectMap;
    static std::mutex s_samplerMapLock;
    static samplerObjectMap s_samplerMap;

    I32 _glswState;
    CEGUI::OpenGL3Renderer* _GUIGLrenderer;
    hashMap<I64, GenericVertexData*> _IMGUIBuffers;
    Time::ProfileTimer& _swapBufferTimer;

    static bool s_bufferBindsNeedsFlush;
    static moodycamel::ConcurrentQueue<BufferWriteData> s_bufferBinds;

    typedef std::unordered_map<I64, GLStateTracker> stateTrackerMap;
    static stateTrackerMap s_stateTrackers;
    static GLStateTracker* s_activeStateTracker;


    std::pair<I64, SDL_GLContext> _currentContext;
};

};  // namespace Divide
#endif
