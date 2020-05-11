#include "stdafx.h"

#include "Headers/RTDrawDescriptor.h"

namespace Divide {

RTDrawMask::RTDrawMask() noexcept
    : _disabledDepth(false),
      _disabledStencil(false)
{
    _disabledColours.fill(false);
}

bool RTDrawMask::isEnabled(RTAttachmentType type) const noexcept {
    switch (type) {
        case RTAttachmentType::Depth: return !_disabledDepth;
        case RTAttachmentType::Stencil: return !_disabledStencil;
        case RTAttachmentType::Colour: {
            for (const bool state : _disabledColours) {
                if (!state) {
                    return true;
                }
            }
            return false;
        }
        default: break;
    }

    return true;
}

bool RTDrawMask::isEnabled(RTAttachmentType type, U8 index) const noexcept {
    assert(index < MAX_RT_COLOUR_ATTACHMENTS);

    if (type == RTAttachmentType::Colour) {
        return !_disabledColours[index];
    }
     
    return isEnabled(type);
}

void RTDrawMask::setEnabled(RTAttachmentType type, U8 index, const bool state) noexcept {
    assert(index < MAX_RT_COLOUR_ATTACHMENTS);

    switch (type) {
        case RTAttachmentType::Depth   : _disabledDepth   = !state; break;
        case RTAttachmentType::Stencil : _disabledStencil = !state; break;
        case RTAttachmentType::Colour  : _disabledColours[index] = !state; break;
        default : break;
    }
}

void RTDrawMask::enableAll() noexcept {
    _disabledDepth = _disabledStencil = false;
    _disabledColours.fill(false);
}

void RTDrawMask::disableAll() noexcept {
    _disabledDepth = _disabledStencil = true;
    _disabledColours.fill(true);
}

RTClearDescriptor::RTClearDescriptor() noexcept
    : _clearDepth(true),
      _clearColours(true)
{
    _clearColourAttachment.fill(true);
    _clearExternalColour = false;
    _clearExternalDepth = false;
}

RTDrawDescriptor::RTDrawDescriptor() noexcept
    : _setViewport(true)
{
    _drawMask.enableAll();
}

}; //namespace Divide