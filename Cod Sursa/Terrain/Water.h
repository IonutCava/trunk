#ifndef _WATER_PLANE_H_
#define _WATER_PLANE_H_

#include "resource.h"
#include "Geometry/Object3D.h"
#include "Geometry/Predefined/Quad3D.h"

#include "Hardware/Video/FrameBufferObject.h"
#include "Utility/Headers/ParamHandler.h"

class Shader;
class WaterPlane : public Object3D
{
public:
	WaterPlane();
	~WaterPlane(){ unload(); }
	void draw(FrameBufferObject* fbo[]);
	bool load(const std::string& name) {return true;}
	bool unload();

	void setParams(F32 shininess, F32 noiseTile, F32 noiseFactor){_shininess = shininess; _noiseTile = noiseTile; _noiseFactor = noiseFactor; } 
	inline Quad3D*     getQuad()    {return _plane;}
	inline Texture2D*  getTexture() {return _texture;}
	inline Shader*     getShader()  {return _shader;}
private:
	Quad3D*			   _plane;
	Texture2D*		   _texture;
	Shader*			   _shader;

	F32 _shininess, _noiseTile, _noiseFactor;
};
#endif