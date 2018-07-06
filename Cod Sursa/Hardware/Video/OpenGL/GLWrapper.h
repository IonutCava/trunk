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

	void setLight(U32 slot, tr1::unordered_map<string,vec4>& properties);
	void createLight(U32 slot);
	~GL_API(){};

SINGLETON_END()

#endif
