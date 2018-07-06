#include "Headers/GFXState.h"
#include "Headers/GFXDevice.h"

namespace Divide {

GPUState::GPUState() {
    _loaderThread = nullptr;
    _MSAASamples = 0;
    _FXAASamples = 0;
    _loadingThreadAvailable = false;
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
    _loaderThread->join();
    _loaderThread.reset(nullptr);
    return true;
}

void GPUState::registerDisplayMode(const GPUVideoMode& mode) {
    // this is terribly slow, but should only be called a couple of times and
    // only on video hardware init
    if (std::find(std::begin(_supportedDislpayModes),
                  std::end(_supportedDislpayModes),
                  mode) != std::end(_supportedDislpayModes)) {
        _supportedDislpayModes.push_back(mode);
    }
}

};  // namespace Divide