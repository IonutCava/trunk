#ifndef _GFX_DEVICE_H
#define _GFX_DEVICE_H

#include "OpenGL\GLWrapper.h"
#include "Direct3D\DXWrapper.h"

class Object3D;
class Object3DFlyWeight;
class Framerate;
SINGLETON_BEGIN_EXT1(GFXDevice,RenderAPIWrapper)

public:
	void setApi(RenderAPI api);
	I8  getApi(){return _api.getId(); }
	void initHardware(){_api.initHardware();}
	void initDevice(){_api.initDevice();}
	void resizeWindow(U16 w, U16 h);
	void lookAt(const vec3& eye,const vec3& center,const vec3& up){_api.lookAt(eye,center,up);}
	void idle() {_api.idle();}

	mat4 getModelViewMatrix(){return _api.getModelViewMatrix();}
	mat4 getProjectionMatrix(){return _api.getProjectionMatrix();}

	void closeRenderingApi(){_api.closeRenderingApi();}
	FrameBufferObject* newFBO(){return _api.newFBO(); }
	VertexBufferObject* newVBO(){return _api.newVBO(); }
	PixelBufferObject*  newPBO(){return _api.newPBO(); }

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

	void clearBuffers(U8 buffer_mask){_api.clearBuffers(buffer_mask);}
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

	void renderModel(Object3D* const model);
	void renderElements(Type t, U32 count, const void* first_element){_api.renderElements(t,count,first_element);}
	void renderElements(std::tr1::unordered_map<std::string,Object3D*>&  primitiveArray);
	void renderElements(std::vector<Object3DFlyWeight*>& geometryArray);
	
	void setMaterial(Material& mat){_api.setMaterial(mat);}
	void setColor(const vec4& v){_api.setColor(v);}
	void setColor(const vec3& v){_api.setColor(v);}

	void setLight(U8 slot, std::tr1::unordered_map<std::string,vec4>& properties){_api.setLight(slot,properties);}
	void createLight(U8 slot){_api.createLight(slot);}
	void setLightCameraMatrices(const vec3& lightVector){_api.setLightCameraMatrices(lightVector);}
	void restoreLightCameraMatrices(){_api.restoreLightCameraMatrices();}

   void toggleWireframe(bool state = false);
   void setDepthMapRendering(bool state) {_depthMapRendering = state;}
   bool getDepthMapRendering() {return _depthMapRendering;}

   void setDeferredShading(bool state) {_deferredShading = state;}
   bool getDeferredShading() {return _deferredShading;}

   bool wireframeRendering() {return _wireframeMode;}  
   void Screenshot(char *filename, U16 xmin, U16 ymin, U16 xmax, U16 ymax);

   void setRenderState(RenderState& state) {_state = state; _api.setRenderState(state);}

public:
	enum BufferType
	{
		COLOR_BUFFER   = 0x0001,
		DEPTH_BUFFER   = 0x0010,
		STENCIL_BUFFER = 0x0100
	};

private:
	GFXDevice() :
	   _api(GL_API::getInstance()) //Defaulting to OpenGL if no api has been defined
	   {
		   _wireframeMode = false;
		   _depthMapRendering = false;
		   _deferredShading = false;
	   }
	RenderAPIWrapper& _api;
	bool _wireframeMode,_depthMapRendering,_deferredShading;
SINGLETON_END()

#endif
