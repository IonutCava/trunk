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
    std::iota(std::begin(_disabledColours), std::end(_disabledColours), to_U8(0));
}


bool RTDrawMask::operator==(const RTDrawMask& other) const {
    return _disabledDepth == other._disabledDepth &&
           _disabledStencil == other._disabledStencil &&
           _disabledColours == other._disabledColours;
}

bool RTDrawMask::operator!=(const RTDrawMask& other) const {
    return _disabledDepth   != other._disabledDepth ||
           _disabledStencil != other._disabledStencil ||
           _disabledColours != other._disabledColours;
}

RTDrawDescriptor::RTDrawDescriptor()
    : _stateMask(0)
{
    enableState(State::CLEAR_COLOUR_BUFFERS);
    enableState(State::CLEAR_DEPTH_BUFFER);
    enableState(State::CHANGE_VIEWPORT);

    _drawMask.enableAll();
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
           _drawMask == other._drawMask;
}

bool RTDrawDescriptor::operator!=(const RTDrawDescriptor& other) const {
    return _stateMask != other._stateMask ||
           _drawMask != other._drawMask;
}

}; //namespace Divide