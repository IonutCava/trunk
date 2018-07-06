#include "Headers/RenderBinDelegate.h"


RenderBinDelegate::RenderBinDelegate(const RenderBinType& rbType,const RenderingOrder::List& renderOrder, D32 drawKey) : RenderBin(rbType, renderOrder,drawKey)
{
}

RenderBinDelegate::~RenderBinDelegate()
{
}