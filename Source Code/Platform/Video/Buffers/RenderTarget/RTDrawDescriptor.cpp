#include "stdafx.h"

#include "Headers/RTDrawDescriptor.h"

namespace Divide {

RTDrawMask::RTDrawMask()
    : _disabledDepth(false),
      _disabledStencil(false)
{
}

bool RTDrawMask::isEnabled(RTAttachmentType type) const {
    switch (type) {
        case RTAttachmentType::Depth: return !_disabledDepth;
        case RTAttachmentType::Stencil: return !_disabledStencil;
        case RTAttachmentType::Colour: {
            for (bool state : _disabledColours) {
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

bool RTDrawMask::isEnabled(RTAttachmentType type, U8 index) const {
    assert(index < MAX_RT_COLOUR_ATTACHMENTS);

    if (type == RTAttachmentType::Colour) {
        return !_disabledColours[index];
    }
     
    return isEnabled(type);;
}

void RTDrawMask::setEnabled(RTAttachmentType type, U8 index, const bool state) {
    assert(index < MAX_RT_COLOUR_ATTACHMENTS);

    switch (type) {
        case RTAttachmentType::Depth   : _disabledDepth   = !state; break;
        case RTAttachmentType::Stencil : _disabledStencil = !state; break;
        case RTAttachmentType::Colour  : _disabledColours[index] = !state; break;
        default : break;
    }
}

void RTDrawMask::enableAll() {
    _disabledDepth = _disabledStencil = false;
    _disabledColours.fill(false);
}

void RTDrawMask::disableAll() {
    _disabledDepth = _disabledStencil = true;
    _disabledColours.fill(true);
}


bool RTDrawMask::operator==(const RTDrawMask& other) const {
    return _disabledDepth   == other._disabledDepth &&
           _disabledStencil == other._disabledStencil &&
           _disabledColours == other._disabledColours;
}

bool RTDrawMask::operator!=(const RTDrawMask& other) const {
    return _disabledDepth   != other._disabledDepth ||
           _disabledStencil != other._disabledStencil ||
           _disabledColours != other._disabledColours;
}

bool RTBlendState::operator==(const RTBlendState& other) const {
    return _blendProperties == other._blendProperties &&
           _blendColour == other._blendColour;
}

bool RTBlendState::operator!=(const RTBlendState& other) const {
    return _blendProperties != other._blendProperties ||
           _blendColour != other._blendColour;
}

RTDrawDescriptor::RTDrawDescriptor()
    : _stateMask(0)
{
    enableState(State::CLEAR_COLOUR_BUFFERS);
    enableState(State::CLEAR_DEPTH_BUFFER);
    enableState(State::CHANGE_VIEWPORT);

    _drawMask.enableAll();
    _clearColourAttachment.fill(true);
}

void RTDrawDescriptor::stateMask(U32 stateMask) {
    if (Config::Build::IS_DEBUG_BUILD) {
        auto validateMask = [stateMask]() -> U32 {
            U32 validMask = 0;
            for (U32 stateIt = 1; stateIt <= to_base(State::COUNT); ++stateIt) {
                U32 bitState = toBit(stateIt);
                if (BitCompare(stateMask, bitState)) {
                    SetBit(validMask, bitState);
                }
            }
            return validMask;
        };
        
        U32 parsedMask = validateMask();
        DIVIDE_ASSERT(parsedMask == stateMask,
                      "RTDrawDescriptor::stateMask error: Invalid state specified!");
        _stateMask = parsedMask;
    } else {
        _stateMask = stateMask;
    }
}

void RTDrawDescriptor::enableState(State state) {
    SetBit(_stateMask, to_U32(state));
}

void RTDrawDescriptor::disableState(State state) {
    ClearBit(_stateMask, to_U32(state));
}

bool RTDrawDescriptor::isEnabledState(State state) const {
    return BitCompare(_stateMask, to_U32(state));
}

bool RTDrawDescriptor::operator==(const RTDrawDescriptor& other) const {
    return _stateMask == other._stateMask &&
           _drawMask == other._drawMask &&
           _blendStates == other._blendStates;
}

bool RTDrawDescriptor::operator!=(const RTDrawDescriptor& other) const {
    return _stateMask != other._stateMask ||
           _drawMask != other._drawMask ||
           _blendStates != other._blendStates;
}

}; //namespace Divide