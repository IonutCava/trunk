#include "stdafx.h"

#include "config.h"

#include "Headers/GFXState.h"
#include "Headers/GFXDevice.h"

namespace Divide {

void GPUState::setDisplayModeCount(U8 displayIndex, I32 count) {
    _supportedDisplayModes[displayIndex].reserve(count);
}

void GPUState::registerDisplayMode(U8 displayIndex, const GPUVideoMode& mode) {
    assert(displayIndex < _supportedDisplayModes.size());

    _supportedDisplayModes[displayIndex].push_back(mode);
}

};  // namespace Divide