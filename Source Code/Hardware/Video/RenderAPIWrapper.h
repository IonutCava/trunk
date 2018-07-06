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
///Simple frustum representation
struct frustum{

	F32 neard;
	F32 fard;
	F32 fov;
	F32 ratio;
	vec3 point[8];
};

class RenderState {

public:
	RenderState(bool enableCulling,
		        bool enableBlend,
				bool enableLighting,
				bool enableTextures) : _culling(enableCulling),
									   _blend(enableBlend),
									   _lighting(enableLighting),
		                               _textures(enableTextures),
									   _enabled(true){}
	RenderState() : _culling(true),
					_blend(false),
					_lighting(false),
		            _textures(true),
					_enabled(true){}

	bool& cullingEnabled()  {return _culling;  }
	bool& blendingEnabled() {return _blend;    }
	bool& lightingEnabled() {return _lighting; }
	bool& texturesEnabled() {return _textures; }
	bool& isEnabled()       {return _enabled;  }

private:
	bool _culling, _blend, _lighting, _textures,_enabled;
};

enum RenderAPI {

	OpenGL10,
	OpenGL12,
	OpenGL20,
	OpenGL21,
	OpenGL22,
	OpenGL30,
	OpenGL32,
	Direct3D8,
	Direct3D9,
	Direct3D10,
	GFX_PLACEHOLDER
};

enum Type {

	API_POINTS      = 0x0000,
	LINES           = 0x0001,
	LINE_LOOP       = 0x0002,
	LINE_STRIP      = 0x0003,
	TRIANGLES       = 0x0004,
	TRIANGLE_STRIP  = 0x0005,
	TRIANGLE_FAN    = 0x0006,
	QUADS           = 0x0007,
	QUAD_STRIP      = 0x0008,
	POLYGON         = 0x0009
	
	
};

enum Format{

	_U8              = 0x0000,
	_U16             = 0x0001,
	_U32             = 0x0002,
	_I8              = 0x0003,
	_I16             = 0x0004,
	_I32             = 0x0005
};

enum DetailLevel{

	HIGH = 2,
	MEDIUM = 1,
	LOW = 0
};

//Forward declarations
class Object3D;
class Text;
class Shader;
class ShaderProgram;
class Button;
class GuiFlash;
class SubMesh;
class FrameBufferObject;
class VertexBufferObject;
class PixelBufferObject;
class Texture;
typedef Texture Texture2D;
typedef Texture TextureCubemap;
class Box3D;
class Sphere3D;
class Quad3D;
class Text3D;
class mat4;
class Material;
class SceneGraph;
class SceneGraphNode;
class Transform;
enum LIGHT_TYPE;
enum SHADER_TYPE;
///Renderer Programming Interface
class RenderAPIWrapper
{

protected:
	RenderAPIWrapper() : _apiId(GFX_PLACEHOLDER){}

	friend class GFXDevice;
	
	void setId(RenderAPI api) {_apiId = api;}
	RenderAPI getId() { return _apiId;}
	virtual void lookAt(const vec3& eye,const vec3& center,const vec3& up = vec3(0,1,0), bool invertx = false, bool inverty = false) = 0;
	virtual void idle() = 0;
	virtual void getModelViewMatrix(mat4& mvMat) = 0;
	virtual void getProjectionMatrix(mat4& projMat) = 0;
	
	virtual void resizeWindow(U16 w, U16 h) = 0;

	virtual FrameBufferObject*  newFBO() = 0;
	virtual VertexBufferObject* newVBO() = 0;
	virtual PixelBufferObject*  newPBO() = 0;
	virtual Texture2D*          newTexture2D(bool flipped = false) = 0;
	virtual TextureCubemap*     newTextureCubemap(bool flipped = false) = 0;
	virtual ShaderProgram*      newShaderProgram() = 0;
	virtual Shader*             newShader(const std::string& name, SHADER_TYPE type) = 0;
	
	virtual void initHardware() = 0;
	virtual void closeRenderingApi() = 0;
	virtual void initDevice() = 0;

	/*Rendering States*/
	virtual void clearBuffers(U8 buffer_mask) = 0;
	virtual void swapBuffers() = 0;
	virtual void enableFog(F32 density, F32* color) = 0;
	/*Rendering States*/

	/*State Matrix Manipulation*/
	virtual void setOrthoProjection(const vec4& rect, const vec2& planes) = 0;
	virtual void setPerspectiveProjection(F32 FoV,F32 aspectRatio, const vec2& planes) = 0;
	virtual void lockProjection() = 0;
	virtual void releaseProjection() = 0;
	virtual void lockModelView() = 0;
	virtual void releaseModelView() = 0;
	/*State Matrix Manipulation*/

	virtual void toggle2D(bool _2D) = 0;

	/*GUI Rendering*/
	virtual void drawTextToScreen(Text*) = 0;
	virtual void drawCharacterToScreen(void* ,char) = 0;
	virtual void drawButton(Button*) = 0;
	virtual void drawFlash(GuiFlash* flash) = 0;
	/*GUI Rendering*/

	/*Object viewing*/
	virtual void renderInViewport(const vec4& rect, boost::function0<void> callback) = 0;
	/*Object viewing*/

	/*Primitives Rendering*/
	virtual void drawBox3D(vec3 min, vec3 max) = 0;
	/*Primitives Rendering*/

	/*Mesh Rendering*/
	virtual void renderModel(Object3D* const model) = 0;
	virtual void renderElements(Type t, Format f, U32 count, const void* first_element) = 0;
	/*Mesh Rendering*/

	/*Color Management*/
	virtual void setMaterial(Material* mat) = 0;
	/*Color Management*/

	/*Light Management*/
	virtual void setAmbientLight(const vec4& light) = 0;
	virtual void setLight(U8 slot, unordered_map<std::string,vec4>& properties_v,unordered_map<std::string,F32>& properties_f, LIGHT_TYPE type) = 0;
	/*Light Management*/

	virtual void toggleDepthMapRendering(bool state) = 0;
	virtual void Screenshot(char *filename, const vec4& rect) = 0;
	virtual void ignoreStateChanges(bool state) = 0;
	virtual void toggleWireframe(bool state) = 0;
	virtual ~RenderAPIWrapper(){};
	virtual void setObjectState(Transform* const transform) = 0;
	virtual void releaseObjectState(Transform* const transform) = 0;

	virtual F32 applyCropMatrix(frustum &f,SceneGraph* sceneGraph) = 0;
public: //RenderAPIWrapper global
	
	virtual void setRenderState(RenderState& state,bool force = false) = 0;
	RenderState& getActiveRenderState() {return _currentRenderState;}

private:
	RenderAPI _apiId;

protected:
	RenderState _defaultRenderState, _currentRenderState, _previousRenderState;
	bool _ignoreStates;
};

#endif