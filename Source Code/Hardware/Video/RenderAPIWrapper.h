/*“Copyright 2009-2011 DIVIDE-Studio”*/
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

#include "Hardware/Platform/PlatformDefines.h"
#include "Utility/Headers/MathClasses.h"

class RenderState
{
public:
	RenderState(bool enableCulling,
		        bool enableBlend,
				bool enableLighting,
				bool enableTextures) : _culling(enableCulling),
									   _blend(enableBlend),
									   _lighting(enableLighting),
		                               _textures(enableTextures),
									   _enabled(true){}

	bool& cullingEnabled()  {return _culling;  }
	bool& blendingEnabled() {return _blend;    }
	bool& lightingEnabled() {return _lighting; }
	bool& texturesEnabled() {return _textures; }
	bool& isEnabled()       {return _enabled;  }

private:
	bool _culling, _blend, _lighting, _textures,_enabled;
};

enum RenderAPI
{
	OpenGL10,
	OpenGL12,
	OpenGL20,
	OpenGL21,
	OpenGL22,
	OpenGL30,
	OpenGL32,
	Direct3D8,
	Direct3D9,
	Direct3D10
};

enum Type
{
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

//Forward declarations
class Object3D;
class Text;
class Shader;
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
#include <iostream>
#include <unordered_map>

//Renderer Programming Interface
class RenderAPIWrapper
{

protected:
	RenderAPIWrapper() : _state(RenderState(true,true,true,true)) {}

	friend class GFXDevice;
	
	void setId(RenderAPI api) {_apiId = api;}
	RenderAPI getId() { return _apiId;}
	virtual void lookAt(const vec3& eye,const vec3& center,const vec3& up) = 0;
	virtual void idle() = 0;
	virtual mat4 getModelViewMatrix() = 0;
	virtual mat4 getProjectionMatrix() = 0;

	virtual void resizeWindow(U16 w, U16 h) = 0;

	virtual FrameBufferObject*  newFBO() = 0;
	virtual VertexBufferObject* newVBO() = 0;
	virtual PixelBufferObject*  newPBO() = 0;
	virtual Texture2D*          newTexture2D(bool flipped = false) = 0;
	virtual TextureCubemap*     newTextureCubemap(bool flipped = false) = 0;
	virtual Shader* newShader(const char *vsFile, const char *fsFile) = 0;
	virtual Shader* newShader() = 0;
	
	virtual void initHardware() = 0;
	virtual void closeRenderingApi() = 0;
	virtual void initDevice() = 0;

	/*Geometry transformations*/
	virtual void translate(const vec3& pos) = 0;
	virtual void rotate(F32 angle,const vec3& weights) = 0;
    virtual void scale(const vec3& scale) = 0;
	/*Geometry transformations*/

	/*Rendering States*/
	virtual void clearBuffers(U8 buffer_mask) = 0;
	virtual void swapBuffers() = 0;
	virtual void enableFog(F32 density, F32* color) = 0;
	/*Rendering States*/

	/*State Matrix Manipulation*/
	virtual void enable_MODELVIEW() = 0;
	virtual void loadIdentityMatrix() = 0;
	virtual void toggle2D(bool _2D) = 0;
	virtual void setTextureMatrix(U16 slot, const mat4& transformMatrix) = 0;
	virtual void restoreTextureMatrix(U16 slot) = 0;
	virtual void setOrthoProjection(const vec4& rect, const vec2& planes) = 0;
	/*State Matrix Manipulation*/

	/*GUI Rendering*/
	virtual void drawTextToScreen(Text*) = 0;
	virtual void drawCharacterToScreen(void* ,char) = 0;
	virtual void drawButton(Button*) = 0;
	virtual void drawFlash(GuiFlash* flash) = 0;
	/*GUI Rendering*/

	/*Primitives Rendering*/
	virtual void drawBox3D(vec3 min, vec3 max) = 0;
	virtual void drawBox3D(Box3D* const box) = 0;
	virtual void drawSphere3D(Sphere3D* const sphere) = 0;
	virtual void drawQuad3D(Quad3D* const quad) = 0;
	virtual void drawText3D(Text3D* const text) = 0;
	/*Primitives Rendering*/

	/*Mesh Rendering*/
	virtual void renderModel(Object3D* const model) = 0;
	virtual void renderElements(Type t, U32 count, const void* first_element) = 0;
	/*Mesh Rendering*/

	/*Color Management*/
	virtual void setMaterial(Material* mat) = 0;
	virtual void setColor(const vec4& v) = 0;
	virtual void setColor(const vec3& v) = 0;
	/*Color Management*/

	/*Light Management*/
	virtual void setLight(U8 slot, std::tr1::unordered_map<std::string,vec4>& properties) = 0;
	virtual void createLight(U8 slot) = 0;
	virtual void setLightCameraMatrices(const vec3& lightVector) = 0;
	virtual void restoreLightCameraMatrices() = 0;
	/*Light Management*/

	virtual void ignoreStateChanges(bool state) = 0;
	virtual void toggleWireframe(bool state) = 0;
	virtual ~RenderAPIWrapper(){};

public: //RenderAPIWrapper global
	
	//void setRenderState(RenderState& state){_state = state; }
	virtual void setRenderState(RenderState& state) = 0;
	RenderState& getActiveRenderState() {return _state;}

private:
	RenderAPI _apiId;

protected:
	RenderState _state;

};

#endif