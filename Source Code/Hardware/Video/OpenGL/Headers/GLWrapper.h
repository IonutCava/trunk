/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _WRAPPER_GL_H_
#define _WRAPPER_GL_H_

#include "core.h"
#include "glResources.h"
#include "Hardware/Video/Headers/RenderAPIWrapper.h"

#include "Hardware/Video/OpenGL/Shaders/Headers/glShaderProgram.h"
#include "Hardware/Video/OpenGL/Shaders/Headers/glShader.h"
#include "Hardware/Video/OpenGL/Textures/Headers/glTexture.h"
#include "Hardware/Video/OpenGL/Headers/glEnumTable.h"
#include <stack>

class FTFont;
class Text3D;
namespace NS_GLIM {
	class GLIM_BATCH;
}
class glRenderStateBlock;

///Ugliest hack in the book, but it's needed for "deferred" immediate mode
typedef struct
{
  F32 mat[16];
} OffsetMatrix;


///IMPrimitive replaces immediate mode calls to VBO based rendering
struct IMPrimitive  : private boost::noncopyable{
	IMPrimitive();
	~IMPrimitive();
	NS_GLIM::GLIM_BATCH* _imInterface;
	Texture2D*           _texture;
	bool                 _hasLines;
	F32                  _lineWidth;
	///After rendering the primitive, we se "inUse" to false but leave it in the vector
	///If a new primitive is to be rendered, it first looks for a zombie primitive
	///If none are found, it adds a new one
	bool                 _inUse; //<For caching
	///after rendering an IMPrimitive, it's "inUse" flag is set to false.
	///If OpenGL tries to render it again, we just increment the _zombieCounter
	///If the _zombieCounter reaches 3, we remove it from the vector as it is not needed
	U8                   _zombieCounter;
	///2 functions used to setup or reset states
	boost::function0<void> _setupStates;
	boost::function0<void> _resetStates;
};

class glUniformBufferObject;
DEFINE_SINGLETON_EXT1(GL_API,RenderAPIWrapper)
	typedef Unordered_map<std::string, SceneGraphNode*> sceneGraphMap;
	typedef void (*callback)();	void glCommand(callback f){f();}

private:

	GL_API() : RenderAPIWrapper(),
			   _currentGLRenderStateBlock(NULL),
			   _state2DRendering(NULL),
			   _previousStateBlock(NULL),
			   _depthMapRendering(false),
			   _useMSAA(false),
               _2DRendering(false),
			   _msaaSamples(0)
	{
	}

	void exitRenderLoop(bool killCommand = false) { GL_API::_applicationClosing = killCommand;}

	GLbyte initHardware(const vec2<GLushort>& resolution, I32 argc, char **argv);
	void closeRenderingApi();
	void initDevice(GLuint targetFrameRate);
	void changeResolution(GLushort w, GLushort h);
	///Change the window size without reshaping window data
	void setWindowSize(U16 w, U16 h);
	///Change the window's position
	void setWindowPos(U16 w, U16 h);
	void lookAt(const vec3<GLfloat>& eye,const vec3<GLfloat>& center,const vec3<GLfloat>& up = vec3<GLfloat>(0,1,0), bool invertx = false, bool inverty = false);
	void idle();
	void flush();
    void clearStates(bool skipShader, bool skipTextures, bool skipBuffers);
    void getMatrix(MATRIX_MODE mode, mat4<GLfloat>& mat);

	FrameBufferObject*  newFBO(FBOType type);
	VertexBufferObject* newVBO(PrimitiveType type);
	PixelBufferObject*  newPBO(PBOType type);

	inline Texture2D*          newTexture2D(bool flipped = false)                   {return New glTexture(glTextureTypeTable[TEXTURE_2D],flipped);}
	inline TextureCubemap*     newTextureCubemap(bool flipped = false)              {return New glTexture(glTextureTypeTable[TEXTURE_CUBE_MAP],flipped);}
	inline ShaderProgram*      newShaderProgram()                                   {return New glShaderProgram(); }
	inline Shader*             newShader(const std::string& name, ShaderType type)  {return New glShader(name,type); }

	void clearBuffers(GLushort buffer_mask);
	void swapBuffers();
	void enableFog(FogMode mode, GLfloat density, GLfloat* color, GLfloat startDist, GLfloat endDist);

    void lockProjection();
	void releaseProjection();
	void lockModelView();
	void releaseModelView();

	void setOrthoProjection(const vec4<GLfloat>& rect, const vec2<GLfloat>& planes);
	void setPerspectiveProjection(GLfloat FoV,GLfloat aspectRatio, const vec2<GLfloat>& planes);

	void toggle2D(bool state);

	void drawTextToScreen(GUIElement* const);
	void render3DText(Text3D* const text);
	void drawBox3D(const vec3<GLfloat>& min,const vec3<GLfloat>& max, const mat4<GLfloat>& globalOffset);
	void drawLines(const vectorImpl<vec3<GLfloat> >& pointsA,const vectorImpl<vec3<GLfloat> >& pointsB,const vectorImpl<vec4<GLfloat> >& colors, const mat4<GLfloat>& globalOffset);

	void renderInViewport(const vec4<GLfloat>& rect, boost::function0<GLvoid> callback);

	void renderModel(Object3D* const model);
	void renderModel(VertexBufferObject* const vbo, GFXDataFormat f, GLuint count, const void* first_element);
	
	void setAmbientLight(const vec4<GLfloat>& light);
	void setLight(Light* const light);

	void Screenshot(char *filename, const vec4<GLfloat>& rect);

	RenderStateBlock* newRenderStateBlock(const RenderStateBlockDescriptor& descriptor);
	void updateStateInternal(RenderStateBlock* block, bool force = false);

	void toggleDepthMapRendering(bool state);

	void setObjectState(Transform* const transform, bool force = false, ShaderProgram* const shader = NULL);
	void releaseObjectState(Transform* const transform, ShaderProgram* const ShaderProgram = NULL);

	GLfloat applyCropMatrix(frustum &f,SceneGraph* sceneGraph);
    bool loadInContext(const CurrentContext& context, boost::function0<GLvoid> callback);

    inline static void setActiveVBOIdInternal(GLuint id)               {_activeVBOId = id;}
    inline static void setActiveVAOIdInternal(GLuint id)               {_activeVAOId = id;}
    inline static void setActiveShaderId(GLuint id)                    {_activeShaderId = id;}
    inline static void setActiveTextureUnitInternal(GLuint unit)       {_activeTextureUnit = unit;}
    inline static void setClientActiveTextureUnitInternal(GLuint unit) {_activeClientTextureUnit = unit;}

    inline static GLuint getActiveClientTextureUnit()                    {return _activeClientTextureUnit;}
    inline static GLuint getActiveTextureUnit()                          {return _activeTextureUnit;}
    inline static GLuint getActiveVBOId()                                {return _activeVBOId;}
    inline static GLuint getActiveVAOId()                                {return _activeVAOId;}

protected:
    friend class glShaderProgram;
    inline static GLuint getActiveShaderId()                             {return _activeShaderId;}

public:
    static void setActiveTextureUnit(GLuint unit);
    static void setClientActiveTextureUnit(GLuint unit);
    static void setActiveVBO(GLuint id);
    static void setActiveIBO(GLuint id);
    static void setActiveVAO(GLuint id);

public:
	static bool _applicationClosing;
	static bool _contextAvailable;
	static bool _useDebugOutputCallback;

private:
	GLFWvidmode _modes[20];
#if defined( __WIN32__ ) || defined( _WIN32 )
	HGLRC _loaderContext;
	void loadInContextInternal(const CurrentContext& context, boost::function0<GLvoid> callback);
#elif defined( __APPLE_CC__ ) // Apple OS X
///??
#else //Linux
	GLXContext _loaderContext;
	void loadInContextInternal(const CurrentContext& context, boost::function0<GLvoid> callback);
#endif
	IMPrimitive* getOrCreateIMPrimitive();
	///Used for rendering skeletons
	void setupSkeletonState(OffsetMatrix mat);
	void releaseSkeletonState();

private: //OpenGL specific:
	typedef std::stack<mat4<GLfloat>, vectorImpl<mat4<GLfloat> > > matrixStack;
	///Matrix management
	matrixStack _modelViewMatrix;
	matrixStack _projectionMatrix;
	//Window management:
	vec2<GLushort> _cachedResolution; ///<Current window resolution

	//Render state specific:
	glRenderStateBlock*   _currentGLRenderStateBlock; ///<Currently active rendering states used by OpenGL
	RenderStateBlock*     _state2DRendering;  ///<Special render state for 2D rendering
	RenderStateBlock*     _previousStateBlock; ///<The active state block before any 2D rendering
	bool                  _depthMapRendering; ///<Tell the OpenGL to use flat shading
    bool                  _2DRendering;
	glUniformBufferObject*  _lightUBO;
	glUniformBufferObject*  _materialsUBO;
	glUniformBufferObject*  _transformsUBO;
	//Immediate mode emulation
	ShaderProgram*             _imShader;      ///<The shader used to render VBO data
	vectorImpl<IMPrimitive* >  _glimInterfaces; ///<The interface that coverts IM calls to VBO data
	
	
	///A cache of all fonts used (use a separate map for 3D text
	typedef Unordered_map< const char* , FTFont* > FontCache;
	///2D GUI-like text (bitmap fonts) go in this
	FontCache  _fonts;
	///3D polygon fonts go in this container
	FontCache  _3DFonts;
	GLuint     _msaaSamples;
	bool       _useMSAA;///<set to falls for FXAA or SMAA

    static GLuint _activeShaderId ;
    static GLuint _activeVBOId;
    static GLuint _activeVAOId;
    static GLuint _activeTextureUnit;
    static GLuint _activeClientTextureUnit;
END_SINGLETON

#endif
