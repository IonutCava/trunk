#include "Headers/IMPrimitive.h"
#include "Platform/Video/Textures/Headers/Texture.h"

namespace Divide {

IMPrimitive::IMPrimitive()
    : GUIDWrapper(),
      _inUse(true),
      _canZombify(true),
      _forceWireframe(false),
      _paused(true),
      _zombieCounter(0),
      _texture(nullptr),
      _drawShader(nullptr),
      _stateHash(0)
{
}

IMPrimitive::~IMPrimitive() 
{
    clear(); 
}

void IMPrimitive::clear() {
    zombieCounter(0);
    stateHash(0);
    clearRenderStates();
    _worldMatrix.identity();
    _canZombify = true;
    _texture = nullptr;
    _drawShader = nullptr;
}

};