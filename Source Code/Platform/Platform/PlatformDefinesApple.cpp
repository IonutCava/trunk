#include "Headers/PlatformDefinesApple.h"

#include <SDL_syswm.h>

namespace Divide {
    bool CheckMemory(const U32 physicalRAMNeeded, SysInfo& info) {
        I32 mib[2] = { CTL_HW, HW_MEMSIZE };
        U32 namelen = sizeof(mib) / sizeof(mib[0]);
        U64 size;
        size_t len = sizeof(size);
        if (sysctl(mib, namelen, &size, &len, NULL, 0) < 0) {
            perror("sysctl");
        } else  {
            info._availableRam = static_cast<size_t>(size);
        }

        return info._availableRam > physicalRAMNeeded;
    }

    void getWindowHandle(void* window, SysInfo& info) {
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(static_cast<SDL_Window*>(window), &wmInfo);

        info._windowHandle = wmInfo.info.cocoa.window;
    }

    void getTicksPerSecond(TimeValue& ticksPerSecond) {
        ticksPerSecond = 1;
    }

    void getCurrentTime(TimeValue& timeOut) {
        gettimeofday(&timeOut, nullptr);
    }

}; //namespace Divide