#include "Headers/GenericCommandPool.h"

namespace Divide {

GenericCommandPool::GenericCommandPool(U32 poolSize)
{
    _commandPool.resize(poolSize);
}

GenericCommandPool::~GenericCommandPool()
{
}

GenericDrawCommand& GenericCommandPool::aquire() {
    GenericDrawCommand& cmd = _commandPool.front();
    cmd.reset();
    return cmd;
}

}; //namespace Divide
