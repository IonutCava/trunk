#if defined(_WIN32)

#if defined(_DEBUG)
//#include <float.h>
//unsigned int fp_control_state = _controlfp(_EM_INEXACT, _MCW_EM);
#endif

#include "Headers/PlatformDefines.h"
#include <iostream>
#if defined(USE_VLD)
#include <vld.h>
#endif

// We are actually importing GL specific libraries in code mainly for
// maintenance reasons
// We can easily adjust them as needed. Same thing with PhysX libs
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "OpenAL32.lib")
#pragma comment(lib, "SDL2_mixer.lib")
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "OpenCL.lib")

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

#if defined(_PROFILE)
    #pragma comment(lib, "PhysXProfileSDKCHECKED.lib")
    #pragma comment(lib, "PhysX3CookingCHECKED_x64.lib")
    #pragma comment(lib, "PhysX3CHECKED_x64.lib")
    #pragma comment(lib, "PhysX3CommonCHECKED_x64.lib")
    #pragma comment(lib, "PhysX3ExtensionsCHECKED.lib")
    #pragma comment(lib, "PhysXVisualDebuggerSDKCHECKED.lib")
#else
    #pragma comment(lib, "PhysXProfileSDK.lib")
    #pragma comment(lib, "PhysX3Cooking_x64.lib")
    #pragma comment(lib, "PhysX3_x64.lib")
    #pragma comment(lib, "PhysX3Common_x64.lib")
    #pragma comment(lib, "PhysX3Extensions.lib")
    #pragma comment(lib, "PhysXVisualDebuggerSDK.lib")
#endif

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
    //https://msdn.microsoft.com/en-us/library/windows/desktop/ms679360%28v=vs.85%29.aspx
    static CHAR * getLastErrorText(CHAR *pBuf, LONG bufSize) {
        DWORD retSize;
        LPTSTR pTemp = NULL;

        if (bufSize < 16) {
            if (bufSize > 0) {
                pBuf[0] = '\0';
            }
            return(pBuf);
        }

        retSize = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                NULL,
                                GetLastError(),
                                LANG_NEUTRAL,
                                (LPTSTR)&pTemp,
                                0,
                                NULL);

        if (!retSize || pTemp == NULL) {
            pBuf[0] = '\0';
        } else {
            pTemp[strlen(pTemp) - 2] = '\0'; //remove cr and newline character
            sprintf(pBuf, "%0.*s (0x%x)", bufSize - 16, pTemp, GetLastError());
            LocalFree((HLOCAL)pTemp);
        }
        return(pBuf);
    }

    void getWindowHandle(void* window, SysInfo& info) {
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(static_cast<SDL_Window*>(window), &wmInfo);
        info._windowHandle = wmInfo.info.win.window;
    }

    bool PlatformInit() {
        return PlatformPostInit();
    }

    bool PlatformClose() {
        return true;
    }

    bool CheckMemory(const U32 physicalRAMNeeded, SysInfo& info) {
        MEMORYSTATUSEX status; 
        status.dwLength = sizeof(status);
        BOOL infoStatus = GlobalMemoryStatusEx(&status);
        if (infoStatus != FALSE) {
            info._availableRam = status.ullAvailPhys;
            return info._availableRam > physicalRAMNeeded;
        } else {
            CHAR msgText[256];
            getLastErrorText(msgText,sizeof(msgText));
            std::cerr << msgText << std::endl;
        }
        return false;
    }


    const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
    typedef struct tagTHREADNAME_INFO
    {
        DWORD dwType; // Must be 0x1000.
        LPCSTR szName; // Pointer to name (in user addr space).
        DWORD dwThreadID; // Thread ID (-1=caller thread).
        DWORD dwFlags; // Reserved for future use, must be zero.
    } THREADNAME_INFO;
#pragma pack(pop)

    void setThreadName(U32 threadID, const char* threadName) {
        // DWORD dwThreadID = ::GetThreadId( static_cast<HANDLE>( t.native_handle() ) );

        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = threadName;
        info.dwThreadID = threadID;
        info.dwFlags = 0;

        __try
        {
            RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    void setThreadName(const char* threadName) {
        setThreadName(GetCurrentThreadId(), threadName);
    }

    void setThreadName(std::thread* thread, const char* threadName) {
        DWORD threadId = ::GetThreadId(static_cast<HANDLE>(thread->native_handle()));
        setThreadName(threadId, threadName);
    }
    
}; //namespace Divide

#endif //defined(_WIN32)
