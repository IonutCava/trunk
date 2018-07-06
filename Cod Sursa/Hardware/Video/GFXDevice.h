#ifndef _GFX_DEVICE_H
#define _GFX_DEVICE_H

#include "OpenGL\GLWrapper.h"
#include "Direct3D\DXWrapper.h"

class Object3D;
class Object3DFlyWeight;

SINGLETON_BEGIN_EXT1(GFXDevice,RenderAPI)

public:
	void setApi(GraphicsAPI api);
	int  getApi(){return _api.getId(); }
	void initHardware(){_api.initHardware();}
	void initDevice(){_api.initDevice();}
	void resizeWindow(U32 w, U32 h);
	void lookAt(const vec3& eye,const vec3& center,const vec3& up){_api.lookAt(eye,center,up);}
	void idle() {_api.idle();}

	F32 getTime(){return _api.getTime();}
	F32 getMSTime(){return _api.getMSTime();}
	mat4 getModelViewMatrix(){return _api.getModelViewMatrix();}
	mat4 getProjectionMatrix(){return _api.getProjectionMatrix();}

	void closeRenderingApi(){_api.closeRenderingApi();}
	FrameBufferObject* newFBO(){return _api.newFBO(); }
	VertexBufferObject* newVBO(){return _api.newVBO(); }

	Texture2D*          newTexture2D(bool flipped = false){return _api.newTexture2D(flipped);}
	TextureCubemap*     newTextureCubemap(bool flipped = false){return _api.newTextureCubemap(flipped);}

	Shader* newShader(const char *vsFile, const char *fsFile){return _api.newShader(vsFile,fsFile); }
	Shader* newShader(){return _api.newShader(); }

	void translate(const vec3& pos){_api.translate(pos);}
	void rotate(F32 angle,const vec3& weights){_api.rotate(angle,weights);}
	void scale(const vec3& scale){_api.scale(scale);}

	void rotate(const vec3& rot)
	{
		_api.rotate(rot.x,vec3(1.0f,0.0f,0.0f)); 
		_api.rotate(rot.y,vec3(0.0f,1.0f,0.0f));
		_api.rotate(rot.z,vec3(0.0f,0.0f,1.0f));
	}

	void clearBuffers(int buffer_mask){_api.clearBuffers(buffer_mask);}
	void swapBuffers(){_api.swapBuffers();}
	void enableFog(F32 density, F32* color){_api.enableFog(density,color);}

	void enable_MODELVIEW(){_api.enable_MODELVIEW();}
	void loadIdentityMatrix(){_api.loadIdentityMatrix();}
	void toggle2D(bool _2D) {_api.toggle2D(_2D);}

	void drawTextToScreen(Text* text){_api.drawTextToScreen(text);}
	void drawCharacterToScreen(void* font,char character){_api.drawCharacterToScreen(font,character);}
	void drawButton(Button* button){_api.drawButton(button);}
	void drawFlash(GuiFlash* flash){_api.drawFlash(flash);}

	void drawBox3D(Box3D* const box){_api.drawBox3D(box);}
	void drawBox3D(vec3 min, vec3 max){_api.drawBox3D(min,max);}
	void drawSphere3D(Sphere3D* const sphere){_api.drawSphere3D(sphere);}
	void drawQuad3D(Quad3D* const quad){_api.drawQuad3D(quad);}
	void drawText3D(Text3D* const text){_api.drawText3D(text);}

	void renderModel(Mesh* const model);
	void renderElements(Type t, U32 count, const void* first_element){_api.renderElements(t,count,first_element);}
	void renderElements(std::tr1::unordered_map<std::string,Object3D*>&  primitiveArray);
	void renderElements(std::tr1::unordered_map<std::string,Mesh*>&  geometryArray);
	void renderElements(std::vector<Mesh*>& geometryArray);
	void renderElements(std::vector<Object3DFlyWeight*>& geometryArray);
	
	void setMaterial(Material& mat){_api.setMaterial(mat);}
	void setColor(const vec4& v){_api.setColor(v);}
	void setColor(const vec3& v){_api.setColor(v);}

	void setLight(U32 slot, std::tr1::unordered_map<std::string,vec4>& properties){_api.setLight(slot,properties);}
	void createLight(U32 slot){_api.createLight(slot);}
	void setLightCameraMatrices(const vec3& lightVector){_api.setLightCameraMatrices(lightVector);}
	void restoreLightCameraMatrices(){_api.restoreLightCameraMatrices();}

   void toggleWireframe(bool state = false);
   void setDepthMapRendering(bool state) {_depthMapRendering = state;}
   bool getDepthMapRendering() {return _depthMapRendering;}
   bool wireframeRendering() {return _wireframeMode;}  
   void Screenshot(char *filename, int xmin, int ymin, int xmax, int ymax);
private:
	GFXDevice() :
	   _api(GL_API::getInstance()) //Defaulting to OpenGL if no api has been defined
	   {
		   _wireframeMode = false;
		   _depthMapRendering = false;
	   }
	RenderAPI& _api;
	bool _wireframeMode,_depthMapRendering;
SINGLETON_END()

#endif
