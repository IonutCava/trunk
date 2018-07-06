#include "stdafx.h"

#include "Headers/PreRenderOperator.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

F32 PreRenderOperator::s_mainCamAspectRatio;
vec2<F32> PreRenderOperator::s_mainCamZPlanes;
mat4<F32> PreRenderOperator::s_mainCamViewMatrixCache;
mat4<F32> PreRenderOperator::s_mainCamProjectionMatrixCache;

PreRenderOperator::PreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache& cache, FilterType operatorType)
    : _context(context),
      _parent(parent),
      _operatorType(operatorType)
{
    ACKNOWLEDGE_UNUSED(cache);
    _screenOnlyDraw.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);
    _screenOnlyDraw.drawMask().disableAll();
    _screenOnlyDraw.drawMask().setEnabled(RTAttachment::Type::Colour, 0, true);
}

PreRenderOperator::~PreRenderOperator()
{
    _context.deallocateRT(_samplerCopy);
}

void PreRenderOperator::cacheDisplaySettings(const GFXDevice& context) {
    context.getMatrix(MATRIX::PROJECTION, s_mainCamProjectionMatrixCache);
    context.getMatrix(MATRIX::VIEW, s_mainCamViewMatrixCache);
    s_mainCamAspectRatio = context.renderingData().aspectRatio();
    s_mainCamZPlanes.set(context.renderingData().currentZPlanes());
}

void PreRenderOperator::reshape(U16 width, U16 height) {
    if (_samplerCopy._rt) {
        _samplerCopy._rt->create(width, height);
    }
}

void PreRenderOperator::debugPreview(U8 slot) const {
};

}; //namespace Divide