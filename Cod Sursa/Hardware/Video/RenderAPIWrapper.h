#ifndef _RENDER_API_H
#define _RENDER_API_H

#include <string>
#include "Utility/Headers/DataTypes.h"
#include <unordered_map>

using namespace std;

enum GraphicsAPI
{
	OpenGL10,
	OpenGL12,
	OpenGL20,
	OpenGL21,
	OpenGL22,
	OpenGL30,
	OpenGL32,
	Direct3D8,
	Direct3D9,
	Direct3D10
};

//Forward declarations
class DVDFile;
class Text;
class Shader;
class Button;
class SubMesh;
class FrameBufferObject;
class VertexBufferObject;
class Box3D;
class Sphere3D;
class Quad3D;
class Text3D;
//Renderer Programming Interface
class RenderAPI
{

protected:
	
	friend class GFXDevice;
	void setId(GraphicsAPI api) {_apiId = api;}
	GraphicsAPI getId() { return _apiId;}
	virtual void resizeWindow(U32 w, U32 h) = 0;
	virtual FrameBufferObject* newFBO() = 0;
	virtual VertexBufferObject* newVBO() = 0;
	virtual Shader* newShader(const char *vsFile, const char *fsFile) = 0;
	virtual Shader* newShader() = 0;
	
	virtual void initHardware() = 0;
	virtual void closeRenderingApi() = 0;
	virtual void initDevice() = 0;
	/*Geometry transformations*/
	virtual void translate(vec3& pos) = 0;
	virtual void rotate(F32 angle, vec3& weights) = 0;
    virtual void scale(vec3& scale) = 0;
	/*Geometry transformations*/

	/*Rendering States*/
	virtual void clearBuffers(int buffer_mask) = 0;
	virtual void swapBuffers() = 0;
	virtual void enableFog(F32 density, F32* color) = 0;
	/*Rendering States*/

	/*State Matrix Manipulation*/
	virtual void pushMatrix() = 0;
	virtual void popMatrix() = 0;
	virtual void enable_MODELVIEW() = 0;
	virtual void loadIdentityMatrix() = 0;
	virtual void toggle2D3D(bool _3D) = 0;
	/*State Matrix Manipulation*/

	/*GUI Rendering*/
	virtual void drawTextToScreen(Text*) = 0;
	virtual void drawCharacterToScreen(void* ,char) = 0;
	virtual void drawButton(Button*) = 0;
	virtual void drawBox3D(Box3D* const box) = 0;
	virtual void drawSphere3D(Sphere3D* const sphere) = 0;
	virtual void drawQuad3D(Quad3D* const quad) = 0;
	virtual void drawText3D(Text3D* const text) = 0;
	/*GUI Rendering*/

	/*Mesh Rendering*/
	virtual void renderModel(DVDFile* const model) = 0;
	/*Mesh Rendering*/

	/*Color Management*/
	virtual void setColor(vec4& v) = 0;
	virtual void setColor(vec3& v) = 0;
	/*Color Management*/

	virtual void setLight(U32 slot, tr1::unordered_map<string,vec4>& properties) = 0;
	virtual void createLight(U32 slot) = 0;

	virtual ~RenderAPI(){};

private:
	GraphicsAPI _apiId;
	
};

#endif