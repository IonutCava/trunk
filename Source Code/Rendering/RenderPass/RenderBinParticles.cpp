#include "Headers/RenderBinParticles.h"

RenderBinParticles::RenderBinParticles(const RenderBinType& rbType,const RenderingOrder::List& renderOrder, D32 drawKey) : RenderBin(rbType, renderOrder,drawKey)
{
}

RenderBinParticles::~RenderBinParticles()
{
}