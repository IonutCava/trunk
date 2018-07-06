#include "Headers/GFXState.h"
#include "Headers/GFXDevice.h"

namespace Divide {

GPUState::GPUState() {
    _loaderThread = nullptr;
    _MSAASamples = 0;
    _FXAASamples = 0;
    _loadingThreadAvailable = false;
    // Atomic boolean use to signal the loading thread to stop
    _closeLoadingThread = false;
}

bool GPUState::startLoaderThread(const DELEGATE_CBK<>& loadingFunction) {
    DIVIDE_ASSERT(_loaderThread == nullptr,
                  "GPUState::startLoaderThread error: double init detected!");
    _loaderThread.reset(new std::thread(loadingFunction));
    return true;
}

bool GPUState::stopLoaderThread() {
    DIVIDE_ASSERT(_loaderThread != nullptr,
                  "GPUState::stopLoaderThread error: stop called without "
                  "calling start first!");
    closeLoadingThread(true);
    while (loadingThreadAvailable()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    _loaderThread->join();
    _loaderThread.reset(nullptr);
    return true;
}

void GPUState::registerDisplayMode(const GPUVideoMode& mode) {
    // this is terribly slow, but should only be called a couple of times and
    // only on video hardware init
    for (GPUVideoMode& crtMode : _supportedDislpayModes) {
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
    _supportedDislpayModes.push_back(mode);
}

};  // namespace Divide