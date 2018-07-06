/*�Copyright 2009-2013 DIVIDE-Studio�*/
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

#ifndef _RENDER_API_H_
#define _RENDER_API_H_

#include "RenderAPIEnums.h"
#include "RenderInstance.h"
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
class IMPrimitive;
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
class RenderStateBlockDescriptor;

///Renderer Programming Interface
class RenderAPIWrapper {
public: //RenderAPIWrapper global

protected:
	RenderAPIWrapper() : _apiId(GFX_RENDER_API_PLACEHOLDER),
		                 _apiVersionId(GFX_RENDER_API_VER_PLACEHOLDER),
					     _GPUVendor(GPU_VENDOR_PLACEHOLDER)
    {
    }

    friend class GFXDevice;

	inline void setId(const RenderAPI& apiId)                       {_apiId = apiId;}
	inline void setVersionId(const RenderAPIVersion& apiVersionId)  {_apiVersionId = apiVersionId;}
	inline void setGPUVendor(const GPUVendor& gpuvendor)            {_GPUVendor = gpuvendor;}

	inline const RenderAPI&        getId()        const { return _apiId;}
	inline const RenderAPIVersion& getVersionId() const { return _apiVersionId;}
	inline const GPUVendor&        getGPUVendor() const { return _GPUVendor;}

	/*Application display frame*/
	///Clear buffers, set default states, etc
	virtual void beginFrame() = 0;
	///Clear shaders, restore active texture units, etc
	virtual void endFrame() = 0;
	///Clear buffers,shaders, etc.
	virtual void flush() = 0;

	virtual void lookAt(const vec3<F32>& eye,
                        const vec3<F32>& center,
                        const vec3<F32>& up = vec3<F32>(0,1,0),
                        const bool invertx = false,
                        const bool inverty = false) = 0;

	virtual void idle() = 0;
	virtual void getMatrix(const MATRIX_MODE& mode, mat4<F32>& mat) = 0;
    virtual void getMatrix(const EXTENDED_MATRIX& mode, mat4<F32>& mat) = 0;
    virtual void getMatrix(const EXTENDED_MATRIX& mode, mat3<F32>& mat) = 0;

	///Change the resolution and reshape all graphics data
	virtual void changeResolution(U16 w, U16 h) = 0;
	///Change the window's position
	virtual void setWindowPos(U16 w, U16 h) = 0;

	virtual FrameBufferObject*  newFBO(const FBOType& type = FBO_2D_COLOR) = 0;
	virtual VertexBufferObject* newVBO(const PrimitiveType& type = TRIANGLES) = 0;
	virtual PixelBufferObject*  newPBO(const PBOType& type = PBO_TEXTURE_2D) = 0;
	virtual Texture2D*          newTexture2D(const bool flipped = false) = 0;
	virtual TextureCubemap*     newTextureCubemap(const bool flipped = false) = 0;
	virtual ShaderProgram*      newShaderProgram(const bool optimise = false) = 0;
	virtual Shader*             newShader(const std::string& name,const ShaderType& type,const bool optimise = false) = 0;
	virtual bool                initShaders() = 0;
    virtual bool                deInitShaders() = 0;

	virtual I8   initHardware(const vec2<U16>& resolution, I32 argc, char **argv) = 0;
	virtual void exitRenderLoop(const bool killCommand = false) = 0;
	virtual void closeRenderingApi() = 0;
	virtual void initDevice(U32 targetFrameRate) = 0;

	/*State Matrix Manipulation*/
	virtual void setOrthoProjection(const vec4<F32>& rect, const vec2<F32>& planes) = 0;
	virtual void setPerspectiveProjection(F32 FoV,F32 aspectRatio, const vec2<F32>& planes) = 0;
    //push-pop our rendering matrices and set the current matrix to whatever "setCurrentMatrix" is
    virtual void lockMatrices(const MATRIX_MODE& setCurrentMatrix, bool lockView , bool lockProjection) = 0;
	virtual void releaseMatrices(const MATRIX_MODE& setCurrentMatrix, bool releaseView, bool releaseProjection) = 0;
	/*State Matrix Manipulation*/

	virtual void toggle2D(bool _2D) = 0;
    virtual void drawText(const std::string& text, const I32 width, const std::string& fontName, const F32 fontSize) = 0;
	virtual void drawText(const std::string& text,  const I32 width, const vec2<I32> position, const std::string& fontName, const F32 fontSize) = 0;

	/*Object viewing*/
	virtual void renderInViewport(const vec4<I32>& rect, boost::function0<void> callback) = 0;
	/*Object viewing*/

	/*Primitives Rendering*/
	virtual void drawBox3D(const vec3<F32>& min,const vec3<F32>& max, const mat4<F32>& globalOffset) = 0;
	virtual void drawLines(const vectorImpl<vec3<F32> >& pointsA,
						   const vectorImpl<vec3<F32> >& pointsB,
						   const vectorImpl<vec4<U8> >& colors,
						   const mat4<F32>& globalOffset,
						   const bool orthoMode = false,
						   const bool disableDepth = false) = 0;
	///Render bounding boxes, skeletons, axis etc.
	virtual void debugDraw() = 0;
	/*Primitives Rendering*/
    /*Immediate Mode Emmlation*/
    virtual IMPrimitive* createPrimitive() = 0;
    /*Immediate Mode Emmlation*/

	/*Mesh Rendering*/
	///Render a specific object with a specific transform and/or transforms for instanced meshes
	virtual void renderInstance(RenderInstance* const instance) = 0;
	///Render a single vbo with the specified transformation (THIS DOES NOT CALL ENABLE/DISABLE for you! - note: OGL does call Enable if needed to avoid crashes)
	virtual void renderBuffer(VertexBufferObject* const vbo, Transform* const vboTransform = NULL) = 0;
	/*Mesh Rendering*/

	/*Light Management*/
	virtual void setLight(Light* const light) = 0;
	/*Light Management*/
	virtual void Screenshot(char *filename, const vec4<F32>& rect) = 0;
	virtual ~RenderAPIWrapper(){};
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
