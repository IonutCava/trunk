/*
   Copyright (c) 2015 DIVIDE-Studio
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

#include "glResources.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/OpenGL/Shaders/Headers/glShaderProgram.h"
#include "Platform/Video/OpenGL/Shaders/Headers/glShader.h"
#include "Platform/Video/OpenGL/Textures/Headers/glSamplerObject.h"
#include "Platform/Video/OpenGL/Textures/Headers/glTexture.h"
#include "Platform/Video/Buffers/Framebuffer/Headers/Framebuffer.h"

struct glslopt_ctx;
struct FONScontext;

namespace CEGUI {
    class OpenGL3Renderer;
};

namespace Divide {
    enum class WindowType : U32;

/// OpenGL implementation of the RenderAPIWrapper
DEFINE_SINGLETON_EXT1_W_SPECIFIER(GL_API, RenderAPIWrapper, final)
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
    /// Prepare our shader loading system
    bool initShaders() override;
    /// Revert everything that was set up in "initShaders()"
    bool deInitShaders() override;
    /// Window positioning is handled by SDL
    void setWindowPosition(U16 w, U16 h);
    /// Centering is also easier via SDL
    void centerWindowPosition();
    /// Mouse positioning is handled by SDL
    void setCursorPosition(U16 x, U16 y) override;
    /// Prepare the GPU for rendering a frame
    void beginFrame() override;
    /// Finish rendering the current frame
    void endFrame() override;
    /// Create and return a new IM emulation primitive. The callee is responsible
    /// for it's deletion!
    IMPrimitive* newIMP() const override;
    /// Create and return a new framebuffer. The callee is responsible for it's
    /// deletion!
    Framebuffer* newFB(bool multisampled) const override;
    /// Create and return a new vertex array (VAO + VB + IB). The callee is
    /// responsible for it's deletion!
    VertexBuffer* newVB() const override;
    /// Create and return a new pixel buffer using the requested format. The callee
    /// is responsible for it's deletion!
    PixelBuffer* newPB(const PBType& type) const override;
    /// Create and return a new generic vertex data object and, optionally set it as
    /// persistently mapped.
    /// The callee is responsible for it's deletion!
    GenericVertexData* newGVD(const bool persistentMapped) const override;
    /// Create and return a new shader buffer. The callee is responsible for it's
    /// deletion!
    /// The OpenGL implementation creates either an 'Uniform Buffer Object' if
    /// unbound is false
    /// or a 'Shader Storage Block Object' otherwise
    ShaderBuffer* newSB(const stringImpl& bufferName, const bool unbound = false,
                        const bool persistentMapped = true) const override;
    /// Create and return a new texture array (optionally, flipped vertically). The
    /// callee is responsible for it's deletion!
    Texture* newTextureArray() const override;
    /// Create and return a new 2D texture (optionally, flipped vertically). The
    /// callee is responsible for it's deletion!
    Texture* newTexture2D() const override;
    /// Create and return a new cube texture (optionally, flipped vertically). The
    /// callee is responsible for it's deletion!
    Texture* newTextureCubemap() const override;
    /// Create and return a new shader program.
    /// The callee is responsible for it's deletion!
    ShaderProgram* newShaderProgram() const override;
    /// Create and return a new shader of the specified type by loading the
    /// specified name (optionally, post load optimised).
    /// The callee is responsible for it's deletion!
    Shader* newShader(const stringImpl& name, const ShaderType& type,
                      const bool optimise = false) const override;
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
    void drawText(const TextLabel& textLabel, const vec2<F32>& relativeOffset) override;
    /// Rendering points is universally useful, so we have a function, and a VAO,
    /// dedicated to this process
    void drawPoints(U32 numPoints) override;
    /// This functions should be run in a separate, consumer thread.
    /// The main app thread, the producer, adds tasks via a lock-free queue that is
    /// checked every 20 ms
    void threadedLoadCallback() override;
    /// Return the time it took to render a single frame (in nanoseconds). Only
    /// works in debug builds
    GLuint64 getFrameDurationGPU() override;
    /// Return the OpenGL framebuffer handle bound and assigned for the specified
    /// usage
    inline static GLuint getActiveFB(Framebuffer::FramebufferUsage usage) {
        return _activeFBID[to_uint(usage)];
    }
    /// Change the clear color for the specified renderTarget
    static void clearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a, vec4<GLfloat>& prevColor);
    /// Change the clear color for the specified renderTarget
    inline static void clearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
        static vec4<GLfloat> prevColor;
        clearColor(r, g, b, a, prevColor);
    }
        /// Change the clear color for the specified renderTarget
    inline static void clearColor(const vec4<GLfloat>& color, vec4<GLfloat>& prevColor) {
        clearColor(color.r, color.g, color.b, color.a, prevColor);
    }
    /// Change the clear color for the specified renderTarget
    inline static void clearColor(const vec4<GLfloat>& color) {
        clearColor(color.r, color.g, color.b, color.a);
    }
    /// Try to find the requested font in the font cache. Load on cache miss.
    I32 getFont(const stringImpl& fontName);
    /// Internally change window size
    void changeWindowSize(U16 w, U16 h);
    /// Change rendering resolution
    void changeResolution(U16 w, U16 h) override;
    /// Change the current viewport area. Redundancy check is performed in GFXDevice
    /// class
    void changeViewport(const vec4<GLint>& newViewport) const override;
    /// Reset as much of the GL default state as possible within the limitations
    /// given
    void clearStates(const bool skipTextures,
                     const bool skipBuffers,
                     const bool skipScissor);
    void uploadDrawCommands(const DrawCommandList& drawCommands,
                            U32 commandCount) const override;

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
    static bool setActiveFB(Framebuffer::FramebufferUsage usage, GLuint ID);
    /// Switch the current framebuffer by binding it as either a R/W buffer, read
    /// buffer or write buffer
    static bool setActiveFB(Framebuffer::FramebufferUsage usage, GLuint ID, GLuint& previousID);
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
                            RenderStateBlock* const oldBlock) const override;
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
    static bool bindTexture(GLushort unit, GLuint handle, GLenum target,
                            size_t samplerHash = 0);
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

  private:
    ErrorCode createWindow();
    ErrorCode createGLContext();
    ErrorCode destroyWindow();
    ErrorCode destroyGLContext();
    void handleChangeWindowType(WindowType newWindowType);
    void pollWindowEvents();
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
    /// The current rendering window type
    WindowType _crtWindowType;
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
    /// Used to render points (e.g. to render full screen quads with geometry
    /// shaders)
    GLuint _pointDummyVAO;
    /// Number of available texture units
    static GLint _maxTextureUnits;
    /// Used to store all of the indirect draw commands
    static GLuint _indirectDrawBuffer;
    /// A cache of all fonts used
    typedef hashMapImpl<stringImpl, I32> FontCache;
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
    static vec4<GLfloat> _prevClearColor;
    /// Boolean value used to verify if primitive restart index is enabled or
    /// disabled
    static bool _primitiveRestartEnabled;
    /// Toggle CEGUI rendering on/off (e.g. to check raw application rendering
    /// performance)
    bool _enableCEGUIRendering;
    /// Did we generate the window move event?
    bool _internalMoveEvent;
    /// Current state of all available clipping planes
    bool _activeClipPlanes[Config::MAX_CLIP_PLANES];
    /// Performance counters: front x 2 and back x 2
    static const GLint PERFORMANCE_COUNTER_BUFFERS = 4;
    /// Number of queries
    static const GLint PERFORMANCE_COUNTERS = 1;
    /// Unique handle for every query object
    GLuint _queryID[PERFORMANCE_COUNTER_BUFFERS][PERFORMANCE_COUNTERS];
    /// Current query ID used for writing to
    GLuint _queryBackBuffer;
    /// Current query ID used for reading from
    GLuint _queryFrontBuffer;
    /// Duration in nanoseconds to render a frane
    GLuint64 FRAME_DURATION_GPU;
    /// FontStash's context
    FONScontext* _fonsContext;
    /// /*texture slot*/ /*<texture handle , texture type>*/
    typedef hashMapImpl<GLushort, std::pair<GLuint, GLenum> > textureBoundMapDef;
    static textureBoundMapDef _textureBoundMap;
    /// /*texture slot*/ /*sampler hash value*/
    typedef hashMapImpl<GLushort, size_t> samplerBoundMapDef;
    static samplerBoundMapDef _samplerBoundMap;
    /// /*sampler hash value*/ /*sampler object*/
    typedef hashMapImpl<size_t, glSamplerObject*> samplerObjectMap;
    static samplerObjectMap _samplerMap;
    CEGUI::OpenGL3Renderer* _GUIGLrenderer;

END_SINGLETON

};  // namespace Divide
#endif
