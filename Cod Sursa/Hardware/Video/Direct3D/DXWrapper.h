#ifndef _WRAPPER_DX_H_
#define _WRAPPER_DX_H_

#include "Utility/Headers/Singleton.h"
#include "../RenderAPIWrapper.h"

SINGLETON_BEGIN_EXT1(DX_API,RenderAPI)

private:
	void initHardware();
	void closeRenderingApi();
	void initDevice();
	void resizeWindow(U32 w, U32 h) {}

	FrameBufferObject* newFBO(){return /*new dxFrameBufferObject();*/ NULL; }
	VertexBufferObject* newVBO(){return /*new dxVertexBufferObject();*/ NULL; }
	Shader* newShader(const char *vsFile, const char *fsFile){return /*new dxShader();*/ NULL;}
	Shader* newShader(){return /*new dxShader();*/ NULL;}
	
	void translate(vec3& pos);
	void rotate(F32 angle, vec3& weights);
	void scale (vec3& scale);


	void clearBuffers(int buffer_mask);
	void swapBuffers();
	void enableFog(F32 density, F32* color);

	void pushMatrix();
	void popMatrix();
	void enable_MODELVIEW();

	void loadIdentityMatrix();
	void toggle2D3D(bool _3D);

	void drawTextToScreen(Text*);
	void drawCharacterToScreen(void* ,char);
	void drawButton(Button*);
	void drawBox3D(Box3D* const box);
	void drawSphere3D(Sphere3D* const sphere);
	void drawQuad3D(Quad3D* const quad);
	void drawText3D(Text3D* const text);

	void renderModel(DVDFile* const model);
	
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