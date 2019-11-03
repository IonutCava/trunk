#include "stdafx.h"

#include "Headers/PreRenderOperator.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

PreRenderOperator::PreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache& cache, FilterType operatorType)
    : _context(context),
      _parent(parent),
      _operatorType(operatorType)
{
    ACKNOWLEDGE_UNUSED(cache);
    _screenOnlyDraw.drawMask().disableAll();
    _screenOnlyDraw.drawMask().setEnabled(RTAttachmentType::Colour, 0, true);
}

PreRenderOperator::~PreRenderOperator()
{
    _context.renderTargetPool().deallocateRT(_samplerCopy);
}

void PreRenderOperator::reshape(U16 width, U16 height) {
    if (_samplerCopy._rt) {
        _samplerCopy._rt->resize(width, height);
    }
}

TextureData PreRenderOperator::getDebugOutput() const {
    return TextureData(0u, 0u, TextureType::COUNT);
};

void PreRenderOperator::onToggle(const bool state) {
    ACKNOWLEDGE_UNUSED(state);
}
}; //namespace Divide