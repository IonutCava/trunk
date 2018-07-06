#ifndef _WRAPPER_GL_H_
#define _WRAPPER_GL_H_

#include "Utility/Headers/Singleton.h"
#include "../RenderAPIWrapper.h"
#include "glFrameBufferObject.h"
#include "glVertexBufferObject.h"
#include "glShader.h"

SINGLETON_BEGIN_EXT1(GL_API,RenderAPI)

private:
	
	void initHardware();
	void closeRenderingApi();
	void initDevice();
	void resizeWindow(U32 w, U32 h) {glutReshapeWindow(w,h);}

	FrameBufferObject* newFBO(){return New glFrameBufferObject(); }
	VertexBufferObject* newVBO(){return New glVertexBufferObject(); }
	Shader* newShader(const char *vsFile, const char *fsFile){return New glShader(vsFile,fsFile); }
	Shader* newShader(){return New glShader(); }

	typedef void (*callback)();	void glCommand(callback f){f();}

	void translate(vec3& pos);
	void rotate(F32 angle, vec3& weights);
    void scale(vec3& scale);

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
	void toggle2D3D(bool _3D);

	void drawTextToScreen(Text*);
	void drawCharacterToScreen(void* ,char);
	void drawButton(Button*);
    void drawCube(F32 size);
	void drawSphere(F32 size,U32 resolution);
	void drawQuad(vec3& _topLeft, vec3& _topRight, vec3& _bottomLeft, vec3& _bottomRight);

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

	void setLight(U32 slot, tr1::unordered_map<string,vec4>& properties);
	void createLight(U32 slot);
	~GL_API(){};

SINGLETON_END()

#endif
