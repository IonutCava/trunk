#include "stdafx.h"

#include "Headers/GFXState.h"
#include "Headers/GFXDevice.h"

namespace Divide {

void GPUState::setDisplayModeCount(const U8 displayIndex, const I32 count) {
    _supportedDisplayModes[displayIndex].reserve(count);
}

void GPUState::registerDisplayMode(const U8 displayIndex, const GPUVideoMode& mode) {
    assert(displayIndex < _supportedDisplayModes.size());

    _supportedDisplayModes[displayIndex].push_back(mode);
}

};  // namespace Divide