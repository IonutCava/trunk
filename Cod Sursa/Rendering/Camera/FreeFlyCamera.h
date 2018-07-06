#ifndef _FREE_FLY_CAMERA_H_
#define _FREE_FLY_CAMERA_H_

#include "Camera.h"

class FreeFlyCamera : public Camera
{

public:
	FreeFlyCamera()
	{
		eType = FREE_FLY;
	}
};

#endif