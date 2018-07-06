#include "Headers/PlatformDefinesUnix.h"

#include <GL/glfw3.h>
#include <GL/glfw3native.h>

namespace Divide {

    bool CheckMemory(const U32 physicalRAMNeeded, SysInfo& info) {
        long pages = sysconf(_SC_PHYS_PAGES);
        long page_size = sysconf(_SC_PAGE_SIZE);
        info._availableRam = pages * page_size;
        return info._availableRam > physicalRAMNeeded;
    }

    void getWindowHandle(void* windowClass, SysInfo& info) {

        info._windowHandle = glfwGetX11Window(static_cast<GLFWwindow*>(windowClass));
    }

    void getTicksPerSecond(TimeValue& ticksPerSecond) {
        ticksPerSecond = 1;
    }

    void getCurrentTime(TimeValue& timeOut) {
        gettimeofday(&timeOut, nullptr);
    }

}; //namespace Divide