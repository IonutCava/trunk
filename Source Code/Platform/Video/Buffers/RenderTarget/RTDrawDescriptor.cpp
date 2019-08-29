#include "stdafx.h"

#include "Headers/RTDrawDescriptor.h"

namespace Divide {

RTDrawMask::RTDrawMask()
    : _disabledDepth(false),
      _disabledStencil(false)
{
    _disabledColours.fill(false);
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
     
    return isEnabled(type);
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

RTClearDescriptor::RTClearDescriptor()
    : _clearDepth(true),
      _clearColours(true)
{
    _clearColourAttachment.fill(true);
    _clearExternalColour = false;
    _clearExternalDepth = false;
}

RTDrawDescriptor::RTDrawDescriptor()
    : _setViewport(true)
{
    _drawMask.enableAll();
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