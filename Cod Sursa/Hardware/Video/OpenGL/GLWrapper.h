#ifndef _WRAPPER_GL_H_
#define _WRAPPER_GL_H_
#include "Utility/Headers/Singleton.h"
#include "../RenderAPIWrapper.h"

#include "glFrameBufferObject.h"
#include "glVertexBufferObject.h"
#include "glShader.h"
#include "glTexture.h"

SINGLETON_BEGIN_EXT1(GL_API,RenderAPI)

private:
	GL_API() : RenderAPI() {}

	void initHardware();
	void closeRenderingApi();
	void initDevice();
	void resizeWindow(U32 w, U32 h);
	void lookAt(const vec3& eye,const vec3& center,const vec3& up);
	void idle();

	F32 getTime();
	F32 getMSTime(); 

    mat4 getModelViewMatrix();
	mat4 getProjectionMatrix();

	FrameBufferObject*  newFBO(){return New glFrameBufferObject(); }
	VertexBufferObject* newVBO(){return New glVertexBufferObject(); }

	Texture2D*          newTexture2D(bool flipped = false){return New glTexture(0x0DE1/*GL_TEXTURE_2D*/,flipped);}
	TextureCubemap*     newTextureCubemap(bool flipped = false){return New glTexture(0x8513/*GL_TEXTURE_CUBEMAP*/,flipped);}

	Shader* newShader(const char *vsFile, const char *fsFile){return New glShader(vsFile,fsFile); }
	Shader* newShader(){return New glShader(); }

	typedef void (*callback)();	void glCommand(callback f){f();}

	void translate(const vec3& pos);
	void rotate(F32 angle,const vec3& weights);
    void scale(const vec3& scale);

	void clearBuffers(int buffer_mask);
	void swapBuffers();
	void enableFog(F32 density, F32* color);

	void enable_MODELVIEW();
	void loadIdentityMatrix();
	void toggle2D(bool _2D);

	void drawTextToScreen(Text*);
	void drawCharacterToScreen(void* ,char);
	void drawButton(Button*);
	void drawFlash(GuiFlash* flash);

	void drawBox3D(vec3 min, vec3 max);
    void drawBox3D(Box3D* const box);
	void drawSphere3D(Sphere3D* const sphere);
	void drawQuad3D(Quad3D* const quad);
	void drawText3D(Text3D* const text);

	void renderModel(Mesh* const model);
	void renderElements(Type t, U32 count, const void* first_element);
	
	void setMaterial(Material& mat);
	void setColor(const vec4& color);
	void setColor(const vec3& color);

	void setLight(U32 slot, std::tr1::unordered_map<std::string,vec4>& properties);
	void createLight(U32 slot);
	void setLightCameraMatrices(const vec3& lightVector);
	void restoreLightCameraMatrices();

	void toggleWireframe(bool state);

private: //OpenGL specific:
	void beginRenderStateProcessing();
	void endRenderStateProcessing();

	void pushMatrix();
	void popMatrix();

SINGLETON_END()

#endif
