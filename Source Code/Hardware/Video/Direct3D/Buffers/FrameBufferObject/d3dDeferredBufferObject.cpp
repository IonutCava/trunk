#include "Headers/d3dDeferredBufferObject.h"

d3dDeferredBufferObject::d3dDeferredBufferObject() : d3dFrameBufferObject() ,
												     _normalBufferHandle(0),
												     _positionBufferHandle(0),
												     _diffuseBufferHandle(0)
{
	_fboType = FBO_2D_DEFERRED;
}