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

namespace Divide {

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
    /// resolution and command line arguments
    ErrorCode initRenderingApi(const vec2<GLushort>& resolution, GLint argc,
                               char** argv) override;
    /// Clear everything that was setup in initRenderingApi()
    void closeRenderingApi() override;
    /// Prepare our shader loading system
    bool initShaders() override;
    /// Revert everything that was set up in "initShaders()"
    bool deInitShaders() override;
    /// Window positioning is handled by GLFW
    void setWindowPos(GLushort w, GLushort h) const override;
    /// Mouse positioning is handled by GLFW
    void setCursorPosition(GLushort x, GLushort y) const override;
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
    ShaderBuffer* newSB(const bool unbound = false,
                        const bool persistentMapped = true) const override;
    /// Create and return a new texture array (optionally, flipped vertically). The
    /// callee is responsible for it's deletion!
    Texture* newTextureArray(const bool flipped = false) const override;
    /// Create and return a new 2D texture (optionally, flipped vertically). The
    /// callee is responsible for it's deletion!
    Texture* newTexture2D(const bool flipped = false) const override;
    /// Create and return a new cube texture (optionally, flipped vertically). The
    /// callee is responsible for it's deletion!
    Texture* newTextureCubemap(const bool flipped = false) const override;
    /// Create and return a new shader program (optionally, post load optimised).
    /// The callee is responsible for it's deletion!
    ShaderProgram* newShaderProgram(const bool optimise = false) const override;
    /// Create and return a new shader of the specified type by loading the
    /// specified name (optionally, post load optimised).
    /// The callee is responsible for it's deletion!
    Shader* newShader(const stringImpl& name, const ShaderType& type,
                      const bool optimise = false) const override;
    /// Enable or disable rasterization (useful for transform feedback)
    inline void toggleRasterization(bool state) override {
        state ? glDisable(GL_RASTERIZER_DISCARD) : glEnable(GL_RASTERIZER_DISCARD);
    }
    /// Modify the line width used by OpenGL when rendering lines. It's upper limit
    /// is capped to what the hardware supports
    inline void setLineWidth(GLfloat width) override {
        glLineWidth(std::min(width, (GLfloat)_lineWidthLimit));
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
    void drawText(const TextLabel& textLabel, const vec2<I32>& position) override;
    /// Rendering points is universally useful, so we have a function, and a VAO,
    /// dedicated to this process
    void drawPoints(GLuint numPoints) override;
    /// This functions should be run in a separate, consumer thread.
    /// The main app thread, the producer, adds tasks via a lock-free queue that is
    /// checked every 20 ms
    void threadedLoadCallback() override;
    /// Return the time it took to render a single frame (in nanoseconds). Only
    /// works in debug builds
    inline GLuint64 getFrameDurationGPU() override {
    #ifdef _DEBUG
        // The returned results are 4 frames old!
        glGetQueryObjectui64v(_queryID[_queryFrontBuffer][0], GL_QUERY_RESULT,
                              &FRAME_DURATION_GPU);
    #endif

        return FRAME_DURATION_GPU;
    }
    /// Return the OpenGL framebuffer handle bound and assigned for the specified
    /// usage
    inline static GLuint getActiveFB(Framebuffer::FramebufferUsage usage) {
        return _activeFBId[usage];
    }
    /// Change the clear color for the specified renderTarget
    static void clearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
    /// Change the clear color for the specified renderTarget
    inline static void clearColor(const vec4<GLfloat>& color) {
        clearColor(color.r, color.g, color.b, color.a);
    }
    /// Try to find the requested font in the font cache. Load on cache miss.
    I32 getFont(const stringImpl& fontName);
    /// changeResolution is simply asking GLFW to do the resizing and updating the
    /// resolution cache
    void changeResolution(GLushort w, GLushort h) override;
    /// Change the current viewport area. Redundancy check is performed in GFXDevice
    /// class
    void changeViewport(const vec4<GLint>& newViewport) const override;
    /// Reset as much of the GL default state as possible within the limitations
    /// given
    void clearStates(const bool skipShader, const bool skipTextures,
                     const bool skipBuffers);
    /// Return the glsl optimisation context (created by the glsl-optimizer library)
    inline glslopt_ctx* getGLSLOptContext() const { return _GLSLOptContex; }
    void uploadDrawCommands(
        const vectorImpl<IndirectDrawCommand>& drawCommands) const override;

  public:
    /// Enable or disable primitive restart and ensure that the correct index size
    /// is used
    static void togglePrimitiveRestart(bool state, bool smallIndices);
    /// Set the currently active texture unit
    static bool setActiveTextureUnit(GLuint unit);
    /// Switch the currently active vertex array object
    static bool setActiveVAO(GLuint id);
    /// Single place to change buffer objects for every target available
    static bool setActiveBuffer(GLenum target, GLuint id);
    /// Switch the current framebuffer by binding it as either a R/W buffer, read
    /// buffer or write buffer
    static bool setActiveFB(GLuint id, Framebuffer::FramebufferUsage usage);
    /// Bind the specified transform feedback object
    static bool setActiveTransformFeedback(GLuint id);
    /// Change the currently active shader program. Passing null will unbind shaders
    /// (will use program 0)
    static bool setActiveProgram(glShaderProgram* const program);
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
    /// Bind a texture specified by a GL handle and GL type to the specified unit
    /// using the sampler object defined by hash value
    static bool bindTexture(GLuint unit, GLuint handle, GLenum type,
                            size_t samplerHash = 0);
    /// Bind the sampler object described by the hash value to the specified unit
    static bool bindSampler(GLuint unit, size_t samplerHash);
    /// Bind the default texture handle (0) to the specified unit and set it to the
    /// specified type
    static bool unbindTexture(GLuint unit, GLenum type) {
        return bindTexture(unit, 0, type);
    }
    /// Return the OpenGL sampler object's handle for the given hash value
    static GLuint getSamplerHandle(size_t samplerHash);

  private:
    /// FontStash library initialization
    void createFonsContext();
    /// FontStash library deinitialization
    void deleteFonsContext();

  private:
    /// The previous Text3D node's font face size
    GLfloat _prevSizeNode;
    /// The previous plain text string's font face size
    GLfloat _prevSizeString;
    /// The previous Text3D node's line width
    GLfloat _prevWidthNode;
    /// The previous plain text string's line width
    GLfloat _prevWidthString;
    /// Current window resolution
    vec2<GLushort> _cachedResolution;
    /// Line width limit (hardware upper limit)
    GLint _lineWidthLimit;
    /// Used to render points (e.g. to render full screen quads with geometry
    /// shaders)
    GLuint _pointDummyVAO;
    /// Used to store all of the indirect draw commands
    static GLuint _indirectDrawBuffer;
    /// A cache of all fonts used
    typedef hashMapImpl<stringImpl, I32> FontCache;
    FontCache _fonts;
    /// Atomic boolean value used to signal the loading thread to stop
    std::atomic_bool _closeLoadingThread;
    /// Optimisation context for shaders (used for post-load optimisation)
    glslopt_ctx* _GLSLOptContex;
    /// Current active vertex array object's handle
    static GLuint _activeVAOId;
    /// 0 - current framebuffer, 1 - current read only framebuffer, 2 - current
    /// write only framebuffer
    static GLuint _activeFBId[3];
    /// VB, IB, SB, TB, UB, PUB, DIB
    static GLuint _activeBufferId[7];
    static GLuint _activeTextureUnit;
    static GLuint _activeTransformFeedback;
    static GLint _activePackUnpackAlignments[2];
    static GLint _activePackUnpackRowLength[2];
    static GLint _activePackUnpackSkipPixels[2];
    static GLint _activePackUnpackSkipRows[2];
    static vec4<GLfloat> _prevClearColor;
    /// Boolean value used to verify if we are using the U16 or U32 primitive
    /// restart index
    static bool _lastRestartIndexSmall;
    /// Boolean value used to verify if primitive restart index is enabled or
    /// disabled
    static bool _primitiveRestartEnabled;
    /// Toggle CEGUI rendering on/off (e.g. to check raw application rendering
    /// performance)
    bool _enableCEGUIRendering;
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
