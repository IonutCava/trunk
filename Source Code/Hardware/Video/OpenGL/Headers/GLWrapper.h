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
#include "Hardware/Video/OpenGL/Textures/Headers/glTexture.h"
#include "Hardware/Video/OpenGL/Headers/glEnumTable.h"
#include <boost/lockfree/spsc_queue.hpp>

class Text3D;
class glIMPrimitive;
class glUniformBuffer;

struct glslopt_ctx;

DEFINE_SINGLETON_EXT1(GL_API,RenderAPIWrapper)
    typedef Unordered_map<std::string, SceneGraphNode*> sceneGraphMap;
    typedef void (*callback)();	void glCommand(callback f){f();}

    friend class glShader;
    friend class glIMPrimitive;
    friend class glFrameBuffer;
    friend class glVertexArray;
    friend class glShaderProgram;
    friend class glSamplerObject;
    friend class glGenericVertexData;
protected:

    GL_API() : RenderAPIWrapper(),
               _prevWidthNode(0),
               _prevWidthString(0),
               _prevSizeNode(0),
               _prevSizeString(0),
               _lineWidthLimit(1),
               _pointDummyVAO(0)
    {
    }

    void exitRenderLoop(bool killCommand = false);

    GLbyte initHardware(const vec2<GLushort>& resolution, GLint argc, char **argv);
    void closeRenderingApi();
    void initDevice(GLuint targetFrameRate);
    ///Change the window's position
    void      setWindowPos(GLushort w, GLushort h) const;
    void      setMousePosition(GLushort x, GLushort y) const;

    void beginFrame();
    void endFrame();
    void idle();
    void flush();

    void getMatrix(const MATRIX_MODE& mode, mat4<GLfloat>& mat);

    FrameBuffer*        newFB(bool multisampled) const;
    VertexBuffer*       newVB(const PrimitiveType& type) const;
    PixelBuffer*        newPB(const PBType& type) const;
    GenericVertexData*  newGVD(const bool persistentMapped) const;
    ShaderBuffer*       newSB(const bool unbound = false) const;

    inline Texture*       newTextureArray(const bool flipped = false)   const {return New glTexture(glTextureTypeTable[TEXTURE_2D_ARRAY], flipped); }
    inline Texture*       newTexture2D(const bool flipped = false)      const {return New glTexture(glTextureTypeTable[TEXTURE_2D],flipped);}
    inline Texture*       newTextureCubemap(const bool flipped = false) const {return New glTexture(glTextureTypeTable[TEXTURE_CUBE_MAP],flipped);}
    inline ShaderProgram* newShaderProgram(const bool optimise = false) const {return New glShaderProgram(optimise); }
    inline Shader*        newShader(const std::string& name,const ShaderType& type, const bool optimise = false) const {return New glShader(name,type,optimise); }
           bool           initShaders();
           bool           deInitShaders();

    void lockMatrices(const MATRIX_MODE& setCurrentMatrix = VIEW_MATRIX, bool lockView = true, bool lockProjection = true);
    void releaseMatrices(const MATRIX_MODE& setCurrentMatrix = VIEW_MATRIX, bool releaseView = true, bool releaseProjection = true);

    GLfloat* lookAt(const mat4<GLfloat>& viewMatrix) const;
    //Setting ortho projection:
    GLfloat* GL_API::setProjection(const vec4<GLfloat>& rect, const vec2<GLfloat>& planes) const;
    //Setting perspective projection:
    GLfloat* GL_API::setProjection(GLfloat FoV, GLfloat aspectRatio, const vec2<GLfloat>& planes) const;
    //Setting anaglyph frustum for specified eye
    void setAnaglyphFrustum(GLfloat camIOD, const vec2<F32>& zPlanes, F32 aspectRatio, F32 verticalFoV, bool rightFrustum = false) const;

    void updateClipPlanes();

    inline void toggleRasterization(bool state) { state ? glDisable(GL_RASTERIZER_DISCARD) : glEnable(GL_RASTERIZER_DISCARD); }

    void debugDraw(const SceneRenderState& sceneRenderState);
    void drawText(const TextLabel& textLabel, const vec2<I32>& position);
    void drawBox3D(const vec3<GLfloat>& min,const vec3<GLfloat>& max, const mat4<GLfloat>& globalOffset);
    void drawLines(const vectorImpl<vec3<GLfloat> >& pointsA,
                   const vectorImpl<vec3<GLfloat> >& pointsB,
                   const vectorImpl<vec4<GLubyte> >& colors,
                   const mat4<GLfloat>& globalOffset,
                   const bool orthoMode = false,
                   const bool disableDepth = false);
    void drawPoints(GLuint numPoints);

    /*immediate mode emmlation*/
    IMPrimitive* createPrimitive(bool allowPrimitiveRecycle = true);
    /*immediate mode emmlation end*/

    void loadInContextInternal();

    inline GLuint64 getFrameDurationGPU() const { 
#ifdef _DEBUG
        glGetQueryObjectui64v(_queryID[_queryFrontBuffer][0], GL_QUERY_RESULT, &GL_API::FRAME_DURATION_GPU); 
#endif
        return GL_API::FRAME_DURATION_GPU; 
    }

    inline GLuint      getFrameCount()    const { return GL_API::FRAME_COUNT; }
    inline GLint       getDrawCallCount() const { return GL_API::FRAME_DRAW_CALLS_PREV; }
    inline static void registerDrawCall()       { GL_API::FRAME_DRAW_CALLS++; }

    inline static GLuint getActiveVAOId()              {return _activeVAOId;}

    static void clearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLuint renderTarget = 0, bool force = false);
    inline static void clearColor(const vec4<GLfloat>& color, GLuint renderTarget = 0, bool force = false) {
        clearColor(color.r,color.g,color.b,color.a,renderTarget,force);
    }
    inline static GLuint getActiveFB() { return _activeFBId; }


    inline static glslopt_ctx*     getGLSLOptContext()          {return _GLSLOptContex;}

    inline static GLuint getActiveTextureUnit() {return _activeTextureUnit;}

public:
    static void togglePrimitiveRestart(bool state, bool smallIndices);
    static bool setActiveTextureUnit(GLuint unit,const bool force = false);
    static bool setActiveVAO(GLuint id, const bool force = false);
    static bool setActiveBuffer(GLenum target, GLuint id, const bool force = false);
    static bool setActiveFB(GLuint id, const bool read = true, const bool write = true, const bool force = false);
    static bool setActiveTransformFeedback(GLuint id, const bool force = false);
    static bool setActiveProgram(glShaderProgram* const program,const bool force = false);
           void updateProjectionMatrix();
           void updateViewMatrix();
           void activateStateBlock(const RenderStateBlock& newBlock, RenderStateBlock* const oldBlock) const;

    static bool bindTexture(GLuint unit, GLuint handle, GLenum type, GLuint samplerID = 0);
    static bool bindSampler(GLuint unit, GLuint handle);
    inline static bool unbindTexture(GLuint unit, GLenum type){
        return bindTexture(unit, 0, type);
    }
protected:
    boost::atomic_bool _closeLoadingThread;

    I32 getFont(const std::string& fontName);
    glIMPrimitive* getOrCreateIMPrimitive(bool allowPrimitiveRecycle = true);
    ///Used for rendering skeletons
    void setupLineState(const mat4<F32>& mat, I64 drawStateHash,const bool ortho);
    void releaseLineState(const bool ortho);
    void drawDebugAxisInternal(const SceneRenderState& sceneRenderState);
    void setupLineStateViewPort(const mat4<F32>& mat);
    void releaseLineStateViewPort();
    void changeResolutionInternal(GLushort w, GLushort h);
    void changeViewport(const vec4<GLint>& newViewport) const;
    void clearStates(const bool skipShader, const bool skipTextures, const bool skipBuffers, const bool forceAll);

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
    vectorImpl<vec3<GLfloat> > _pointsA, _axisPointsA;
    vectorImpl<vec3<GLfloat> > _pointsB, _axisPointsB;
    vectorImpl<vec4<GLubyte> > _colors, _axisColors;
    //Immediate mode emulation
    ShaderProgram*               _imShader;       //<The shader used to render VB data
    vectorImpl<glIMPrimitive* >  _glimInterfaces; //<The interface that coverts IM calls to VB data
    GLuint _pointDummyVAO; //< Used to render points (e.g. to render full screen quads with geometry shaders)
    ///A cache of all fonts used 
    typedef Unordered_map<std::string , I32 > FontCache;
    ///2D GUI-like text (bitmap fonts) go in this
    FontCache  _fonts;

    mat4<F32> _ViewProjectionCacheMatrix;
    static glslopt_ctx* _GLSLOptContex;
    static GLuint _activeVAOId;
    static GLuint _activeFBId;
    static GLuint _activeBufferId[6]; //< VB, IB, SB, TB, UB, PUB
    static GLuint _activeTextureUnit;
    static GLuint _activeTransformFeedback;
    static Unordered_map<GLuint, vec4<GLfloat> > _prevClearColor;
    
    static bool _lastRestartIndexSmall;
    static bool _primitiveRestartEnabled;
    bool _activeClipPlanes[Config::MAX_CLIP_PLANES];

    /// performance counters
    // front x 2 and back x 2
    static const GLint PERFORMANCE_COUNTER_BUFFERS = 4;
    // number of queries
    static const GLint PERFORMANCE_COUNTERS = 1;
    // number of draw calls (rough estimate)
    static GLint FRAME_DRAW_CALLS;
    static GLint FRAME_DRAW_CALLS_PREV;
    static GLuint FRAME_COUNT;

    GLuint _queryID[PERFORMANCE_COUNTER_BUFFERS][PERFORMANCE_COUNTERS];
    GLuint _queryBackBuffer;
    GLuint _queryFrontBuffer;
    static GLuint64 FRAME_DURATION_GPU;

    struct FONScontext* _fonsContext;
    /*slot*/ /*<textureHandle , textureType>*/
    typedef Unordered_map<GLushort, std::pair<GLuint, GLenum> > textureBoundMapDef;
    static textureBoundMapDef textureBoundMap;

    typedef Unordered_map<GLushort, GLuint> samplerBoundMapDef;
    static samplerBoundMapDef samplerBoundMap;

END_SINGLETON

#endif
