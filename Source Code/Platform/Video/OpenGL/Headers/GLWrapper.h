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

#ifndef _GL_WRAPPER_H_
#define _GL_WRAPPER_H_

#include "config.h"

#include "glResources.h"
#include "glHardwareQueryPool.h"
#include "Rendering/Camera/Headers/Frustum.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/OpenGL/Shaders/Headers/glShaderProgram.h"
#include "Platform/Video/OpenGL/Shaders/Headers/glShader.h"
#include "Platform/Video/OpenGL/Textures/Headers/glSamplerObject.h"
#include "Platform/Video/OpenGL/Textures/Headers/glTexture.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"
#include "Platform/Video/OpenGL/Buffers/VertexBuffer/Headers/glVAOPool.h"

struct glslopt_ctx;
struct FONScontext;

namespace CEGUI {
    class OpenGL3Renderer;
};

namespace Divide {

namespace Time {
    class ProfileTimer;
};

static const U32 MAX_ACTIVE_TEXTURE_SLOTS = 64;

enum class WindowType : U32;

class DisplayWindow;
class RenderStateBlock;
class glHardwareQueryRing;

namespace GLUtil {
    class glVAOCache;
};

struct GLConfig {
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

public:
    GL_API(GFXDevice& context);
    ~GL_API();

protected:
    /// Try and create a valid OpenGL context taking in account the specified
    /// command line arguments
    ErrorCode initRenderingAPI(I32 argc, char** argv, Configuration& config) override;
    /// Clear everything that was setup in initRenderingAPI()
    void closeRenderingAPI() override;
    /// Prepare the GPU for rendering a frame
    void beginFrame() override;
    /// Finish rendering the current frame
    void endFrame() override;

    /// Verify if we have a sampler object created and available for the given
    /// descriptor
    static size_t getOrCreateSamplerObject(const SamplerDescriptor& descriptor);
    /// Clipping planes are only enabled/disabled if they differ from the current
    /// state
    void updateClipPlanes() override;
    /// Text rendering is handled exclusively by Mikko Mononen's FontStash library
    /// (https://github.com/memononen/fontstash)
    /// with his OpenGL frontend adapted for core context profiles
    void drawText(const vectorImpl<GUITextBatchEntry>& batch) override;
    bool draw(const GenericDrawCommand& cmd) override;

    /// Sets the current state block to the one passed as a param
    size_t setStateBlock(size_t stateBlockHash) override;

    void flushCommandBuffer(CommandBuffer& commandBuffer) override;

    /// Return the time it took to render a single frame (in nanoseconds). Only
    /// works in GPU validation builds
    U32 getFrameDurationGPU() override;
    void pushDebugMessage(const char* message, I32 id) override;
    void popDebugMessage() override;
    /// Return the OpenGL framebuffer handle bound and assigned for the specified usage
    inline static GLuint getActiveFB(RenderTarget::RenderTargetUsage usage) {
        return s_activeFBID[to_U32(usage)];
    }
    /// Try to find the requested font in the font cache. Load on cache miss.
    I32 getFont(const stringImpl& fontName);
    /// Change the current viewport area. Redundancy check is performed in GFXDevice
    /// class
    void changeViewport(const vec4<GLint>& newViewport) const override;
    /// Reset as much of the GL default state as possible within the limitations
    /// given
    void clearStates();
    void registerCommandBuffer(const ShaderBuffer& commandBuffer) const override;

    bool makeTexturesResident(const TextureDataContainer& textureData);
    bool makeTextureResident(const TextureData& textureData);

public:
    /// Makes sure that the calling thread has a valid GL context. If not, a new one is created.
    static void createOrValidateContextForCurrentThread(GFXDevice& context);

    /// Enable or disable primitive restart and ensure that the correct index size is used
    static void togglePrimitiveRestart(bool state);
    /// Enable or disable primitive rasterization
    static void toggleRasterization(bool state);
    /// Set the number of vertices per patch used in tessellation
    static void setPatchVertexCount(U32 count);
    /// Switch the currently active vertex array object
    static bool setActiveVAO(GLuint ID);
    /// Switch the currently active vertex array object
    static bool setActiveVAO(GLuint ID, GLuint& previousID);
    /// Single place to change buffer objects for every target available
    static bool setActiveBuffer(GLenum target, GLuint ID);
    /// Single place to change buffer objects for every target available
    static bool setActiveBuffer(GLenum target, GLuint ID, GLuint& previousID);
    /// Switch the current framebuffer by binding it as either a R/W buffer, read
    /// buffer or write buffer
    static bool setActiveFB(RenderTarget::RenderTargetUsage usage, GLuint ID);
    /// Set a new depth range. Default is 0 - 1 with 0 mapping to the near plane and 1 to the far plane
    static void setDepthRange(F32 nearVal, F32 farVal);
    /// Set the blending properties for the specified draw buffer
    static void setBlending(GLuint drawBufferIdx, bool enable, const BlendingProperties& blendingProperties, const vec4<U8>& blendColour);
    /// Switch the current framebuffer by binding it as either a R/W buffer, read
    /// buffer or write buffer
    static bool setActiveFB(RenderTarget::RenderTargetUsage usage, GLuint ID, GLuint& previousID);
    /// Bind the specified transform feedback object
    static bool setActiveTransformFeedback(GLuint ID);
    /// Bind the specified transform feedback object
    static bool setActiveTransformFeedback(GLuint ID, GLuint& previousID);
    /// Change the currently active shader program.
    static bool setActiveProgram(GLuint programHandle);
    /// A state block should contain all rendering state changes needed for the next draw call.
    /// Some may be redundant, so we check each one individually
    void activateStateBlock(const RenderStateBlock& newBlock,
                            const RenderStateBlock& oldBlock) const;
    void activateStateBlock(const RenderStateBlock& newBlock) const;
    /// Pixel pack and unpack alignment is usually changed by textures, PBOs, etc
    static bool setPixelPackUnpackAlignment(GLint packAlignment = 1,
        GLint unpackAlignment = 1) {
        return (setPixelPackAlignment(packAlignment) &&
            setPixelUnpackAlignment(unpackAlignment));
    }
    /// Pixel pack alignment is usually changed by textures, PBOs, etc
    static bool setPixelPackAlignment(GLint packAlignment = 1, GLint rowLength = 0,
        GLint skipRows = 0, GLint skipPixels = 0);
    /// Pixel unpack alignment is usually changed by textures, PBOs, etc
    static bool setPixelUnpackAlignment(GLint unpackAlignment = 1,
        GLint rowLength = 0, GLint skipRows = 0,
        GLint skipPixels = 0);
    /// Bind a texture specified by a GL handle and GL type to the specified
    /// unit
    /// using the sampler object defined by hash value
    static bool bindTexture(GLushort unit, GLuint handle, size_t samplerHash = 0);
    static bool bindTextureImage(GLushort unit, GLuint handle, GLint level,
        bool layered, GLint layer, GLenum access,
        GLenum format);
    /// Bind multiple textures specified by an array of handles and an offset
    /// unit
    static bool bindTextures(GLushort unitOffset, GLuint textureCount, GLuint* textureHandles, GLuint* samplerHandles);

    /// Bind multiple samplers described by the array of hash values to the
    /// consecutive texture units starting from the specified offset
    static bool bindSamplers(GLushort unitOffset, GLuint samplerCount,
        GLuint* samplerHandles);
    /// Return the OpenGL sampler object's handle for the given hash value
    static GLuint getSamplerHandle(size_t samplerHash);
    /// Modify buffer bindings for a specific vao
    static bool bindActiveBuffer(GLuint vaoID,
        GLuint location,
        GLuint bufferID,
        GLintptr offset,
        GLsizei stride);

    /// Called right before a glDraw* command
    static void preCommandSubmission();
    /// Called right after a glDraw* command returns
    static void postCommandSubmission();
private:
    /// Prepare our shader loading system
    bool initShaders();
    /// Revert everything that was set up in "initShaders()"
    bool deInitShaders();

    bool setState(const GenericDrawCommand& cmd);

    ErrorCode createGLContext(const DisplayWindow& window);
    ErrorCode destroyGLContext();
    /// FontStash library initialization
    bool createFonsContext();
    /// FontStash library deinitialization
    void deleteFonsContext();
    /// Use GLSW to append tokens to shaders. Use ShaderType::COUNT to append to
    /// all stages
    typedef std::array<GLint, to_base(ShaderType::COUNT) + 1> ShaderOffsetArray;
    void appendToShaderHeader(ShaderType type, const stringImpl& entry,
        ShaderOffsetArray& inOutOffset);
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
    /// Used to store all of the indirect draw commands
    static GLuint s_indirectDrawBuffer;
    /// Preferred anisotropic filtering level
    static GLuint s_anisoLevel;
    /// A cache of all fonts used
    typedef hashMapImpl<U64, I32> FontCache;
    FontCache _fonts;
    hashAlg::pair<stringImpl, I32> _fontCache;
    /// Current active vertex array object's handle
    static GLuint s_activeVAOID;
    /// 0 - current framebuffer, 1 - current read only framebuffer, 2 - current write only framebuffer
    static GLuint s_activeFBID[3];
    /// VB, IB, SB, TB, UB, PUB, DIB
    static GLuint s_activeBufferID[7];
    static GLuint s_activeTransformFeedback;
    static GLint  s_activePackUnpackAlignments[2];
    static GLint  s_activePackUnpackRowLength[2];
    static GLint  s_activePackUnpackSkipPixels[2];
    static GLint  s_activePackUnpackSkipRows[2];
    static GLuint s_activeShaderProgram;
    static GLfloat s_depthNearVal;
    static GLfloat s_depthFarVal;
    static GLboolean s_blendEnabled;
    static vectorImpl<BlendingProperties> s_blendProperties;
    static vec4<U8> s_blendColour;
    /// The main VAO pool. We use a pool to avoid multithreading issues with VAO states
    static GLUtil::glVAOPool s_vaoPool;

    /// Boolean value used to verify if primitive restart index is enabled or
    /// disabled
    static bool s_primitiveRestartEnabled;
    static bool s_rasterizationEnabled;
    static U32  s_patchVertexCount;
    /// Current state of all available clipping planes
    std::array<bool, to_base(Frustum::FrustPlane::COUNT)> _activeClipPlanes;
    /// Hardware query objects used for performance measurements
    std::shared_ptr<glHardwareQueryRing> _elapsedTimeQuery;

    /// Duration in milliseconds to render a frame
    GLuint FRAME_DURATION_GPU;
    /// FontStash's context
    FONScontext* _fonsContext;
    /// /*texture slot*/ /*texture handle*/
    typedef std::array<GLuint, MAX_ACTIVE_TEXTURE_SLOTS> textureBoundMapDef;
    static textureBoundMapDef s_textureBoundMap;

    typedef std::array<ImageBindSettings, MAX_ACTIVE_TEXTURE_SLOTS> imageBoundMapDef;
    static imageBoundMapDef s_imageBoundMap;

    /// /*texture slot*/ /*sampler handle*/
    typedef std::array<GLuint, MAX_ACTIVE_TEXTURE_SLOTS> samplerBoundMapDef;
    static samplerBoundMapDef s_samplerBoundMap;
    /// /*sampler hash value*/ /*sampler object*/
    typedef hashMapImpl<size_t, glSamplerObject> samplerObjectMap;
    static SharedLock s_samplerMapLock;
    static samplerObjectMap s_samplerMap;

    static VAOBindings s_vaoBufferData;

    CEGUI::OpenGL3Renderer* _GUIGLrenderer;

    /* GL State management */
    size_t _currentStateBlockHash;
    size_t _previousStateBlockHash;

    Time::ProfileTimer& _swapBufferTimer;
};

};  // namespace Divide
#endif
