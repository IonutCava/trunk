#include "stdafx.h"

#include "config.h"

#include "Headers/GFXState.h"
#include "Headers/GFXDevice.h"

namespace Divide {

GPUState::GPUState()
{
}

GPUState::~GPUState()
{
}

void GPUState::registerDisplayMode(U8 displayIndex, const GPUVideoMode& mode) {
    if (displayIndex >= _supportedDisplayModes.size()) {
        _supportedDisplayModes.push_back(vectorImpl<GPUVideoMode>());
    }

    vectorImpl<GPUVideoMode>& displayModes = _supportedDisplayModes[displayIndex];

    // this is terribly slow, but should only be called a couple of times and
    // only on video hardware init
    for (GPUVideoMode& crtMode : displayModes) {
        if (crtMode._resolution == mode._resolution &&
            crtMode._bitDepth == mode._bitDepth) {
            U8 crtRefresh = mode._refreshRate.front();
            if (std::find_if(std::begin(crtMode._refreshRate),
                std::end(crtMode._refreshRate),
                [&crtRefresh](U8 refresh)
                -> bool { return refresh == crtRefresh; }) == std::end(crtMode._refreshRate)){
                crtMode._refreshRate.push_back(crtRefresh);
            }
            return;
        }
    }

    displayModes.push_back(mode);
}

};  // namespace Divide