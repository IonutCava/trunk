#include "Headers/RenderBinTranslucent.h"


RenderBinTranslucent::RenderBinTranslucent(const RenderBinType& rbType,const RenderingOrder::List& renderOrder, D32 drawKey) : RenderBin(rbType, renderOrder,drawKey)
{
}

RenderBinTranslucent::~RenderBinTranslucent()
{
}

