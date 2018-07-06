#include "Headers/d3dRenderStateBlock.h"

d3dRenderStateBlock::d3dRenderStateBlock(const RenderStateBlockDescriptor& descriptor) : RenderStateBlock(),
   _descriptor(descriptor),
   _hashValue(descriptor.getGUID())
{
}

d3dRenderStateBlock::~d3dRenderStateBlock()
{
}

void d3dRenderStateBlock::activate(d3dRenderStateBlock* oldState){
}