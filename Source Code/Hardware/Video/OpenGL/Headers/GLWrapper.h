/*
   Copyright (c) 2013 DIVIDE-Studio
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
class FTFont;
class FTPoint;
class glIMPrimitive;
class glUniformBufferObject;
class glRenderStateBlock;

struct glslopt_ctx;

///Ugliest hack in the book, but it's needed for "deferred" immediate mode
typedef struct
{
  F32 mat[16];
} OffsetMatrix;

DEFINE_SINGLETON_EXT1(GL_API,RenderAPIWrapper)
	typedef Unordered_map<std::string, SceneGraphNode*> sceneGraphMap;
	typedef void (*callback)();	void glCommand(callback f){f();}

private:

	GL_API() : RenderAPIWrapper(),
			   _currentGLRenderStateBlock(NULL),
			   _state2DRendering(NULL),
               _defaultStateNoDepth(NULL),
			   _useMSAA(false),
               _2DRendering(false),
			   _msaaSamples(0),
			   _prevWidthNode(0),
			   _prevWidthString(0),
			   _prevSizeNode(0),
			   _prevSizeString(0)
	{
	}

	void exitRenderLoop(const bool killCommand = false);

	GLbyte initHardware(const vec2<GLushort>& resolution, I32 argc, char **argv);
	void closeRenderingApi();
	void initDevice(GLuint targetFrameRate);
	inline void changeResolution(GLushort w, GLushort h) {changeResolutionInternal(w,h);}
	///Change the window's position
	void setWindowPos(U16 w, U16 h) const;
	void setMousePosition(D32 x, D32 y) const;

	void lookAt(const vec3<F32>& eye, const vec3<F32>& target, const vec3<F32>& up);

	inline void lookAt(const mat4<F32>& viewMatrix, const vec3<F32>& viewDirection) {
		Divide::GL::_LookAt(viewMatrix.mat, viewDirection);
	}

	void beginFrame();
	void endFrame();
	void idle();
	void flush();
    void clearStates(const bool skipShader,const bool skipTextures,const bool skipBuffers, const bool forceAll);
    void getMatrix(const MATRIX_MODE& mode,     mat4<GLfloat>& mat);
    void getMatrix(const EXTENDED_MATRIX& mode, mat4<GLfloat>& mat);
    void getMatrix(const EXTENDED_MATRIX& mode, mat3<GLfloat>& mat);

	FrameBufferObject*  newFBO(const FBOType& type);
	VertexBufferObject* newVBO(const PrimitiveType& type);
	PixelBufferObject*  newPBO(const PBOType& type);

	inline Texture2D*          newTexture2D(const bool flipped = false)                   {return New glTexture(glTextureTypeTable[TEXTURE_2D],flipped);}
	inline TextureCubemap*     newTextureCubemap(const bool flipped = false)              {return New glTexture(glTextureTypeTable[TEXTURE_CUBE_MAP],flipped);}
	inline ShaderProgram*      newShaderProgram(const bool optimise = false)              {return New glShaderProgram(optimise); }

	inline Shader*             newShader(const std::string& name,const ShaderType& type, const bool optimise = false)  {return New glShader(name,type,optimise); }
           bool                initShaders();
           bool                deInitShaders();

	void lockMatrices(const MATRIX_MODE& setCurrentMatrix = VIEW_MATRIX, bool lockView = true, bool lockProjection = true);
	void releaseMatrices(const MATRIX_MODE& setCurrentMatrix = VIEW_MATRIX, bool releaseView = true, bool releaseProjection = true);

	void setOrthoProjection(const vec4<GLfloat>& rect, const vec2<GLfloat>& planes);
	void setPerspectiveProjection(GLfloat FoV,GLfloat aspectRatio, const vec2<GLfloat>& planes);
	void setAnaglyphFrustum(F32 camIOD, bool rightFrustum = false);
	void updateClipPlanes();

	void toggle2D(bool state);

	void debugDraw();
    void drawText(const std::string& text, const I32 width, const std::string& fontName, const F32 fontSize);
	void drawText(const std::string& text, const I32 width, const vec2<I32> position, const std::string& fontName, const F32 fontSize);
	void drawBox3D(const vec3<GLfloat>& min,const vec3<GLfloat>& max, const mat4<GLfloat>& globalOffset);
	void drawLines(const vectorImpl<vec3<GLfloat> >& pointsA,
				   const vectorImpl<vec3<GLfloat> >& pointsB,
				   const vectorImpl<vec4<GLubyte> >& colors,
				   const mat4<GLfloat>& globalOffset,
				   const bool orthoMode = false,
				   const bool disableDepth = false);

    /*immediate mode emmlation*/
    IMPrimitive* createPrimitive(bool allowPrimitiveRecycle = true);
    /*immediate mode emmlation end*/

	void renderInViewport(const vec4<GLuint>& rect, boost::function0<GLvoid> callback);

	void renderInstance(RenderInstance* const instance);
	void renderBuffer(VertexBufferObject* const vbo, Transform* const vboTransform = NULL);

	void setLight(Light* const light);

	void Screenshot(char *filename, const vec4<GLfloat>& rect);

	RenderStateBlock* newRenderStateBlock(const RenderStateBlockDescriptor& descriptor);
	void updateStateInternal(RenderStateBlock* block, bool force = false);

    bool loadInContext(const CurrentContext& context, boost::function0<GLvoid> callback);

protected:
	friend class glVertexArrayObject;
	inline static glShaderProgram* getActiveProgram()  {return _activeShaderProgram;}
    inline static GLuint getActiveVAOId()              {return _activeVAOId;}

protected:
    friend class glFrameBufferObject;
    friend class glDeferredBufferObject;
           static void restoreViewport();
		   static vec4<U32> setViewport(const vec4<U32>& viewport ,bool force = false);
		   static void clearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a, bool force = false);
		   inline static void clearColor(const vec4<F32>& color,bool force = false) {
			   clearColor(color.r,color.g,color.b,color.a,force);
		   }
protected:
    friend class glShader;
    friend class glShaderProgram;
    inline static glslopt_ctx* getGLSLOptContext()                    {return _GLSLOptContex;}
	inline        glUniformBufferObject* getUBO(const UBO_NAME& name) {return _uniformBufferObjects[name]; }

protected:
	friend class glSamplerObject;
	inline static GLuint getActiveTextureUnit() {return _activeTextureUnit;}

public:

    static void setActiveTextureUnit(GLuint unit,const bool force = false);
    static void setActiveVAO(GLuint id,const bool force = false);
	static void setActiveProgram(glShaderProgram* const program,const bool force = false);
		   void updateProjectionMatrix();
		   void updateViewMatrix();
private:
	GLFWvidmode _modes[20];
	void loadInContextInternal();
	boost::lockfree::spsc_queue<boost::function0<GLvoid>, boost::lockfree::capacity<10> > _loadQueue;
	boost::thread *_loaderThread;
	boost::atomic_bool _closeLoadingThread;

    FTFont* getFont(const std::string& fontName);
	glIMPrimitive* getOrCreateIMPrimitive(bool allowPrimitiveRecycle = true);
	///Used for rendering skeletons
	void setupLineState(const OffsetMatrix& mat, RenderStateBlock* const drawState,const bool ortho);
	void releaseLineState(const bool ortho);
    void drawDebugAxisInternal();
	void setupLineStateViewPort(const OffsetMatrix& mat);
	void releaseLineStateViewPort();
	void changeResolutionInternal(U16 w, U16 h);

private: //OpenGL specific:

	///Text Rendering
	///The previous plain text string's relative position on screen
	FTPoint* _prevPointString;
	///The previous Text3D node's font face size
	F32 _prevSizeNode;
	///The previous plain text string's font face size
	F32 _prevSizeString;
	///The previous Text3D node's line width
	F32 _prevWidthNode;
	///The previous plain text string's line width
	F32 _prevWidthString;
	///Window management:
	vec2<GLushort> _cachedResolution; ///<Current window resolution
	//Render state specific:
	glRenderStateBlock*   _currentGLRenderStateBlock; //<Currently active rendering states used by OpenGL
	RenderStateBlock*     _state2DRendering;    //<Special render state for 2D rendering
	RenderStateBlock*     _defaultStateNoDepth; //<The default render state buth with GL_DEPTH_TEST false
    bool                  _2DRendering;

    vectorImpl<vec3<GLfloat> > _pointsA;
    vectorImpl<vec3<GLfloat> > _pointsB;
    vectorImpl<vec4<GLubyte> > _colors;
	vectorImpl<glUniformBufferObject* > _uniformBufferObjects;
	//Immediate mode emulation
	ShaderProgram*               _imShader;       //<The shader used to render VBO data
	vectorImpl<glIMPrimitive* >  _glimInterfaces; //<The interface that coverts IM calls to VBO data

	///A cache of all fonts used (use a separate map for 3D text
    typedef Unordered_map<std::string , FTFont* > FontCache;
	///2D GUI-like text (bitmap fonts) go in this
	FontCache  _fonts;
	GLuint     _msaaSamples;
	bool       _useMSAA;///<set to falls for FXAA or SMAA

    static glslopt_ctx* _GLSLOptContex;
    static glShaderProgram* _activeShaderProgram;
    static GLuint _activeVAOId;
    static GLuint _activeTextureUnit;
	static vec4<GLfloat> _prevClearColor;

	bool _activeClipPlanes[Config::MAX_CLIP_PLANES];

END_SINGLETON

#endif
