#if defined(_WIN32)

#include "Headers/PlatformDefines.h"

// We are actually importing GL specific libraries in code mainly for
// maintenance reasons
// We can easily adjust them as needed. Same thing with PhysX libs
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "OpenAL32.lib")
#pragma comment(lib, "SDL2_mixer.lib")
#pragma comment(lib, "Winmm.lib")

#ifdef _DEBUG
#pragma comment(lib, "DbgHelp.lib")

#pragma comment(lib, "glbindingd.lib")
#pragma comment(lib, "OIS_static_d.lib")
#pragma comment(lib, "assimp_d.lib")
#pragma comment(lib, "IL_d.lib")
#pragma comment(lib, "libpng_d.lib")
#pragma comment(lib, "jpeg_d.lib")
#pragma comment(lib, "libmng_d.lib")
#pragma comment(lib, "libtiff_d.lib")
#pragma comment(lib, "freetype_d.lib")
#pragma comment(lib, "zlib_d.lib")
#pragma comment(lib, "SILLY_d.lib")
#pragma comment(lib, "pcre_d.lib")
#pragma comment(lib, "expat_d.lib")

#pragma comment(lib, "CEGUIOpenGLRenderer-0_Static_d.lib")
#pragma comment(lib, "CEGUIBase-0_Static_d.lib")
#pragma comment(lib, "CEGUISILLYImageCodec_Static_d.lib")
#pragma comment(lib, "CEGUICoreWindowRendererSet_Static_d.lib")
#pragma comment(lib, "CEGUIExpatParser_Static_d.lib")

#pragma comment(lib, "PhysXProfileSDKDEBUG.lib")
#pragma comment(lib, "PhysX3CookingDEBUG_x64.lib")
#pragma comment(lib, "PhysX3DEBUG_x64.lib")
#pragma comment(lib, "PhysX3CommonDEBUG_x64.lib")
#pragma comment(lib, "PhysX3ExtensionsDEBUG.lib")
#pragma comment(lib, "PhysXVisualDebuggerSDKDEBUG.lib")

#else  //_DEBUG
#pragma comment(lib, "glbinding.lib")
#pragma comment(lib, "OIS_static.lib")
#pragma comment(lib, "assimp.lib")
#pragma comment(lib, "IL.lib")
#pragma comment(lib, "libpng.lib")
#pragma comment(lib, "jpeg.lib")
#pragma comment(lib, "libmng.lib")
#pragma comment(lib, "libtiff.lib")
#pragma comment(lib, "freetype.lib")
#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "SILLY.lib")
#pragma comment(lib, "pcre.lib")
#pragma comment(lib, "expat.lib")

#pragma comment(lib, "CEGUIOpenGLRenderer-0_Static.lib")
#pragma comment(lib, "CEGUIBase-0_Static.lib")
#pragma comment(lib, "CEGUISILLYImageCodec_Static.lib")
#pragma comment(lib, "CEGUICoreWindowRendererSet_Static.lib")
#pragma comment(lib, "CEGUIExpatParser_Static.lib")

#pragma comment(lib, "PhysXProfileSDKCHECKED.lib")
#pragma comment(lib, "PhysX3CookingCHECKED_x64.lib")
#pragma comment(lib, "PhysX3CHECKED_x64.lib")
#pragma comment(lib, "PhysX3CommonCHECKED_x64.lib")
#pragma comment(lib, "PhysX3ExtensionsCHECKED.lib")
#pragma comment(lib, "PhysXVisualDebuggerSDKCHECKED.lib")
#endif  //_DEBUG

#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
// SDL redefines WIN32_LEAN_AND_MEAN
#include <SDL_syswm.h>
#endif

#ifdef FORCE_HIGHPERFORMANCE_GPU
extern "C" {
    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
    _declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

namespace {
    static LARGE_INTEGER g_time;
}

void* malloc_aligned(const size_t size, size_t alignment) {
    return _aligned_malloc(size, alignment);
}

void  malloc_free(void*& ptr) {
	_aligned_free(ptr);
}

LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return FALSE;
}

namespace Divide {
    void getWindowHandle(void* window, SysInfo& info) {
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(static_cast<SDL_Window*>(window), &wmInfo);
        info._windowHandle = wmInfo.info.win.window;
    }

    bool CheckMemory(const U32 physicalRAMNeeded, SysInfo& info) {
        MEMORYSTATUSEX status; 
        GlobalMemoryStatusEx(&status);
        info._availableRam = status.ullAvailPhys;
        return info._availableRam > physicalRAMNeeded;
    }

    void getTicksPerSecond(TimeValue& ticksPerSecond) {
        bool queryAvailable = QueryPerformanceFrequency(&g_time) != 0;
        DIVIDE_ASSERT(queryAvailable,
            "Current system does not support 'QueryPerformanceFrequency calls!");
        ticksPerSecond = g_time.QuadPart;
    }

    void getCurrentTime(TimeValue& timeOut) {
        QueryPerformanceCounter(&g_time);
        timeOut = g_time.QuadPart;
    }

    void addTimeValue(TimeValue& timeInOut, const U64 value) {
    	timeInOut.QuadPart += value;
    }

    void subtractTimeValue(TimeValue& timeInOut, const U64 value) {
    	timeInOut.QuadPart -= value;
    }

    void divideTimeValue(TimeValue& timeInOut, const U64 value) {
    	timeInOut.QuadPart /= value;
    }

    void assignTimeValue(TimeValue& timeInOut, const U64 value) {
    	timeInOut.QuadPart = value;
    }

    U64 getUsTimeValue(const TimeValue& timeIn) {
    	return timeIn.QuadPart;
    }

}; //namespace Divide

#endif //defined(_WIN32)
