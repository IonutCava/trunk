#ifndef _LIGHT_IMPOSTOR_H_
#define _LIGHT_IMPOSTOR_H_

#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

class SceneGraphNode;
class LightImpostor {
friend class Light;
private:
	LightImpostor(const std::string& lightName, F32 radius);
	~LightImpostor();

	void render(SceneGraphNode* const lightNode);
	inline void setRadius(F32 radius) {_dummy->setRadius(radius);}
	inline Sphere3D* const getDummy() {return _dummy;}

private:
	bool      _visible;
	Sphere3D* _dummy;
	RenderStateBlock* _dummyStateBlock;
};

#endif