#include "Headers/RTDrawDescriptor.h"
#include <numeric>

namespace Divide {

RTDrawMask::RTDrawMask()
    : _disabledDepth(false),
      _disabledStencil(false)
{
}

bool RTDrawMask::isEnabled(RTAttachment::Type type, U8 index) const {
    switch (type) {
        case RTAttachment::Type::Depth   : return !_disabledDepth;
        case RTAttachment::Type::Stencil : return !_disabledStencil;
        case RTAttachment::Type::Colour  : {
            for (U8 crtIndex : _disabledColours) {
                if (crtIndex == index) {
                    return false;
                }
            }
        } break;
    }

    return true;
}

void RTDrawMask::setEnabled(RTAttachment::Type type, U8 index, const bool state) {
    switch (type) {
        case RTAttachment::Type::Depth   : _disabledDepth   = !state; break;
        case RTAttachment::Type::Stencil : _disabledStencil = !state; break;
        case RTAttachment::Type::Colour  : {
            for (U8 crtIndex : _disabledColours) {
                if (crtIndex == index) {
                    return;
                }
            }
            _disabledColours.push_back(index);
        } break;
    }
}

void RTDrawMask::enableAll() {
    _disabledDepth = _disabledStencil = false;
    _disabledColours.clear();
}

void RTDrawMask::disableAll() {
    _disabledDepth = _disabledStencil = true;
    _disabledColours.resize(std::numeric_limits<U8>::max());
    std::iota(std::begin(_disabledColours), std::end(_disabledColours), 0);
}


bool RTDrawMask::operator==(const RTDrawMask& other) const {
    return _disabledDepth == other._disabledDepth &&
           _disabledStencil == other._disabledStencil &&
           _disabledColours == other._disabledColours;
}

bool RTDrawMask::operator!=(const RTDrawMask& other) const {
    return _disabledDepth != other._disabledDepth ||
           _disabledStencil != other._disabledStencil ||
           _disabledColours != other._disabledColours;
}

RTDrawDescriptor::RTDrawDescriptor()
    : _clearColourBuffersOnBind(true),
      _clearDepthBufferOnBind(true),
      _changeViewport(true)
{
    _drawMask.enableAll();
}

}; //namespace Divide