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

	FrameBufferObject* newFBO(){return new glFrameBufferObject(); }
	VertexBufferObject* newVBO(){return new glVertexBufferObject(); }
	Shader* newShader(const char *vsFile, const char *fsFile){return new glShader(vsFile,fsFile); }
	Shader* newShader(){return new glShader(); }

	typedef void (*callback)();	void glCommand(callback f){f();}

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

	void drawTextToScreen(void*,string);
	void drawCharacterToScreen(void* ,char);

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

	~GL_API(){};

SINGLETON_END()

#endif
