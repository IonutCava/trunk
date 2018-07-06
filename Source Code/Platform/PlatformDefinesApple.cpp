#if defined(__APPLE_CC__)

#include "Headers/PlatformDefinesApple.h"
#include "Core/Math/Headers/MathHelper.h"

#include <SDL_syswm.h>

void* malloc_aligned(const size_t size, size_t alignment) {
	return malloc(size);
}

void  malloc_free(void*& ptr) {
	free(ptr);
}

namespace Divide {
    bool PlatformInit() {
        return true;
    }

    bool PlatformClose() {
        return true;
    }

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

}; //namespace Divide

#endif //defined(__APPLE_CC__)
