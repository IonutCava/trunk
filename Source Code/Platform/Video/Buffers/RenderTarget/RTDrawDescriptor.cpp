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

RTDrawDescriptor::RTDrawDescriptor()
    : _stateMask(0)
{
    enableState(State::CLEAR_COLOUR_BUFFERS);
    enableState(State::CLEAR_DEPTH_BUFFER);
    enableState(State::CHANGE_VIEWPORT);

    _drawMask.enableAll();
    _clearColourAttachment.fill(true);

    _clearExternalColour = false;
    _clearExternalDepth = false;
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

void RTDrawDescriptor::markDirtyLayer(RTAttachmentType type, U8 index, U16 layer) {
    vectorEASTL<DirtyLayersEntry>& retEntry = _dirtyLayers[to_base(type)];
    for (DirtyLayersEntry& entry : retEntry) {
        if (entry.first == index) {
            entry.second.insert(layer);
            return;
        }
    }
    retEntry.push_back(std::make_pair(index, DirtyLayers{ layer }));
}

const std::unordered_set<U16>& RTDrawDescriptor::getDirtyLayers(RTAttachmentType type, U8 index) const {
    static std::unordered_set<U16> defaultRet = {};

    const vectorEASTL<DirtyLayersEntry>& retEntry = _dirtyLayers[to_base(type)];
    auto it = eastl::find_if(eastl::cbegin(retEntry),
                             eastl::cend(retEntry),
                             [index](const DirtyLayersEntry& entry) {
                                 return entry.first == index;
                             });
    if (it != std::cend(retEntry)) {
        return it->second;
    }

    return defaultRet;
}

}; //namespace Divide