#ifndef _WRAPPER_DX_H_
#define _WRAPPER_DX_H_

#include "Utility/Headers/Singleton.h"
#include "..\RenderAPIWrapper.h"

SINGLETON_BEGIN_EXT1(DX_API,RenderAPI)

private:
	void initHardware();
	void closeRenderingApi();
	void initDevice();

	FrameBufferObject* newFBO(){return /*new dxFrameBufferObject();*/ NULL; }
	VertexBufferObject* newVBO(){return /*new dxVertexBufferObject();*/ NULL; }
	Shader* newShader(const char *vsFile, const char *fsFile){return /*new dxShader();*/ NULL;}
	Shader* newShader(){return /*new dxShader();*/ NULL;}
	void translate(F32 x, F32 y, F32 z);
	void translate(D32 x, D32 y, D32 z);
	void rotate(F32 angle, F32 x, F32 y, F32 z);
	void rotate(D32 angle, D32 x, D32 y, D32 z);
	void scale (F32 x, F32 y, F32 z);
	void scale (D32 x, D32 y, D32 z);
	void scale (int x, int y, int z);

	void clearBuffers(int buffer_mask);
	void swapBuffers();
	void enableFog(F32 density, F32* color);

	void pushMatrix();
	void popMatrix();
	void enable_MODELVIEW();
	void enable_PROJECTION();
	void enable_TEXTURE(int slot);
	void loadIdentityMatrix();
	void loadOrtographicView();
	void loadModelView();

	void drawTextToScreen(Text*);
	void drawCharacterToScreen(void* ,char);
	void drawButton(Button*);

	void renderMesh(const Mesh& mesh);
	void renderSubMesh(const SubMesh& subMesh);

	void setColor(F32 r, F32 g, F32 b);
	void setColor(D32 r, D32 g, D32 b);
	void setColor(int r, int g, int b);
	void setColor(F32 r, F32 g, F32 b, F32 alpha);
	void setColor(D32 r, D32 g, D32 b, D32 alpha);
	void setColor(int r, int g, int b, int alpha);
	void setColor(F32 *v);
	void setColor(D32 *v);
	void setColor(int *v);

	friend class GFXDevice;
	typedef void (*callback)();
	void dxCommand(callback f){(*f)();};

	void setLight(U32 slot, tr1::unordered_map<string,vec4>& properties){};
	void createLight(U32 slot){};
	~DX_API(){};
SINGLETON_END()

#endif