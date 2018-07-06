#ifndef _WRAPPER_DX_H_
#define _WRAPPER_DX_H_

#include "Utility/Headers/Singleton.h"
#include "../RenderAPIWrapper.h"

SINGLETON_BEGIN_EXT1(DX_API,RenderAPI)

private:
	DX_API() : RenderAPI() {}

	void initHardware();
	void closeRenderingApi();
	void initDevice();
	void resizeWindow(U32 w, U32 h) {}
	void lookAt(const vec3& eye,const vec3& center,const vec3& up);
	void idle();

	F32 getTime()   {return (F32)(1000.0f/1000.0f);}
	F32 getMSTime() {return (F32)1000.0f;} 
	mat4 getModelViewMatrix();
	mat4 getProjectionMatrix();

	FrameBufferObject* newFBO(){return /*new dxFrameBufferObject();*/ NULL; }
	VertexBufferObject* newVBO(){return /*new dxVertexBufferObject();*/ NULL; }
	Texture2D*          newTexture2D(bool flipped = false){return /*new dxTexture2D();*/ NULL;}
	TextureCubemap*     newTextureCubemap(bool flipped = false){return /*new dxTextureCubemap();*/ NULL;}
	Shader* newShader(const char *vsFile, const char *fsFile){return /*new dxShader();*/ NULL;}
	Shader* newShader(){return /*new dxShader();*/ NULL;}
	
	void translate(vec3& pos);
	void rotate(F32 angle, vec3& weights);
	void scale (vec3& scale);


	void clearBuffers(int buffer_mask);
	void swapBuffers();
	void enableFog(F32 density, F32* color);

	void enable_MODELVIEW();

	void loadIdentityMatrix();
	void toggle2D(bool _2D);

	void drawTextToScreen(Text*);
	void drawCharacterToScreen(void* ,char);
	void drawButton(Button*);

	void drawBox3D(vec3 min, vec3 max);
	void drawBox3D(Box3D* const box);
	void drawSphere3D(Sphere3D* const sphere);
	void drawQuad3D(Quad3D* const quad);
	void drawText3D(Text3D* const text);

	void renderModel(DVDFile* const model);
	void renderElements(Type t, U32 count, const void* first_element);

	void setColor(vec4& color);
	void setColor(vec3& color);

	friend class GFXDevice;
	typedef void (*callback)();
	void dxCommand(callback f){(*f)();};

	void setLight(U32 slot, tr1::unordered_map<string,vec4>& properties){};
	void createLight(U32 slot){};
	~DX_API(){};
SINGLETON_END()

#endif