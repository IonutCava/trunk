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

#ifndef _RENDER_API_H
#define _RENDER_API_H

#include "RenderAPIEnums.h"
#include <boost/function.hpp>
#include "Utility/Headers/Vector.h"
#include "Core/Math/Headers/MathClasses.h"
#if defined( __WIN32__ ) || defined( _WIN32 )
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include "windows.h"
#  ifdef min
#    undef min
#  endif
#  ifdef max
#	 undef max
#  endif
//////////////////////////////////////////////////////////////////////
////////////////////////////////////Needed Linux Headers//////////////
#elif defined OIS_LINUX_PLATFORM
#  include <X11/Xlib.h>
//////////////////////////////////////////////////////////////////////
////////////////////////////////////Needed Mac Headers//////////////
#elif defined OIS_APPLE_PLATFORM
#  include <Carbon/Carbon.h>
#endif
///Simple frustum representation
struct frustum{

	F32 nearPlane;
	F32 farPlane;
	F32 fov;
	F32 ratio;
	vec3<F32> point[8];
};

typedef struct {
	// Video resolution
	I32 Width, Height;
	// Red bits per pixel
	I32 RedBits;
	// Green bits per pixel
	I32 GreenBits;
	// Blue bits per pixel
	I32 BlueBits;

} VideoModes;
//FWD DECLARE CLASSES

class Light;
class Shader;
class Kernel;
class SubMesh;
class Texture;
class Material;
class Object3D;
class Transform;
class SceneGraph;
class GUIElement;
class ShaderProgram;
class SceneGraphNode;
class RenderStateBlock;
class PixelBufferObject;
class FrameBufferObject;
class VertexBufferObject;

///FWD DECLARE ENUMS
enum ShaderType;

///FWD DECLARE TYPEDEFS
typedef Texture Texture2D;
typedef Texture TextureCubemap;

///FWD DECLARE STRUCTS
struct RenderStateBlockDescriptor;

///Renderer Programming Interface
class RenderAPIWrapper {
public: //RenderAPIWrapper global
	enum MATRIX_MODE{
		MODEL_VIEW_MATRIX = 0,
		PROJECTION_MATRIX = 1
	};
protected:
	RenderAPIWrapper() : _apiId(GFX_RENDER_API_PLACEHOLDER),
		                 _apiVersionId(GFX_RENDER_API_VER_PLACEHOLDER),
					     _GPUVendor(GPU_VENDOR_PLACEHOLDER)
    {
    }

    friend class GFXDevice;
	
	inline void setId(RenderAPI apiId)                       {_apiId = apiId;}
	inline void setVersionId(RenderAPIVersion apiVersionId)  {_apiVersionId = apiVersionId;}
	inline void setGPUVendor(GPUVendor gpuvendor)            {_GPUVendor = gpuvendor;}

	inline RenderAPI        getId()        { return _apiId;}
	inline RenderAPIVersion getVersionId() { return _apiVersionId;}
	inline GPUVendor        getGPUVendor() { return _GPUVendor;}

	virtual void lookAt(const vec3<F32>& eye,const vec3<F32>& center,const vec3<F32>& up = vec3<F32>(0,1,0), bool invertx = false, bool inverty = false) = 0;
	virtual void idle() = 0;
	virtual void getMatrix(MATRIX_MODE mode, mat4<F32>& mat) = 0;
	
	///Change the resolution and reshape all graphics data
	virtual void changeResolution(U16 w, U16 h) = 0;
	///Change the window size without reshaping window data
	virtual void setWindowSize(U16 w, U16 h) = 0;
	///Change the window's position
	virtual void setWindowPos(U16 w, U16 h) = 0;

	virtual FrameBufferObject*  newFBO(FBOType type = FBO_2D_COLOR) = 0;
	virtual VertexBufferObject* newVBO(PrimitiveType type = TRIANGLES) = 0;
	virtual PixelBufferObject*  newPBO(PBOType type = PBO_TEXTURE_2D) = 0;
	virtual Texture2D*          newTexture2D(bool flipped = false) = 0;
	virtual TextureCubemap*     newTextureCubemap(bool flipped = false) = 0;
	virtual ShaderProgram*      newShaderProgram() = 0;
	virtual Shader*             newShader(const std::string& name, ShaderType type) = 0;
	
	virtual I8   initHardware(const vec2<U16>& resolution, I32 argc, char **argv) = 0;
	virtual void exitRenderLoop(bool killCommand = false) = 0;
	virtual void closeRenderingApi() = 0;
	virtual void initDevice(U32 targetFrameRate) = 0;
	virtual void flush() = 0;
    virtual void clearStates(bool skipShader, bool skipTextures, bool skipBuffers) = 0;
	/*Rendering States*/
	virtual void clearBuffers(U16 buffer_mask) = 0;
	virtual void swapBuffers() = 0;
	virtual void enableFog(FogMode mode, F32 density, F32* color, F32 startDist, F32 endDist) = 0;
	/*Rendering States*/

	/*State Matrix Manipulation*/
	virtual void setOrthoProjection(const vec4<F32>& rect, const vec2<F32>& planes) = 0;
	virtual void setPerspectiveProjection(F32 FoV,F32 aspectRatio, const vec2<F32>& planes) = 0;
	virtual void lockProjection() = 0;
	virtual void releaseProjection() = 0;
	virtual void lockModelView() = 0;
	virtual void releaseModelView() = 0;
	/*State Matrix Manipulation*/

	virtual void toggle2D(bool _2D) = 0;

	virtual void drawTextToScreen(GUIElement* const) = 0;

	/*Object viewing*/
	virtual void renderInViewport(const vec4<F32>& rect, boost::function0<void> callback) = 0;
	/*Object viewing*/

	/*Primitives Rendering*/
	virtual void drawBox3D(const vec3<F32>& min,const vec3<F32>& max, const mat4<F32>& globalOffset) = 0;
	virtual void drawLines(const vectorImpl<vec3<F32> >& pointsA,const vectorImpl<vec3<F32> >& pointsB,const vectorImpl<vec4<F32> >& colors, const mat4<F32>& globalOffset) = 0;
	/*Primitives Rendering*/

	/*Mesh Rendering*/
	virtual void renderModel(Object3D* const model) = 0;
	virtual void renderModel(VertexBufferObject* const vbo, GFXDataFormat f, U32 count, const void* first_element) = 0;
	/*Mesh Rendering*/

	/*Light Management*/
	virtual void setAmbientLight(const vec4<F32>& light) = 0;
	virtual void setLight(Light* const light) = 0;
	/*Light Management*/

	virtual void toggleDepthMapRendering(bool state) = 0;
	virtual void Screenshot(char *filename, const vec4<F32>& rect) = 0;
	virtual ~RenderAPIWrapper(){};
	virtual void setObjectState(Transform* const transform, bool force = false, ShaderProgram* const shader = NULL) = 0;
	virtual void releaseObjectState(Transform* const transform, ShaderProgram* const shader = NULL) = 0;

	virtual F32 applyCropMatrix(frustum &f,SceneGraph* sceneGraph) = 0;

    virtual bool loadInContext(const CurrentContext& context, boost::function0<void> callback) = 0;

public: //RenderAPIWrapper global

#if defined( __WIN32__ ) || defined( _WIN32 )
	virtual HWND getHWND() {return _hwnd;}
#elif defined( __APPLE_CC__ ) // Apple OS X
	??
#else //Linux
	virtual Display* getDisplay() {return _dpy;}
	virtual GLXDrawable getDrawSurface() {return _drawable;}
#endif
	virtual void updateStateInternal(RenderStateBlock* block, bool force = false) = 0;
	virtual RenderStateBlock* newRenderStateBlock(const RenderStateBlockDescriptor& descriptor) = 0;
	
private:
	RenderAPI        _apiId;
	GPUVendor        _GPUVendor;
	RenderAPIVersion _apiVersionId;

protected:
#if defined( __WIN32__ ) || defined( _WIN32 )
	HWND _hwnd;
	HDC  _hdc ;
#elif defined( __APPLE_CC__ ) // Apple OS X
	??
#else //Linux
	Display* _dpy;
	GLXDrawable _drawable; 
#endif
};

#endif