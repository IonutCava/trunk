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
class Mesh;
class Text;
class Shader;
class Button;
class SubMesh;
class FrameBufferObject;
class VertexBufferObject;

//Renderer Programming Interface
class RenderAPI
{

protected:
	
	friend class GFXDevice;
	void setId(GraphicsAPI api) {_apiId = api;}
	GraphicsAPI getId() { return _apiId;}

	virtual FrameBufferObject* newFBO() = 0;
	virtual VertexBufferObject* newVBO() = 0;
	virtual Shader* newShader(const char *vsFile, const char *fsFile) = 0;
	virtual Shader* newShader() = 0;
	
	virtual void initHardware() = 0;
	virtual void closeRenderingApi() = 0;
	virtual void initDevice() = 0;
	/*Geometry transformations*/
	virtual void translate(F32 x, F32 y, F32 z) = 0;
	virtual void translate(D32 x, D32 y, D32 z) = 0;
	virtual void rotate(F32 angle, F32 x, F32 y, F32 z) = 0;
	virtual void rotate(D32 angle, D32 x, D32 y, D32 z) = 0;
	virtual void scale (F32 x, F32 y, F32 z) = 0;
	virtual void scale (D32 x, D32 y, D32 z) = 0;
	virtual void scale (int x, int y, int z) = 0;
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
	virtual void enable_PROJECTION() = 0;
	virtual void enable_TEXTURE(int slot) = 0;
	virtual void loadIdentityMatrix() = 0;
	virtual void loadOrtographicView() = 0;
	virtual void loadModelView() = 0;
	/*State Matrix Manipulation*/

	/*GUI Rendering*/
	virtual void drawTextToScreen(Text*) = 0;
	virtual void drawCharacterToScreen(void* ,char) = 0;
	virtual void drawButton(Button*) = 0;
	/*GUI Rendering*/

	/*Mesh Rendering*/
	virtual void renderMesh(const Mesh& mesh) = 0;
	virtual void renderSubMesh(const SubMesh& subMesh) = 0;
	/*Mesh Rendering*/

	/*Color Management*/
	virtual void setColor(F32 r, F32 g, F32 b) = 0;
	virtual void setColor(D32 r, D32 g, D32 b) = 0;
	virtual void setColor(int r, int g, int b) = 0;
	virtual void setColor(F32 r, F32 g, F32 b, F32 alpha) = 0;
	virtual void setColor(D32 r, D32 g, D32 b, D32 alpha) = 0;
	virtual void setColor(int r, int g, int b, int alpha) = 0;
	virtual void setColor(F32 *v) = 0;
	virtual void setColor(D32 *v) = 0;
	virtual void setColor(int *v) = 0;
	/*Color Management*/

	virtual void setLight(U32 slot, tr1::unordered_map<string,vec4>& properties) = 0;
	virtual void createLight(U32 slot) = 0;

	virtual ~RenderAPI(){};

private:
	GraphicsAPI _apiId;
	
};

#endif