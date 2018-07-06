/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _WRAPPER_GL_H_
#define _WRAPPER_GL_H_

#include "core.h"
#include "glResources.h"
#include "Hardware/Video/Headers/RenderAPIWrapper.h"
#include "Hardware/Video/Headers/ImmediateModeEmulation.h"
#include "Hardware/Video/OpenGL/Shaders/Headers/glShaderProgram.h"
#include "Hardware/Video/OpenGL/Shaders/Headers/glShader.h"
#include "Hardware/Video/OpenGL/Textures/Headers/glSamplerObject.h"
#include "Hardware/Video/OpenGL/Textures/Headers/glTexture.h"
#include "Hardware/Video/OpenGL/Headers/glEnumTable.h"
#include <boost/lockfree/spsc_queue.hpp>

class Text3D;
class glIMPrimitive;
class glUniformBuffer;

struct glslopt_ctx;

DEFINE_SINGLETON_EXT1(GL_API, RenderAPIWrapper)

    typedef void (*callback)();	void glCommand(callback f){f();}

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

    I8   initRenderingApi(const vec2<GLushort>& resolution, GLint argc, char **argv);
    void closeRenderingApi();
    void initDevice(GLuint targetFrameRate);   bool initShaders();
    bool deInitShaders();

    ///Change the window's position
    void setWindowPos(GLushort w, GLushort h) const;
    void setMousePosition(GLushort x, GLushort y) const;

    void beginFrame();
    void endFrame();

    IMPrimitive*        newIMP() const;
    Framebuffer*        newFB(bool multisampled) const;
    VertexBuffer*       newVB() const;
    PixelBuffer*        newPB(const PBType& type) const;
    GenericVertexData*  newGVD(const bool persistentMapped) const;
    ShaderBuffer*       newSB(const bool unbound = false, const bool persistentMapped = true) const;

    inline Texture*       newTextureArray(const bool flipped = false)   const {return New glTexture(TEXTURE_2D_ARRAY, flipped); }
    inline Texture*       newTexture2D(const bool flipped = false)      const {return New glTexture(TEXTURE_2D, flipped);}
    inline Texture*       newTextureCubemap(const bool flipped = false) const {return New glTexture(TEXTURE_CUBE_MAP, flipped);}
    inline ShaderProgram* newShaderProgram(const bool optimise = false) const {return New glShaderProgram(optimise); }
    inline Shader*        newShader(const std::string& name,const ShaderType& type, const bool optimise = false) const {return New glShader(name,type,optimise); }

    inline void toggleRasterization(bool state) { state ? glDisable(GL_RASTERIZER_DISCARD) : glEnable(GL_RASTERIZER_DISCARD); }
    inline void setLineWidth(GLfloat width) { glLineWidth(std::min(width, (GLfloat)_lineWidthLimit)); }

    static size_t getOrCreateSamplerObject(const SamplerDescriptor& descriptor);

    void updateClipPlanes();
    void drawText(const TextLabel& textLabel, const vec2<I32>& position);
    void drawPoints(GLuint numPoints);

    void createLoaderThread();

    inline GLuint64 getFrameDurationGPU() const { 
#ifdef _DEBUG
        glGetQueryObjectui64v(_queryID[_queryFrontBuffer][0], GL_QUERY_RESULT, &GL_API::FRAME_DURATION_GPU); 
#endif
        return GL_API::FRAME_DURATION_GPU; 
    }

    inline static GLuint       getActiveFB()          { return _activeFBId; }
    inline static glslopt_ctx* getGLSLOptContext()    { return _GLSLOptContex; }

    static void clearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLuint renderTarget = 0);
    inline static void clearColor(const vec4<GLfloat>& color, GLuint renderTarget = 0) {
        clearColor(color.r,color.g,color.b,color.a,renderTarget);
    }

    I32 getFont(const std::string& fontName);

    void changeResolutionInternal(GLushort w, GLushort h);
    void changeViewport(const vec4<GLint>& newViewport) const;
    void clearStates(const bool skipShader, const bool skipTextures, const bool skipBuffers);

public:
    static void togglePrimitiveRestart(bool state, bool smallIndices);
    static bool setActiveTextureUnit(GLuint unit);
    static bool setActiveVAO(GLuint id);
    static bool setActiveBuffer(GLenum target, GLuint id);
    static bool setActiveFB(GLuint id, const bool read = true, const bool write = true);
    static bool setActiveTransformFeedback(GLuint id);
    static bool setActiveProgram(glShaderProgram* const program);
           void activateStateBlock(const RenderStateBlock& newBlock, RenderStateBlock* const oldBlock) const;


    static bool setPixelPackUnpackAlignment(GLint packAlignment = 1, GLint unpackAlignment = 1) {
        return (setPixelPackAlignment(packAlignment) && setPixelUnpackAlignment(unpackAlignment));
    }
    static bool setPixelPackAlignment(GLint packAlignment = 1, GLint rowLength = 0, GLint skipRows = 0, GLint skipPixels = 0);
    static bool setPixelUnpackAlignment(GLint unpackAlignment = 1, GLint rowLength = 0, GLint skipRows = 0, GLint skipPixels = 0);

    static bool bindTexture(GLuint unit, GLuint handle, GLenum type, size_t samplerHash = 0);
    static bool bindSampler(GLuint unit, size_t samplerHash);
    static bool unbindTexture(GLuint unit, GLenum type) { return bindTexture(unit, 0, type); }

    static GLuint getSamplerHandle(size_t samplerHash);

private:
    /// createFonsContext only exists so that we do not include the fontstash headers in GLFWWrapper as well
    void createFonsContext();
    /// deleteFonsContext only exists so that we do not include the fontstash headers in GLFWWrapper as well
    void deleteFonsContext();
    
private: //OpenGL specific:

    ///Text Rendering
    ///The previous Text3D node's font face size
    GLfloat _prevSizeNode;
    ///The previous plain text string's font face size
    GLfloat _prevSizeString;
    ///The previous Text3D node's line width
    GLfloat _prevWidthNode;
    ///The previous plain text string's line width
    GLfloat _prevWidthString;
    ///Window management:
    vec2<GLushort> _cachedResolution; ///<Current window resolution
    // line width limit
    GLint _lineWidthLimit;
    GLuint _pointDummyVAO; //< Used to render points (e.g. to render full screen quads with geometry shaders)
    ///A cache of all fonts used 
    typedef Unordered_map<std::string , I32 > FontCache;
    ///2D GUI-like text (bitmap fonts) go in this
    FontCache  _fonts;

    boost::atomic_bool _closeLoadingThread;

    static glslopt_ctx* _GLSLOptContex;
    static GLuint _activeVAOId;
    static GLuint _activeFBId;
    static GLuint _activeBufferId[7]; //< VB, IB, SB, TB, UB, PUB, DIB
    static GLuint _activeTextureUnit;
    static GLuint _activeTransformFeedback;
    static GLint _activePackUnpackAlignments[2];
    static GLint _activePackUnpackRowLength[2];
    static GLint _activePackUnpackSkipPixels[2];
    static GLint _activePackUnpackSkipRows[2];
    static Unordered_map<GLuint, vec4<GLfloat> > _prevClearColor;
    
    static bool _lastRestartIndexSmall;
    static bool _primitiveRestartEnabled;
    bool _activeClipPlanes[Config::MAX_CLIP_PLANES];

    /// performance counters
    // front x 2 and back x 2
    static const GLint PERFORMANCE_COUNTER_BUFFERS = 4;
    // number of queries
    static const GLint PERFORMANCE_COUNTERS = 1;
  
    GLuint _queryID[PERFORMANCE_COUNTER_BUFFERS][PERFORMANCE_COUNTERS];
    GLuint _queryBackBuffer;
    GLuint _queryFrontBuffer;
    static GLuint64 FRAME_DURATION_GPU;

    struct FONScontext* _fonsContext;
    /*slot*/ /*<textureHandle , textureType>*/
    typedef Unordered_map<GLushort, std::pair<GLuint, GLenum> > textureBoundMapDef;
    static textureBoundMapDef _textureBoundMap;

    typedef Unordered_map<GLushort, size_t> samplerBoundMapDef;
    static samplerBoundMapDef _samplerBoundMap;

    typedef Unordered_map<size_t, glSamplerObject* > samplerObjectMap;
    static samplerObjectMap _samplerMap;

END_SINGLETON

#endif
