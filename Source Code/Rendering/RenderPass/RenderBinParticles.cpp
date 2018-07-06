#include "Headers/RenderBinParticles.h"

namespace Divide {

RenderBinParticles::RenderBinParticles(const RenderBinType& rbType,
                                       const RenderingOrder::List& renderOrder,
                                       D32 drawKey)
    : RenderBin(rbType, renderOrder, drawKey) {}

RenderBinParticles::~RenderBinParticles() {}
};