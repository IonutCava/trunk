#include "Headers/PlatformDefinesUnix.h"

#include <SDL_syswm.h>

void* malloc_aligned(const size_t size, size_t alignment) {
	return memalign(alignment, size);
}

void  malloc_free(void*& ptr) {
	free(ptr);
}

namespace Divide {

    bool CheckMemory(const U32 physicalRAMNeeded, SysInfo& info) {
        long pages = sysconf(_SC_PHYS_PAGES);
        long page_size = sysconf(_SC_PAGE_SIZE);
        info._availableRam = pages * page_size;
        return info._availableRam > physicalRAMNeeded;
    }

    void getWindowHandle(void* window, SysInfo& info) {
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(static_cast<SDL_Window*>(window), &wmInfo);

        info._windowHandle = wmInfo.info.x11.display;
    }

    void getTicksPerSecond(TimeValue& ticksPerSecond) {
        ticksPerSecond = 1;
    }

    void getCurrentTime(TimeValue& timeOut) {
        gettimeofday(&timeOut, nullptr);
    }

}; //namespace Divide