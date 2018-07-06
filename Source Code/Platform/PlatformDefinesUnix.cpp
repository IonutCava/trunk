#if !defined(_WIN32) && !defined(__APPLE_CC__)

#include "Headers/PlatformDefinesUnix.h"

#include <SDL_syswm.h>
#include <malloc.h>
#include <unistd.h>

void* malloc_aligned(const size_t size, size_t alignment) {
	return memalign(alignment, size);
}

void  malloc_free(void*& ptr) {
	free(ptr);
}

int _vscprintf (const char * format, va_list pargs) {
    int retval;
    va_list argcopy;
    va_copy(argcopy, pargs);
    retval = vsnprintf(NULL, 0, format, argcopy);
    va_end(argcopy);
    return retval;
}

namespace Divide {

    bool PlatformInit(int argc, char** argv) {
        return PlatformPostInit();
    }

    bool PlatformClose() {
        return true;
    }

    bool GetAvailableMemory(SysInfo& info) {
        long pages = sysconf(_SC_PHYS_PAGES);
        long page_size = sysconf(_SC_PAGESIZE);
        info._availableRam = pages * page_size;
        return true;
    }

    void getWindowHandle(void* window, SysInfo& info) {
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(static_cast<SDL_Window*>(window), &wmInfo);

        info._windowHandle = wmInfo.info.x11.window;
    }

    void setThreadName(std::thread* thread, const char* threadName) {
        auto handle = thread->native_handle();
        pthread_setname_np(handle, threadName);
    }

    void createDirectory(const char* path) {
        int ret = mkdir(path, 0777);
        if (ret != 0) {
            return errno == EEXIST;
        }

        return true;
    }

    #include <sys/prctl.h>
    void setThreadName(const char* threadName) {
        prctl(PR_SET_NAME, threadName, 0, 0, 0);
    }

    FileWithPath getExecutableLocation(I32 argc, char** argv) {
        ACKNOWLEDGE_UNUSED(argc);
        char buf[1024] = { 0 };
        ssize_t size = readlink("/proc/self/exe", buf, sizeof(buf));
        if (size == 0 || size == sizeof(buf))
        {
            return getExecutableLocation(argv0);
        }

        return splitPathToNameAndLocation(stringImpl(buf, size));
    }

}; //namespace Divide

#endif //defined(_UNIX)
