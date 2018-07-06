#include "Headers/d3dDeferredBufferObject.h"

d3dDeferredBufferObject::d3dDeferredBufferObject() : d3dFrameBufferObject(FBO_2D_DEFERRED),
												     _normalBufferHandle(0),
												     _positionBufferHandle(0),
												     _diffuseBufferHandle(0)
{
}