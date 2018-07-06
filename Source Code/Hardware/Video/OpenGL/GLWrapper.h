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

#ifndef _WRAPPER_GL_H_
#define _WRAPPER_GL_H_

#include "../RenderAPIWrapper.h"
#include "core.h"
#include "glFrameBufferObject.h"
#include "glVertexBufferObject.h"
#include "glPixelBufferObject.h"
#include "glShaderProgram.h"
#include "glShader.h"
#include "glTexture.h"
#include "glEnumTable.h"

class glRenderStateBlock;
void GLCheckError(const std::string& File, unsigned int Line, char* operation);

DEFINE_SINGLETON_EXT1(GL_API,RenderAPIWrapper)
	typedef unordered_map<std::string, SceneGraphNode*> sceneGraphMap;
	typedef void (*callback)();	void glCommand(callback f){f();}

private:

	GL_API() : RenderAPIWrapper(), _windowId(0), _currentGLRenderStateBlock(NULL), _state2DRendering(NULL), _depthMapRendering(false) {}

	void exitRenderLoop(bool killCommand = false);

	I8   initHardware(const vec2<U16>& resolution);
	void closeRenderingApi();
	void initDevice(U32 targetFPS);
	void changeResolution(U16 w, U16 h);
	void lookAt(const vec3<F32>& eye,const vec3<F32>& center,const vec3<F32>& up = vec3<F32>(0,1,0), bool invertx = false, bool inverty = false);
	void idle();

    void getModelViewMatrix(mat4<F32>& mvMat);
	void getProjectionMatrix(mat4<F32>& projMat);

	inline FrameBufferObject*  newFBO()                                              {return New glFrameBufferObject(); }
	inline VertexBufferObject* newVBO()                                              {return New glVertexBufferObject(); }
	inline PixelBufferObject*  newPBO()                                              {return New glPixelBufferObject(); }
	inline Texture2D*          newTexture2D(bool flipped = false)                    {return New glTexture(glTextureTypeTable[TEXTURE_2D],flipped);}
	inline TextureCubemap*     newTextureCubemap(bool flipped = false)               {return New glTexture(glTextureTypeTable[TEXTURE_CUBE_MAP],flipped);}
	inline ShaderProgram*      newShaderProgram()                                    {return New glShaderProgram(); }
	inline Shader*             newShader(const std::string& name, SHADER_TYPE type)  {return New glShader(name,type); }

	void clearBuffers(U8 buffer_mask);
	void swapBuffers();
	void enableFog(F32 density, F32* color);

	void lockProjection();
	void releaseProjection();
	void lockModelView();
	void releaseModelView();

	void setOrthoProjection(const vec4<F32>& rect, const vec2<F32>& planes);
	void setPerspectiveProjection(F32 FoV,F32 aspectRatio, const vec2<F32>& planes);

	void toggle2D(bool state);

	void drawTextToScreen(GUIElement* const);
	void drawCharacterToScreen(void* ,char);
	void drawButton(GUIElement* const);
	void drawFlash(GUIElement* const);
	void drawConsole();

	void drawBox3D(const vec3<F32>& min,const vec3<F32>& max, const mat4<F32>& globalOffset);
	void drawLines(const std::vector<vec3<F32> >& pointsA,const std::vector<vec3<F32> >& pointsB,const std::vector<vec4<F32> >& colors, const mat4<F32>& globalOffset);

	void renderInViewport(const vec4<F32>& rect, boost::function0<void> callback);

	void renderModel(Object3D* const model);
	void renderElements(PRIMITIVE_TYPE t, VERTEX_DATA_FORMAT f, U32 count, const void* first_element);
	
	void setMaterial(Material* mat);

	void setAmbientLight(const vec4<F32>& light);
	void setLight(Light* const light);

	void Screenshot(char *filename, const vec4<F32>& rect);

	RenderStateBlock* newRenderStateBlock(const RenderStateBlockDescriptor& descriptor);
	void updateStateInternal(RenderStateBlock* block, bool force = false);

	void toggleDepthMapRendering(bool state);

	void setObjectState(Transform* const transform, bool force = false, ShaderProgram* const shader = NULL);
	void releaseObjectState(Transform* const transform, ShaderProgram* const ShaderProgram = NULL);

	F32 applyCropMatrix(frustum &f,SceneGraph* sceneGraph);

private: //OpenGL specific:
	U8 _windowId;
	glRenderStateBlock* _currentGLRenderStateBlock;
	RenderStateBlock*   _state2DRendering;
	bool _depthMapRendering;
	vec2<U16> _cachedResolution;

END_SINGLETON

#endif
