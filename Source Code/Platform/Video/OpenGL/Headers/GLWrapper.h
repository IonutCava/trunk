/*
   Copyright (c) 2016 DIVIDE-Studio
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
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/OpenGL/Shaders/Headers/glShaderProgram.h"
#include "Platform/Video/OpenGL/Shaders/Headers/glShader.h"
#include "Platform/Video/OpenGL/Textures/Headers/glSamplerObject.h"
#include "Platform/Video/OpenGL/Textures/Headers/glTexture.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

struct glslopt_ctx;
struct FONScontext;

namespace CEGUI {
    class OpenGL3Renderer;
};

namespace Divide {
    static const U32 MAX_ACTIVE_TEXTURE_SLOTS = 64;

    enum class WindowType : U32;

    class DisplayWindow;

/// OpenGL implementation of the RenderAPIWrapper
DEFINE_SINGLETON_W_SPECIFIER(GL_API, RenderAPIWrapper, final)
    friend class glShader;
    friend class glTexture;
    friend class glIMPrimitive;
    friend class glFramebuffer;
    friend class glVertexArray;
    friend class glShaderProgram;
    friend class glSamplerObject;
    friend class glGenericVertexData;

  protected:
    GL_API();
    ~GL_API();

    /// Try and create a valid OpenGL context taking in account the specified
    /// command line arguments
    ErrorCode initRenderingAPI(I32 argc, char** argv) override;
    /// Clear everything that was setup in initRenderingAPI()
    void closeRenderingAPI() override;
    /// Prepare the GPU for rendering a frame
    void beginFrame() override;
    /// Finish rendering the current frame
    void endFrame(bool swapBuffers) override;
  
    inline void toggleDepthWrites(bool state) override {
        glDepthMask(state ? GL_TRUE : GL_FALSE);
    }

    /// Enable or disable rasterization (useful for transform feedback)
    inline void toggleRasterization(bool state) override {
        state ? glDisable(GL_RASTERIZER_DISCARD) : glEnable(GL_RASTERIZER_DISCARD);
    }
    /// Verify if we have a sampler object created and available for the given
    /// descriptor
    static size_t getOrCreateSamplerObject(const SamplerDescriptor& descriptor);
    /// Clipping planes are only enabled/disabled if they differ from the current
    /// state
    void updateClipPlanes() override;
    /// Text rendering is handled exclusively by Mikko Mononen's FontStash library
    /// (https://github.com/memononen/fontstash)
    /// with his OpenGL frontend adapted for core context profiles
    void drawText(const TextLabel& textLabel, const vec2<F32>& position) override;
    void draw(const GenericDrawCommand& cmd) override;
    /// This functions should be run in a separate, consumer thread.
    /// The main app thread, the producer, adds tasks via a lock-free queue that is
    /// checked every 20 ms
    void syncToThread(std::thread::id threadID) override;
    /// Return the time it took to render a single frame (in nanoseconds). Only
    /// works in GPU validation builds
    GLuint64 getFrameDurationGPU() override;
    /// Return the OpenGL framebuffer handle bound and assigned for the specified usage
    inline static GLuint getActiveFB(RenderTarget::RenderTargetUsage usage) {
        return _activeFBID[to_uint(usage)];
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

    bool makeTexturesResident(const TextureDataContainer& textureData) override;
    bool makeTextureResident(const TextureData& textureData) override;

  public:
    /// Enable or disable primitive restart and ensure that the correct index size
    /// is used
    static void togglePrimitiveRestart(bool state);
    /// Set the currently active texture unit
    static bool setActiveTextureUnit(GLushort unit);
    /// Set the currently active texture unit
    static bool setActiveTextureUnit(GLushort unit, GLuint& previousUnit);
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
    /// Switch the current framebuffer by binding it as either a R/W buffer, read
    /// buffer or write buffer
    static bool setActiveFB(RenderTarget::RenderTargetUsage usage, GLuint ID, GLuint& previousID);
    /// Bind the specified transform feedback object
    static bool setActiveTransformFeedback(GLuint ID);
    /// Bind the specified transform feedback object
    static bool setActiveTransformFeedback(GLuint ID, GLuint& previousID);
    /// Change the currently active shader program.
    static bool setActiveProgram(GLuint programHandle);
    /// A state block should contain all rendering state changes needed for the next
    /// draw call.
    /// Some may be redundant, so we check each one individually
    void activateStateBlock(const RenderStateBlock& newBlock,
                            const RenderStateBlock& oldBlock) const override;
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
    static bool bindTexture(GLushort unit, GLuint handle, GLenum target, size_t samplerHash = 0);
    static bool bindTextureImage(GLushort unit, GLuint handle, GLint level,
                                 bool layered, GLint layer, GLenum access,
                                 GLenum format);
    /// Bind multiple textures specified by an array of handles and an offset
    /// unit
    static bool bindTextures(GLushort unitOffset, GLuint textureCount,
                             GLuint* textureHandles, GLenum* targets,
                             GLuint* samplerHandles);

    /// Bind the sampler object described by the hash value to the specified
    /// unit
    static bool bindSampler(GLushort unit, size_t samplerHash);
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
  private:
    /// Prepare our shader loading system
    bool initShaders();
    /// Revert everything that was set up in "initShaders()"
    bool deInitShaders();

    ErrorCode createGLContext(const DisplayWindow& window);
    ErrorCode destroyGLContext();
    /// FontStash library initialization
    bool createFonsContext();
    /// FontStash library deinitialization
    void deleteFonsContext();
    /// Use GLSW to append tokens to shaders. Use ShaderType::COUNT to append to
    /// all stages
    typedef std::array<GLint, to_const_uint(ShaderType::COUNT) + 1> ShaderOffsetArray;
    void appendToShaderHeader(ShaderType type, const stringImpl& entry,
                              ShaderOffsetArray& inOutOffset);

  private:
    /// The previous Text3D node's font face size
    GLfloat _prevSizeNode;
    /// The previous plain text string's font face size
    GLfloat _prevSizeString;
    /// The previous Text3D node's line width
    GLfloat _prevWidthNode;
    /// The previous plain text string's line width
    GLfloat _prevWidthString;
    /// Line width limit (hardware upper limit)
    GLint _lineWidthLimit;
    /// Used to render points (e.g. to render full screen quads with geometry shaders)
    GLuint _dummyVAO;
    /// Number of available texture units
    static GLint _maxTextureUnits;
    /// Number of available attribute binding indices
    static GLint _maxAttribBindings;
    /// Used to store all of the indirect draw commands
    static GLuint _indirectDrawBuffer;
    /// A cache of all fonts used
    typedef hashMapImpl<ULL, I32> FontCache;
    FontCache _fonts;
    hashAlg::pair<stringImpl, I32> _fontCache;
    /// Current active vertex array object's handle
    static GLuint _activeVAOID;
    /// 0 - current framebuffer, 1 - current read only framebuffer, 2 - current
    /// write only framebuffer
    static GLuint _activeFBID[3];
    /// VB, IB, SB, TB, UB, PUB, DIB
    static GLuint _activeBufferID[7];
    static GLuint _activeTextureUnit;
    static GLuint _activeTransformFeedback;
    static GLint _activePackUnpackAlignments[2];
    static GLint _activePackUnpackRowLength[2];
    static GLint _activePackUnpackSkipPixels[2];
    static GLint _activePackUnpackSkipRows[2];
    static GLuint _activeShaderProgram;
    /// Boolean value used to verify if primitive restart index is enabled or
    /// disabled
    static bool _primitiveRestartEnabled;
    /// Current state of all available clipping planes
    std::array<bool, Config::MAX_CLIP_PLANES> _activeClipPlanes;
    /// Hardware query objects used for performance measurements
    vectorImpl<glHardwareQueryRing*> _hardwareQueries;
    /// Duration in nanoseconds to render a frame
    GLuint64 FRAME_DURATION_GPU;
    /// FontStash's context
    FONScontext* _fonsContext;
    /// /*texture slot*/ /*<texture handle , texture type>*/
    typedef std::array<std::pair<GLuint, GLenum>, MAX_ACTIVE_TEXTURE_SLOTS> textureBoundMapDef;
    static textureBoundMapDef _textureBoundMap;

    typedef std::array<ImageBindSettings, MAX_ACTIVE_TEXTURE_SLOTS> imageBoundMapDef;
    static imageBoundMapDef _imageBoundMap;

    /// /*texture slot*/ /*sampler hash value*/
    typedef std::array<size_t, MAX_ACTIVE_TEXTURE_SLOTS> samplerBoundMapDef;
    static samplerBoundMapDef _samplerBoundMap;
    /// /*sampler hash value*/ /*sampler object*/
    typedef hashMapImpl<size_t, glSamplerObject*> samplerObjectMap;
    static SharedLock _samplerMapLock;
    static samplerObjectMap _samplerMap;

    typedef std::tuple<GLuint, GLintptr, GLsizei> BufferBindingParams;
    typedef hashMapImpl<GLuint /*bind index*/, BufferBindingParams> VAOBufferData;
    typedef hashMapImpl<GLuint /*vao ID*/, VAOBufferData> VAOBindings;
    static VAOBindings _vaoBufferData;

    CEGUI::OpenGL3Renderer* _GUIGLrenderer;


    Time::ProfileTimer& _swapBufferTimer;
END_SINGLETON

};  // namespace Divide
#endif
