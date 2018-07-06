#include "Headers/PreRenderOperator.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

F32 PreRenderOperator::s_mainCamAspectRatio;
vec2<F32> PreRenderOperator::s_mainCamZPlanes;
mat4<F32> PreRenderOperator::s_mainCamViewMatrixCache;
mat4<F32> PreRenderOperator::s_mainCamProjectionMatrixCache;

void PreRenderOperator::cacheDisplaySettings(const GFXDevice& context) {
    context.getMatrix(MATRIX::PROJECTION, s_mainCamProjectionMatrixCache);
    context.getMatrix(MATRIX::VIEW, s_mainCamViewMatrixCache);
    s_mainCamAspectRatio = context.renderingData().aspectRatio();
    s_mainCamZPlanes.set(context.renderingData().currentZPlanes());
}

}; //namespace Divide