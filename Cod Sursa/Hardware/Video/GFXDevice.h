#ifndef _GFX_DEVICE_H
#define _GFX_DEVICE_H

#include "OpenGL\GLWrapper.h"
#include "Direct3D\DXWrapper.h"

SINGLETON_BEGIN_EXT1(GFXDevice,RenderAPI)

public:
	void setApi(GraphicsAPI api);
	int  getApi(){return _api.getId(); }
	void initHardware(){_api.initHardware();}
	void initDevice(){_api.initDevice();}
	void resizeWindow(U32 w, U32 h);

	void closeRenderingApi(){_api.closeRenderingApi();}
	FrameBufferObject* newFBO(){return _api.newFBO(); }
	VertexBufferObject* newVBO(){return _api.newVBO(); }
	Shader* newShader(const char *vsFile, const char *fsFile){return _api.newShader(vsFile,fsFile); }
	Shader* newShader(){return _api.newShader(); }

	/*typedef void (*callback)();
	void glCommand(callback f){GL_API::getInstance().glCommand(f);}
	void dxCommand(callback f){DX_API::getInstance().dxCommand(f);}
	*/

	
	void translate(vec3& pos){_api.translate(pos);}
	void rotate(F32 angle, vec3& weights){_api.rotate(angle,weights);}
	void scale(vec3& scale){_api.scale(scale);}

	void rotate(vec3& rot)
	{
		_api.rotate(rot.x,vec3(1.0f,0.0f,0.0f)); 
		_api.rotate(rot.y,vec3(0.0f,1.0f,0.0f));
		_api.rotate(rot.z,vec3(0.0f,0.0f,1.0f));
	}



	void clearBuffers(int buffer_mask){_api.clearBuffers(buffer_mask);}
	void swapBuffers(){_api.swapBuffers();}
	void enableFog(F32 density, F32* color){_api.enableFog(density,color);}

	void pushMatrix(){_api.pushMatrix();}
	void popMatrix(){_api.popMatrix();}
	void enable_MODELVIEW(){_api.enable_MODELVIEW();}
	void enable_PROJECTION(){_api.enable_PROJECTION();}
	void enable_TEXTURE(int slot){_api.enable_TEXTURE(slot);}
	void loadIdentityMatrix(){_api.loadIdentityMatrix();}
	void loadOrtographicView(){_api.loadOrtographicView();}
	void loadModelView(){_api.loadModelView();}
	void toggle2D3D(bool _3D) {_api.toggle2D3D(_3D);}

	void drawTextToScreen(Text* text){_api.drawTextToScreen(text);}
	void drawCharacterToScreen(void* font,char character){_api.drawCharacterToScreen(font,character);}
	void drawButton(Button* button){_api.drawButton(button);}
	void drawCube(F32 size){_api.drawCube(size);}
	void drawSphere(F32 size,U32 resolution){_api.drawSphere(size,resolution);}
	void drawQuad(vec3& _topLeft, vec3& _topRight, vec3& _bottomLeft, vec3& _bottomRight){_api.drawQuad(_topLeft,_topRight,_bottomLeft,_bottomRight);}

	void renderMesh(const Mesh& mesh){_api.renderMesh(mesh);}
	void renderSubMesh(const SubMesh& subMesh){_api.renderSubMesh(subMesh);}

	void setColor(F32 r, F32 g, F32 b){_api.setColor(r,g,b);}
	void setColor(D32 r, D32 g, D32 b){_api.setColor(r,g,b);}
	void setColor(int r, int g, int b){_api.setColor(r,g,b);}
	void setColor(F32 r, F32 g, F32 b, F32 alpha){_api.setColor(r,g,b,alpha);}
	void setColor(D32 r, D32 g, D32 b, D32 alpha){_api.setColor(r,g,b,alpha);}
	void setColor(int r, int g, int b, int alpha){_api.setColor(r,g,b,alpha);}
	void setColor(F32 *v){_api.setColor(v);}
	void setColor(D32 *v){_api.setColor(v);}
	void setColor(int *v){_api.setColor(v);}

	void setLight(U32 slot, tr1::unordered_map<string,vec4>& properties){_api.setLight(slot,properties);}
	void createLight(U32 slot){_api.createLight(slot);}

private:
	GFXDevice() :
	   _api(GL_API::getInstance()) //Defaulting to OpenGL if no api has been defined
	   {}
	RenderAPI& _api;


SINGLETON_END()

#endif
