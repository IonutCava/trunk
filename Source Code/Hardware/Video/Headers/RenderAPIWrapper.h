/*“Copyright 2009-2012 DIVIDE-Studio”*/
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

#include "Core/Math/Headers/MathClasses.h"
#include "RenderAPIEnums.h"
#include <boost/function.hpp>
#include <vector>

///Simple frustum representation
struct frustum{

	F32 neard;
	F32 fard;
	F32 fov;
	F32 ratio;
	vec3<F32> point[8];
};

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
enum SHADER_TYPE;

///FWD DECLARE TYPEDEFS
typedef Texture Texture2D;
typedef Texture TextureCubemap;

///FWD DECLARE STRUCTS
struct RenderStateBlockDescriptor;

///Renderer Programming Interface
class RenderAPIWrapper {

protected:
	RenderAPIWrapper() : _apiId(GFX_RENDER_API_PLACEHOLDER),_apiVersionId(GFX_RENDER_API_VER_PLACEHOLDER) {}

	friend class GFXDevice;
	
	inline void setId(RENDER_API apiId)                        {_apiId = apiId;}
	inline void setVersionId(RENDER_API_VERSION apiVersionId)  {_apiVersionId = apiVersionId;}

	inline RENDER_API         getId()        { return _apiId;}
	inline RENDER_API_VERSION getVersionId() { return _apiVersionId;}

	virtual void lookAt(const vec3<F32>& eye,const vec3<F32>& center,const vec3<F32>& up = vec3<F32>(0,1,0), bool invertx = false, bool inverty = false) = 0;
	virtual void idle() = 0;
	virtual void getModelViewMatrix(mat4<F32>& mvMat) = 0;
	virtual void getProjectionMatrix(mat4<F32>& projMat) = 0;
	
	virtual void changeResolution(U16 w, U16 h) = 0;

	virtual FrameBufferObject*  newFBO() = 0;
	virtual VertexBufferObject* newVBO() = 0;
	virtual PixelBufferObject*  newPBO() = 0;
	virtual Texture2D*          newTexture2D(bool flipped = false) = 0;
	virtual TextureCubemap*     newTextureCubemap(bool flipped = false) = 0;
	virtual ShaderProgram*      newShaderProgram() = 0;
	virtual Shader*             newShader(const std::string& name, SHADER_TYPE type) = 0;
	
	virtual I8   initHardware(const vec2<U16>& resolution) = 0;
	virtual void exitRenderLoop(bool killCommand = false) = 0;
	virtual void closeRenderingApi() = 0;
	virtual void initDevice(U32 targetFPS) = 0;

	/*Rendering States*/
	virtual void clearBuffers(U8 buffer_mask) = 0;
	virtual void swapBuffers() = 0;
	virtual void enableFog(F32 density, F32* color) = 0;
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

	/*GUI Rendering*/
	virtual void drawTextToScreen(GUIElement* const) = 0;
	virtual void drawCharacterToScreen(void* ,char) = 0;
	virtual void drawButton(GUIElement* const) = 0;
	virtual void drawFlash(GUIElement* const) = 0;
	///console is singleton and viewport dimensions are updated by the API
	virtual void drawConsole( ) = 0;
	/*GUI Rendering*/

	/*Object viewing*/
	virtual void renderInViewport(const vec4<F32>& rect, boost::function0<void> callback) = 0;
	/*Object viewing*/

	/*Primitives Rendering*/
	virtual void drawBox3D(const vec3<F32>& min,const vec3<F32>& max, const mat4<F32>& globalOffset) = 0;
	virtual void drawLines(const std::vector<vec3<F32> >& pointsA,const std::vector<vec3<F32> >& pointsB,const std::vector<vec4<F32> >& colors, const mat4<F32>& globalOffset) = 0;
	/*Primitives Rendering*/

	/*Mesh Rendering*/
	virtual void renderModel(Object3D* const model) = 0;
	virtual void renderElements(PRIMITIVE_TYPE t, VERTEX_DATA_FORMAT f, U32 count, const void* first_element) = 0;
	/*Mesh Rendering*/

	/*Color Management*/
	virtual void setMaterial(Material* mat) = 0;
	/*Color Management*/

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

public: //RenderAPIWrapper global

	virtual void updateStateInternal(RenderStateBlock* block, bool force = false) = 0;
	virtual RenderStateBlock* newRenderStateBlock(const RenderStateBlockDescriptor& descriptor) = 0;
	
private:
	RENDER_API         _apiId;
	RENDER_API_VERSION _apiVersionId;
};

#endif